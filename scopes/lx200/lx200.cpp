/* $Id: $ */
/*

  Copyright (C) 2005 Rafael Gonzalez (astrorafael@yahoo.es)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/ 



#include <string.h> 
#include <errno.h>
#include <sys/types.h>
#include <regex.h>


#include "lx200.h"
#include "basiccmd.h"


/*---------------------------------------------------------------------------*/

const double 
LX200Simple::EPSILON_RA  = 5.0/(3600*15); // 5 arsec

const double 
LX200Simple::EPSILON_DEC = 5.0/(3600);    // 5 arsec

/*---------------------------------------------------------------------------*/

Plugin* CreatePlugin(Device* device,  const char* port)
{ 
  int perif;
  Plugin* teles;


  if((perif = cor_str2serial(port)) == -1) {
    return(0);
  }

  teles = new LX200Simple(device, perif);
  return(teles);
}

/*---------------------------------------------------------------------------*/

LX200Simple::LX200Simple(Device* dev, unsigned int perif) : 
  PluginBase(dev,"LX200Simple"), targetRA(0), targetDEC(0), 
  timeoutCount(0), perifNum(perif)
{
}


/*---------------------------------------------------------------------------*/

void 
LX200Simple::init()
{
  
  log->info(IFUN,"Device initialization\n");

  target = DYNAMIC_CAST(TextPropertyVector*, device->find("TARGET"));
  assert(target != NULL);
  
  edbLine = DYNAMIC_CAST(TextPropertyVector*, device->find("EDB"));
  assert(edbLine != NULL);

  fitsTextData = DYNAMIC_CAST(TextPropertyVector*, device->find("FITS_TEXT_DATA"));
  assert(fitsTextData != NULL);
  
  mount = DYNAMIC_CAST(TextPropertyVector*, device->find("MOUNT"));
  assert(mount != NULL);
  
  eqCoords = DYNAMIC_CAST(NumberPropertyVector*, device->find("EQUATORIAL_COORD"));
  assert(eqCoords != NULL);

  optics = DYNAMIC_CAST(NumberPropertyVector*, device->find("OPTICS"));
  assert(optics != NULL);

  configuration  = DYNAMIC_CAST(SwitchPropertyVector*, device->find("CONFIGURATION"));
  assert(configuration != NULL);
  configuration->off();

  abortMotion  = DYNAMIC_CAST(SwitchPropertyVector*, device->find("ABORT_MOTION"));
  assert(abortMotion != NULL);

  movement  = DYNAMIC_CAST(SwitchPropertyVector*, device->find("MOVEMENT"));
  assert(movement != NULL);

  onCoordSet  = DYNAMIC_CAST(SwitchPropertyVector*, device->find("ON_COORD_SET"));
  assert(onCoordSet != NULL);

  slewRate  = DYNAMIC_CAST(SwitchPropertyVector*, device->find("SLEW_RATE"));
  assert(slewRate != NULL);

  coordFormat  = DYNAMIC_CAST(SwitchPropertyVector*, device->find("COORD_FORMAT"));
  assert(coordFormat != NULL);

  rawCommand = DYNAMIC_CAST(TextPropertyVector*, device->find("RAW_COMMAND"));
  assert(rawCommand != NULL);

  /* Create the commands for this telescope model */

  rawCmd       = createQCommand(new RawCommand(this));
  abortCmd     = createQCommand(new Abort(this));
  getRaDec     = createQCommand(new MacroGetRaDec(this));
  getRaDecSlew = createQCommand(new MacroGetRaDecSlew(this));
  slewToTarget = createQCommand(new MacroSlewToTarget(this));
  syncRaDec    = createQCommand(new MacroSyncRaDec(this));
  getMount     = createQCommand(new MacroGetMount(this));

  rawCmd->setTimeout(5000);
  syncRaDec->setTimeout(5000);

  /* commands for testing the short/long format */
  testRACmd    = createQCommand(new TestRA(this));
  togglePrec   = createQCommand(new TogglePrecision(this));


  /* registers to the INDI infraestructure */
  demux->add(this, perifNum);

  device->idleStatus();
  device->indiSetProperty();

  /* initial values for test format state variables */
  nconv = 0;

}

/*---------------------------------------------------------------------------*/

// This is not used for the time being. Maybe I need it for a telescope
// that do not support the :D# command ...

#if 0				
bool
LX200Simple::matchedCoords() const
{
  return((fabs(targetRA  - eqCoords->getValue("RA"))  < EPSILON_RA) &&
	 (fabs(targetDEC - eqCoords->getValue("DEC")) < EPSILON_DEC)); 
}
#endif

/*---------------------------------------------------------------------------*/

void 
LX200Simple::unknown(PropertyVector* pv)
{
  pv->formatMsg("operacion sobre %s no reconocida", pv->getName());
  pv->forceChange();
  pv->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
LX200Simple::save(char* name, ISState swit)
{

  /* do it regardless of connection status */

  if(swit == ISS_ON) 
    device->xmlSave(); 
  configuration->off("SAVE");
  configuration->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
LX200Simple::toggleFormat(char* name, ISState swit)
{
  coordFormat->setValue(name,swit);
  coordFormat->forceChange();
  coordFormat->indiSetProperty();
  queue->add(togglePrec);
}

/*---------------------------------------------------------------------------*/

void
LX200Simple::update(PropertyVector* pvorig, ITopic topic)
{

  assert( !strcmp("COR",pvorig->getParent()->getName()));
  
  if(!pvorig->equals("HUB"))
    return;

  /* handles COR connenction */
  if(eqCoords->getState() == IPS_IDLE && pvorig->getState() == IPS_OK) {
    startFormatProcess();
    device->okStatus();		
    device->indiSetProperty();  

    /* handles COR disconnection */
  } else if(pvorig->getState() == IPS_IDLE) {
    hubTimer->remove(this);
    device->idleStatus();	
    device->indiSetProperty();  

    /* handles COR alert */
  } else  if(pvorig->getState() == IPS_ALERT) {
    corDisconnected = true;
    hubTimer->remove(this);
    device->alertStatus();	
    device->indiSetProperty();  

    /* handles COR reconnection */
  } else if(eqCoords->getState()==IPS_ALERT && corDisconnected && 
	    pvorig->getState() == IPS_OK) {
    corDisconnected = false;
    startFormatProcess();
    device->okStatus();		
    device->indiSetProperty();  
  }
}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::update(NumberPropertyVector* pv, char* name[], double num[], int n) 
{
  assert(pv->equals("EQUATORIAL_COORD"));

  /* do not send commands if not connected to COR or already busy or alarm */
  if(eqCoords->getState() != IPS_OK) {
    pv->forceChange();
    pv->indiSetProperty();
    return;
  }

  // sets the target values 

  for(int i=0; i<n; i++) {
    if(!strcmp(name[i],"RA"))
      targetRA  = num[i];
    else if(!strcmp(name[i],"DEC"))
      targetDEC = num[i];
  }

  /* schedules the proper command for execution */

  if(onCoordSet->getValue("SLEW"))
    queue->add(slewToTarget);
  else
    queue->add(syncRaDec);

}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::handle(unsigned int event, const void* data, int len)
{
  Command* cmd = queue->current();

  // Sending raw commands is dangerous. 
  // i.e. by sending :GD##  we receive the declination twice !
  // we ignore stale data if not cmd in execution.

  if(!cmd) {
    const char* p = STATIC_CAST(const char*,data);
    log->warn(IFUN,"No hay comando para dato %*c\n",
	      len-sizeof(Header), p+sizeof(Header));
    return;
  }

  cmd->handle(data, len);
}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::update(SwitchPropertyVector* pv, char* name, ISState swit) 
{ 
  if(pv->equals("CONFIGURATION")) 
    save(name, swit);
  else if(pv->equals("FORMAT_COORD") )
    toggleFormat(name,swit);
  else if(pv->equals("ABORT_MOTION") && (swit == ISS_ON)  )
    queue->add(abortCmd); 

}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::update(TextPropertyVector* pv, char* name[], char* text[], int n) 
{
  if(eqCoords->getState() == IPS_IDLE) {
    pv->forceChange();
    pv->indiSetProperty();
    return;
  }
   
  if(pv->equals("FITS_TEXT_DATA")) 
    updateFITSData(name, text, n);
  else if(pv->equals("TARGET"))
    updateTarget(name, text, n);
  else if(pv->equals("EDB"))
    updateEdbLine(name, text, n);
  else if(pv->equals("RAW_COMMAND"))
    updateRawCommand(name, text, n);
  else {
    unknown(pv);
  } 
}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::updateFITSData(char* name[], char* text[], int n)
{
  for(int i=0; i<n; i++) {
    if(strlen(text[i]))
       fitsTextData->setValue(name[i], text[i]);
  }

  fitsTextData->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::updateTarget(char* name[], char* text[], int n)
{
  target->setValue(name[0], text[0]);
  target->indiSetProperty();
  edbLine->setValue("LINE","");         // empties edb line
  edbLine->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::updateRawCommand(char* name[], char* text[], int n)
{
  int i;
  for(i=0; i<n; i++) {
    if(!strcmp(name[i],"REQUEST")) {
      rawCommand->setValue("REQUEST",text[i]);    
      queue->add(rawCmd);
    } else {
      rawCommand->setValue("RESPONSE","");
    }
  }

  rawCommand->busyStatus();
  rawCommand->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

bool
LX200Simple::parseEDBLine(const char* lin, char* objnm, double* ra, double* dec)
{
  int len;
  const char* p = lin;
  char coord[16+1];

  /* parses edb Field 1 until next subfield or field end or max size */
  /* copy input line into name */

  len = 0;
  while(*p != ',' && *p != '|' && len<68) {
    objnm[len++] = *p++;
  }

  objnm[len] = 0;			// end of object name string

  log->verbose(IFUN,"objname = %s\n", objnm);

  if(*p == '|') {
    while(*p++ != ',');		// if subfield, skip chars until Field 2
  } else
    p++;

  // We only have RA, DEC coordinates for Field 2 type 'f'

  log->verbose(IFUN,"object type = %c\n", *p);

  if(*p != 'f') {		
    *ra  = 0;
    *dec = 0;
    return(false);
  }


  while(*p++ != ',');  // skip to Field 3


  len = 0;
  while(*p != ',' && *p != '|' && len<16) {
    coord[len++] = *p++;
  }
  coord[len] = 0;

  log->verbose(IFUN,"RA = %s\n",coord);

  if( f_scansexa(coord, ra) == -1) {
    log->verbose(IFUN,"ar scansesa error for %s\n",coord);
  }
  
  // skip to Field 4

  if(*p == '|') {
    while(*p++ != ',');		// if subfieldd, skip chars until Field 4
  } else
    p++;

  log->verbose(IFUN,"skipped subfield 3\n");

  len = 0;
  while(*p != ',' && *p != '|' && len<16) {
    coord[len++] = *p++;
  }
  coord[len] = 0;

  log->verbose(IFUN,"DEC = %s\n",coord);
  
  if( f_scansexa(coord, dec) == -1) {
    log->verbose(IFUN,"dec scansesa error for %s\n",coord);
  }

  return(true);

}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::updateEdbLine(char* name[], char* text[], int n)
{
  
  /* parses the EDB line to extract Object's name and coordinates */
  /* updates related properties and notifies AUDINEs */

  char objname[68+1];
  double ra, dec;
  bool result;

  // skips empty lines

  if(!strlen(text[0])) {
    edbLine->forceChange();
    edbLine->indiSetProperty();
    return;
  }
     
  result = parseEDBLine(text[0], objname, &ra, &dec);

  log->debug(IFUN,"obj=%s, ra=%g, dec=%g\n",
	    objname, ra, dec);

  edbLine->setValue(name[0], text[0]);
  edbLine->indiSetProperty();
  
  target->setValue("NAME",objname);      
  target->indiSetProperty();

}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::tick()
{
  IPState state = eqCoords->getState();

  // do not send commands if not connected by COR
  if(state == IPS_IDLE)
    return;

  // schedules command for operation ignoring duplication errors
  queue->add(curMacroRaDec);	

}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::sendMessage(Outgoing_Message* msg, int len)
{
  msg->header.peripheal = perifNum;
  mux->sendMessage(msg, sizeof(Header)+len);
}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::startSlewing()
{
  eqCoords->formatMsg("Telescopio saltando a un nuevo objeto");
  eqCoords->indiMessage();
  eqCoords->busyStatus();
  eqCoords->indiSetProperty();

  /* inserts new polling macro */
  curMacroRaDec = getRaDecSlew;
}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::slewComplete(bool flag)
{
  if(flag) {
    eqCoords->formatMsg("Telescopio ha llegado al objeto");
    eqCoords->indiMessage();
    eqCoords->okStatus();
    eqCoords->indiSetProperty();

    /* restores previous polling macro */
    curMacroRaDec = getRaDec;
  }
}

/*---------------------------------------------------------------------------*/

void 
LX200Simple::testRA(double ra, int n)
{

  bool res;

  if(nconv == 0) {		// result of the first :GR#

    log->debug(IFUN,"n = %d, nconv = 0, primera recepcion\n",n);
    
    testedRA = ra;
    nconv = n;

    if(n == 2) { 		// short format => to long format
      res = queue->add(togglePrec);
      res = queue->add(testRACmd);

    } else {		

      // final state with long format

      coordFormat->on("LONG");
      coordFormat->formatMsg("Usando formato largo de coordenadas");
      coordFormat->indiMessage();
      coordFormat->indiSetProperty();
      getMountInfo();		//  starts next process

    }
    
    
  } else if(n == 2) {	

    // result of the second :GR# was short format again
    // this is a final state with short format
    
    log->debug(IFUN,"n = %d, nconv = 2, segunda  recepcion\n",n);

    nconv = 2;

    coordFormat->off("LONG");
    coordFormat->formatMsg("Usando el formato corto de coordenadas");
    coordFormat->indiMessage();
    coordFormat->indiSetProperty();
    getMountInfo();		// starts next process

    
  } else {			// result of 2nd :Gr# was long format
    
    log->debug(IFUN,"n = %d, nconv = 3, segunda  recepcion\n",n);
    assert(n == 3);
    nconv = 3;
    
    coordFormat->on("LONG");
    coordFormat->formatMsg("Pasando a formato largo de coordenadas");
    coordFormat->indiMessage();
    coordFormat->indiSetProperty();
    
    if(fabs(testedRA - ra) >= 1/600.0 ) {

      // final state back to short format again

      coordFormat->off("LONG");
      coordFormat->formatMsg("Posible perdida de precision en coordenadas, pasando a formato corto de nuevo");
      coordFormat->indiMessage();
      coordFormat->indiSetProperty();
      res = queue->add(togglePrec);
      nconv = 2;

    }

    getMountInfo();		// starts next process
  }
}



/*---------------------------------------------------------------------------*/
