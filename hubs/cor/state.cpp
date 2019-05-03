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


#include "cor.h"


/*---------------------------------------------------------------------------*/

CORState* CORIdle::_instance  = 0;
CORState* COROk::_instance    = 0;
CORState* CORBusy::_instance  = 0;
CORState* CORAlert::_instance = 0;


/*---------------------------------------------------------------------------*/

CORState* 
CORIdle::instance()
{
  if(! _instance)
    _instance = new CORIdle();
  return(_instance);
}

CORState* 
COROk::instance()
{
  if(! _instance)
    _instance = new COROk();
  return(_instance);
}

CORState* 
CORBusy::instance()
{
  if(! _instance)
    _instance = new CORBusy();
  return(_instance);
}

CORState* 
CORAlert::instance()
{
  if(! _instance)
    _instance = new CORAlert();
  return(_instance);
}

/*---------------------------------------------------------------------------*/

/*****************************************************************************/
/* ************************** ABSTRACT STATE *********************************/
/*****************************************************************************/


CORState::CORState(const char* tag) :
  log(0)
{
  log = LogFactory::instance()->forClass(tag);
}

void 
CORState::forbidden(PropertyVector* pv)
{
  pv->formatMsg("operacion sobre %s no permitida en estado %s",
		pv->getName(), getName());
  pv->forceChange();
  pv->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
CORState::connectResp(COR* cor, Incoming_Message* msg)
{
  log->debug(IFUN,"mensaje COR connectResp ignorado en estado %s\n", getName());
}

/*---------------------------------------------------------------------------*/

void 
CORState::presenceInd(COR* cor, Incoming_Message* msg)
{
  cor->updateSensors(msg);
  cor->device->formatMsg("emitiendo en broadcast: %s",
      			msg->body.keepAlive.response);
  cor->device->indiMessage();
}


/*---------------------------------------------------------------------------*/

void
CORState::update(COR* cor, PropertyVector* pv, ITopic t)
{
  log->verbose(IFUN,"cambio sobre %s ignorado en estado %s\n",
			 pv->getName(), getName());
}

/*---------------------------------------------------------------------------*/

void
CORState::tick(COR* cor)
{

}

/*---------------------------------------------------------------------------*/

void 
CORState::timeout(COR* cor)
{
  cor->device->formatMsg("timeout() ignorado en estado %s", getName());
  cor->device->indiMessage();
}

/*---------------------------------------------------------------------------*/


void
CORState::defUpdate(COR* cor, PropertyVector* pvorig, ITopic t)
{

  /* for the time being no need to check where it comes from */
  /* it comes from the AUDINE objects */
  
  assert( !strcmp("AUDINE1",pvorig->getParent()->getName()) || 
	  !strcmp("AUDINE2",pvorig->getParent()->getName()));
  assert(t == IT_VALUE);

  LightPropertyVector* pv = DYNAMIC_CAST(LightPropertyVector*,pvorig);
  LightProperty*      sel = DYNAMIC_CAST(LightProperty*, pv->find("READ"));

  // if audine has entered the READ state, disables polling, else enables it
  
  if(sel->getValue() != IPS_IDLE ) {
    cor->alarm->cancel();
    cor->hubTimer->disable(); // globally disabled for all SyncTimer clients
  } else {
    cor->hubTimer->enable(); // globaly enabled for all SyncTimer clients
  }
}


/*---------------------------------------------------------------------------*/

void 
CORState::minimalUpdate(COR* cor, SwitchPropertyVector* pv,
		  char* name, ISState swit) 
{
  if(pv->equals("CONFIGURATION"))
    cor->save(name, swit);
  else if(pv->equals("HARDWARE"))
    cor->resetHw(name, swit);
  else if(pv->equals("HUB"))  
    connectReq(cor, pv, name, swit);
  else 
    forbidden(pv);
}

/*---------------------------------------------------------------------------*/

void 
CORState::minimalUpdate(COR* cor, NumberPropertyVector* pv,
		 char* name[], double number[], int n) 
{
  if(pv->equals("IP_ADDRESS"))
    cor->setIP(name, number, n);
  if(pv->equals("POLLING"))
    cor->setPolling(name, number, n);
  else
    forbidden(pv);
}

/*---------------------------------------------------------------------------*/

void
CORState::connectReq(COR* cor, SwitchPropertyVector* pv, 
		    char* name, ISState swit) 
{
  if(!strcmp(name,"CONNECT" ) && swit == ISS_ON) {
    cor->connectReq(name, swit);
    nextState(cor, CORBusy::instance()); 
  } else {
    pv->forceChange();
    pv->indiSetProperty();
  }
}

/*---------------------------------------------------------------------------*/

void
CORState::disconnectReq(COR* cor, SwitchPropertyVector* pv, 
		    char* name, ISState swit) 
{
  if(!strcmp(name,"DISCONNECT" )  && swit == ISS_ON) {
    cor->disconnectReq(name, swit);
    nextState(cor, CORIdle::instance()); 
  } else {
    pv->forceChange(); 
    pv->indiSetProperty();
  }
}


/*****************************************************************************/
/* **************************** IDLE STATE ***********************************/
/*****************************************************************************/

/*---------------------------------------------------------------------------*/

void
CORIdle::setVisualState(COR* cor)
{
  cor->device->idleStatus();	// All COR device to IDLE status
  cor->device->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
CORIdle::presenceInd(COR* cor, Incoming_Message* msg)
{
  cor->corDetected(msg);
  cor->device->formatMsg("emitiendo en broadcast: %s",
      			msg->body.keepAlive.response);
  cor->device->indiMessage();
}

/*---------------------------------------------------------------------------*/

void 
CORIdle::update(COR* cor, SwitchPropertyVector* pv,
		  char* name, ISState swit) 
{
  minimalUpdate(cor, pv, name, swit);
}

/*---------------------------------------------------------------------------*/

void 
CORIdle::update(COR* cor, NumberPropertyVector* pv,
		 char* name[], double number[], int n) 
{
  minimalUpdate(cor, pv, name, number, n);
}


/*****************************************************************************/
/* ****************************  OK  STATE ***********************************/
/*****************************************************************************/

void
COROk::setVisualState(COR* cor)
{
  cor->device->okStatus();	// all device Status is OK
  cor->device->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
COROk::update(COR* cor, PropertyVector* pvorig, ITopic t)
{
  defUpdate(cor, pvorig, t);
}

/*---------------------------------------------------------------------------*/

 void 
 COROk::update(COR* cor, SwitchPropertyVector* pv,
		  char* name, ISState swit) 
{

  if(pv->equals("HUB")) 
    disconnectReq(cor, pv, name, swit);
  else if(pv->equals("HARDWARE"))
    cor->resetHw(name, swit);
  else if(pv->equals("CONFIGURATION"))
    cor->save(name, swit);
  else {
    forbidden(pv);
  }
}

/*---------------------------------------------------------------------------*/

void
COROk::tick(COR* cor)
{
  cor->ping();
  nextState(cor, CORBusy::instance());
}

/*---------------------------------------------------------------------------*/



/*****************************************************************************/
/* **************************** BUSY STATE ***********************************/
/*****************************************************************************/

void
CORBusy::setVisualState(COR* cor)
{
  cor->hub->busyStatus();	// Property status to BUSY
  cor->hub->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
CORBusy::update(COR* cor, PropertyVector* pvorig,  ITopic t)
{
  defUpdate(cor, pvorig, t);
}

/*---------------------------------------------------------------------------*/

 void 
 CORBusy::update(COR* cor, SwitchPropertyVector* pv,
		  char* name, ISState swit) 
{

  if(pv->equals("HUB")) 
    disconnectReq(cor, pv, name, swit);
  else if(pv->equals("HARDWARE"))
    cor->resetHw(name, swit);
  else if(pv->equals("CONFIGURATION"))
    cor->save(name, swit);
  else {
    forbidden(pv);
  }
}

/*---------------------------------------------------------------------------*/

void
CORBusy::tick(COR* cor)
{
  cor->ping();
}

/*---------------------------------------------------------------------------*/

void
CORBusy::timeout(COR* cor)
{
  log->debug(IFUN,"\n");

  if(cor->timeoutCount++ < COR::MAX_TIMEOUTS) {
    cor->device->formatMsg("Perdida puntual de contacto");
    cor->device->indiMessage();
    log->warn(IFUN,"Temporary lost contact with COR\n");
  } else {
    cor->setBroadcast(true);
    nextState(cor, CORAlert::instance());
  }
}

/*---------------------------------------------------------------------------*/

void
CORBusy::connectResp(COR* cor, Incoming_Message* msg)
{
  cor->corConnected(msg);
  nextState(cor, COROk::instance());    
}


/*****************************************************************************/
/* **************************** ALERT  STATE *********************************/
/*****************************************************************************/

/*---------------------------------------------------------------------------*/

void
CORAlert::setVisualState(COR* cor)
{
  cor->device->idleStatus();
  cor->hub->alertStatus();  
  cor->hub->on("DISCONNECT");
  cor->hub->formatMsg("Perdida definitiva de contacto");
  cor->device->indiSetProperty();
  log->error(IFUN,"lost contact with COR\n");
}

/*---------------------------------------------------------------------------*/

void 
CORAlert::update(COR* cor, NumberPropertyVector* pv,
		 char* name[], double number[], int n) 
{
  minimalUpdate(cor,pv,name,number,n);
}

/*---------------------------------------------------------------------------*/

void 
CORAlert::update(COR* cor, SwitchPropertyVector* pv,
		  char* name, ISState swit) 
{
  minimalUpdate(cor, pv, name, swit);
  cor->timeoutCount = 0;	// reset it anyway
}

/*---------------------------------------------------------------------------*/
