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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <string.h> 
#include <errno.h>


#include "cor.h"

/*---------------------------------------------------------------------------*/

Plugin* CreatePlugin(Device* device, const char* args)
{
  return (new COR(device));
}


/*---------------------------------------------------------------------------*/

void 
COR::init()
{
  
  log->info(IFUN,"Device initialization\n");


  hub  = DYNAMIC_CAST(SwitchPropertyVector*, device->find("HUB"));
  assert(connect != NULL);
  hub->on("DISCONNECT");

  
  configuration = DYNAMIC_CAST(SwitchPropertyVector*, device->find("CONFIGURATION"));
  assert(configuration != NULL);
  configuration->off();

  hardware  = DYNAMIC_CAST(SwitchPropertyVector*, device->find("HARDWARE"));
  assert(hardware != NULL);
  hardware->off();

  /* we do not reset values on these properties or else we loose stored values */

  ipAddress = DYNAMIC_CAST(NumberPropertyVector*, device->find("IP_ADDRESS"));
  assert(ipAddress != NULL);
  sprintf(ipCOR, "%d.%d.%d.%d",
	  STATIC_CAST(int,ipAddress->getValue("IP1")), 
	  STATIC_CAST(int,ipAddress->getValue("IP2")),  
	  STATIC_CAST(int,ipAddress->getValue("IP3")),  
	  STATIC_CAST(int,ipAddress->getValue("IP4")));  
  strcpy(maskCOR,"255.255.255.0");

  strcpy(ipPC, mux->getMyHostIP());
  
  udpPort  = DYNAMIC_CAST(NumberPropertyVector*, device->find("UDP_PORT"));
  assert(udpPort != NULL);

  ccdTemp  = DYNAMIC_CAST(NumberPropertyVector*, device->find("CCD_TEMP"));
  assert(ccdTemp != NULL);
  
  driver = DYNAMIC_CAST(TextPropertyVector*, device->find("DRIVER"));
  assert(driver != NULL);
  driver->setValue("DATE",__DATE__);
  driver->setValue("TIME",__TIME__);
  driver->setValue("PROGRAM", device->getParent()->getName());

  firmware = DYNAMIC_CAST(TextPropertyVector*, device->find("FIRMWARE"));
  assert(firmware != NULL);

  polling = DYNAMIC_CAST(NumberPropertyVector*, device->find("POLLING"));
  assert(polling != NULL);
  

  device->idleStatus();

  demux->add(this, PERI_COR);
  hubTimer->add(this);
  alarm = createAlarm();

  curState = CORIdle::instance();
  timeoutCount = 0;

}

/*---------------------------------------------------------------------------*/

void 
COR::handle(unsigned int event, const void* data, int len)
{
  Incoming_Message* msg = STATIC_CAST(Incoming_Message*, data);

  log->debug(IFUN,"%s\n",msg->body.keepAlive.response);
  
  if(!strcmp(MENS_ALIVE, msg->body.keepAlive.response)) {
    curState->presenceInd(this, msg);
  } else if(!strcmp(MENS_CONN_RESP, msg->body.connConf.response))
    curState->connectResp(this,msg);  
}

/*---------------------------------------------------------------------------*/


void 
COR::updateSensors(Incoming_Message* msg)
{

  /* sets the CCD temperatures in the ccdTemp property vector */
  
  updateCCDTemp(&msg->body.keepAlive.temp);
}

/*---------------------------------------------------------------------------*/

void 
COR::corDetected(Incoming_Message* msg)
{  
  char buf[3];

  updateSensors(msg);


  /* makes sure strings are null terminated */
  msg->body.keepAlive.compDate[11] = 0;
  msg->body.keepAlive.compTime[8]  = 0;
  msg->body.keepAlive.firmware[8]  = 0;

  // SEQ_KCOL is numeric => convert to ASCII

  buf[2] = 0;		
  snprintf(buf,2,"%d",msg->body.keepAlive.step);

  firmware->setValue("DATE", msg->body.keepAlive.compDate);
  firmware->setValue("TIME", msg->body.keepAlive.compTime);
  firmware->setValue("PROGRAM", msg->body.keepAlive.firmware);
  firmware->setValue("SEQ_KCOL", buf);
  firmware->indiSetProperty();

}

/*---------------------------------------------------------------------------*/

void
COR::save(char* name, ISState swit)
{

  /* do it regardless of connection status */

  if(swit == ISS_ON) 
    device->xmlSave(); 
  configuration->off("SAVE");
  configuration->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
COR::resetHw(char* name, ISState swit)
{
  Outgoing_Message msg;

  assert(swit == ISS_ON);
  hardware->setValue(name, swit);
  hardware->off("RESET");  
  hardware->indiSetProperty();

  msg.header.peripheal    = PERI_POWER;
  msg.body.resetReq.dummy = 0;
  mux->sendMessage(&msg, MSG_LEN(Reset_Req_Msg));
}

/*---------------------------------------------------------------------------*/

void
COR::setPolling(char* name[], double number[], int n)
{
  hubTimer->setPeriod(STATIC_CAST(unsigned int,number[0]));
  polling->setValue(name[0], number[0]);	// updates property
  polling->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
COR::setIP(char* name[], double number[], int n)
{

  for(int i=0; i<n; i++)
    ipAddress->setValue(name[i], number[i]);	// updates property
  ipAddress->indiSetProperty();

  sprintf(ipCOR, "%d.%d.%d.%d",
	  STATIC_CAST(int, ipAddress->getValue("IP1")),
	  STATIC_CAST(int, ipAddress->getValue("IP2")),  
	  STATIC_CAST(int, ipAddress->getValue("IP3")),  
	  STATIC_CAST(int, ipAddress->getValue("IP4")));  
}

/*---------------------------------------------------------------------------*/

void
COR::updateCCDTemp(CCD_Temp* temp)
{
 
  double cold  = (temp->ccd   * (1000.0/32768.0))-273.0;
  double hot   = (temp->box   * (1000.0/32768.0))-273.0;
  double vpelt = (temp->VPelt * (10.0/32768.0));

  ccdTemp->setValue("COLD", cold);
  ccdTemp->setValue("HOT" , hot);
  ccdTemp->setValue("VPELT",vpelt);

  ccdTemp->indiSetProperty();
 
}


/*---------------------------------------------------------------------------*/

void
COR::ping()
{
  sendKeepAlive();
  alarm->start(2*CONN_TIMEOUT);
}

/*---------------------------------------------------------------------------*/

void
COR::connectReq(char* name, ISState swit)
{
  hub->setValue(name, swit);	// updates property
  hub->indiSetProperty();

  setBroadcast(true);
  sendConnectReq();
  setBroadcast(false);
  alarm->start(CONN_TIMEOUT);
} 

/*---------------------------------------------------------------------------*/

void
COR::disconnectReq(char* name, ISState swit)
{
  hub->setValue(name, swit); // updates property
  alarm->cancel();
  setBroadcast(true); 
} 

/*---------------------------------------------------------------------------*/

void
COR::sendConnectReq()
{
  Outgoing_Message req;
  char myName[64];

  gethostname(myName, sizeof(myName)-1);
  device->formatMsg("Conectando al COR(%s) desde %s(%s)", ipCOR, myName, ipPC);
  device->indiMessage();

  req.header.peripheal = PERI_COR;
  req.header.origin    = ORIGIN_PC;
  strcpy(req.body.conReq.ipCOR, ipCOR);
  strcpy(req.body.conReq.maskCOR, maskCOR);
  strcpy(req.body.conReq.ipPC, ipPC);
  strcpy(req.body.conReq.request, MSG_REQUEST);

  gethostname(myName, sizeof(myName)-1 );
  mux->sendMessage(&req, MSG_LEN(Connect_Req_Msg));
}

/*---------------------------------------------------------------------------*/

void 
COR::corConnected(Incoming_Message* msg)
{
  timeoutCount = 0;
  alarm->cancel();
  updateSensors(msg);		// performs the same tasks
}

/*---------------------------------------------------------------------------*/

void 
COR::sendKeepAlive()
{
  Outgoing_Message msg;
    
  msg.header.peripheal = PERI_COR;
  msg.header.origin    = ORIGIN_PC;
  strcpy(msg.body.keepReq.manten, MSG_CONT);

  /* gracias a Dios corplus no hace caso de este byte */
  /* HAY QUE CAMBIAR EL PROTOCOLO COR */
  msg.body.keepReq.powerStatus = 0; 
  
  mux->sendMessage(&msg, MSG_LEN(KeepAlive_Req_Msg));
} 

/*---------------------------------------------------------------------------*/

void
COR::setRemoteSocket()
{
  mux->setPeer(ipCOR,STATIC_CAST(u_short,udpPort->getValue("PORT")));
}

/*---------------------------------------------------------------------------*/

void
COR::setBroadcast(bool flag)
{
  const char* addr = (flag) ? "255.255.255.255" : ipCOR;
  mux->setPeer(addr,STATIC_CAST(u_short,udpPort->getValue("PORT")));

}
