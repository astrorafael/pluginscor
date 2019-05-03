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
#include <stdarg.h>

#include "trackscope.h"

/*---------------------------------------------------------------------------*/
/* SHARED LIBRARY ENTRY POINT */

Plugin* CreatePlugin(Device* device, const char* args)
{
  return (new TrackScope(device));
}

/*---------------------------------------------------------------------------*/

void 
TrackScope::init()
{
  
  log->info(IFUN,"Device initialization\n");

  object = DYNAMIC_CAST(TextPropertyVector*, device->find("OBJECT"));
  assert(object != NULL);
  
  edbLine = DYNAMIC_CAST(TextPropertyVector*, device->find("EDB"));
  assert(edbLine != NULL);

  fitsTextData = DYNAMIC_CAST(TextPropertyVector*, device->find("FITS_TEXT_DATA"));
  assert(fitsTextData != NULL);
  
  eqCoords = DYNAMIC_CAST(NumberPropertyVector*, device->find("EQUATORIAL_COORD"));
  assert(eqCoords != NULL);

  optics = DYNAMIC_CAST(NumberPropertyVector*, device->find("OPTICS"));
  assert(optics != NULL);

  configuration  = DYNAMIC_CAST(SwitchPropertyVector*, device->find("CONFIGURATION"));
  assert(configuration != NULL);
  configuration->off();

  
  device->idleStatus();
  device->indiSetProperty();

}

/*---------------------------------------------------------------------------*/
void 
TrackScope::unknown(PropertyVector* pv)
{
  pv->formatMsg("operacion sobre %s no reconocida", pv->getName());
  pv->forceChange();
  pv->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
TrackScope::save(char* name, ISState swit)
{

  /* do it regardless of connection status */

  if(swit == ISS_ON) 
    device->xmlSave(); 
  configuration->off("SAVE");
  configuration->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
TrackScope::update(PropertyVector* pvorig, ITopic topic)
{

  assert( !strcmp("COR",pvorig->getParent()->getName()));
  
  if(!pvorig->equals("HUB"))
    return;

  assert(topic == IT_STATE);

  if(eqCoords->getState() == IPS_IDLE && pvorig->getState() == IPS_OK) {
    device->okStatus();		/* handles connenction */
  } else if(eqCoords->getState() == IPS_OK && pvorig->getState() == IPS_IDLE) {
    device->idleStatus();	/* handles disconnection */
  } else  if(pvorig->getState() == IPS_ALERT) {
    device->alertStatus();	/* handles alert */
  } else if(eqCoords->getState()==IPS_ALERT && pvorig->getState() == IPS_OK) {
    device->okStatus();		/* handles reconnection */
  }
  device->indiSetProperty();  
}

/*---------------------------------------------------------------------------*/

/* NOTIFIES AUDINES OF COORDINATE CHANGES */

void 
TrackScope::update(NumberPropertyVector* pv, char* name[], double num[], int n) 
{
  if(eqCoords->getState() == IPS_IDLE) {
    pv->forceChange();
    pv->indiSetProperty();
    return;
  }
    
  /* no matter the property. change it and notify observers */

  for(int i=0; i<n; i++)
    pv->setValue(name[i], num[i]);
  pv->indiSetProperty();

}

/*---------------------------------------------------------------------------*/

void 
TrackScope::update(SwitchPropertyVector* pv, char* name, ISState swit) 
{
  save(name, swit);
}

/*---------------------------------------------------------------------------*/

void 
TrackScope::update(TextPropertyVector* pv, char* name[], char* text[], int n) 
{
  if(eqCoords->getState() == IPS_IDLE) {
    pv->forceChange();
    pv->indiSetProperty();
    return;
  }
   
  if(pv->equals("FITS_TEXT_DATA")) 
    updateFITSData(name, text, n);
  else if(pv->equals("OBJECT"))
    updateObject(name, text, n);
  else if(pv->equals("EDB"))
    updateEdbLine(name, text, n);
  else {
    unknown(pv);
  } 
}

/*---------------------------------------------------------------------------*/

void 
TrackScope::updateFITSData(char* name[], char* text[], int n)
{
  for(int i=0; i<n; i++) {
    if(strlen(text[i]))
       fitsTextData->setValue(name[i], text[i]);
  }

  fitsTextData->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
TrackScope::updateObject(char* name[], char* text[], int n)
{
  object->setValue(name[0], text[0]);
  object->indiSetProperty();
  edbLine->setValue("LINE","");         // empties edb line
  edbLine->indiSetProperty();

}

/*---------------------------------------------------------------------------*/

bool
TrackScope::parseEDBLine(const char* lin, char* objnm, double* ra, double* dec)
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

  if( ::f_scansexa(coord, ra) == -1) {
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
  
  if( ::f_scansexa(coord, dec) == -1) {
    log->verbose(IFUN,"dec scansesa error for %s\n",coord);
  }

  return(true);

}

/*---------------------------------------------------------------------------*/

#if 0 /* ORIGINAL CODE */
void 
TrackScope::updateEdbLine(char* name[], char* text[], int n)
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

  if(!result) {
    edbLine->formatMsg("No puedo calcular la posicion para esta linea EDB");
    edbLine->forceChange();
    edbLine->indiSetProperty();
    object->setValue("NAME",objname); // changes object name anyway
    object->indiSetProperty();
    return;
  }

  log->debug(IFUN,"obj=%s, ra=%g, dec=%g\n",
	    objname, ra, dec);

  edbLine->setValue(name[0], text[0]);
  edbLine->indiSetProperty();
  
  object->setValue("NAME",objname);      
  object->indiSetProperty();

  eqCoords->setValue("RA",ra);
  eqCoords->setValue("DEC",dec);
  eqCoords->indiSetProperty();
  
  changed(eqCoords);
  changed(object);
}

#else

void 
TrackScope::updateEdbLine(char* name[], char* text[], int n)
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
  
  object->setValue("NAME",objname);      
  object->indiSetProperty();

}

#endif

/*---------------------------------------------------------------------------*/


