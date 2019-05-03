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

#ifndef COR_DEVICE_H
#define COR_DEVICE_H


#include <indicor/api.h>


/*******************************/
/* THE PLUGIN FACTORY FUNCTION */
/*******************************/

BEGIN_C_DECLS

Plugin* CreatePlugin(Device*,  const char* args);

END_C_DECLS


class CORState;			/* forward reference */

class COR : public PluginBase
{

 protected:

  friend class CORState;	/* base class */
  friend class CORIdle;
  friend class COROk;	
  friend class CORBusy;	
  friend class CORAlert;

 public:

  static const int MAX_TIMEOUTS = 3; /* retries */
  static const int CONN_TIMEOUT = 2000;	/* milliseconds */

  COR(Device* dev ) : PluginBase(dev, "COR") {}
  virtual ~COR() {}

  void nextState(CORState* st);

  
  /* ****************** */
  /* THE INDI INTERFACE */
  /* ****************** */

  virtual void init();

  virtual void 
    update(SwitchPropertyVector* pv, char* name, ISState swit);
  
  virtual void 
    update(NumberPropertyVector* pv, char* name[], double num[], int n);

  /*************************/
  /* The UDP event handler */
  /*************************/

  virtual void handle(unsigned int event, const void* data, int len);

  /**************************************/
  /* COR CONTROL TIMER DRIVEN FUNCTIONS */
  /**************************************/

  /* connection timeout */
  void timeout();
  
  /* periodic poll method */
  void tick();

  /***************************************************/
  /* events comming from other Devices (observables) */
  /***************************************************/

  virtual void
    update(PropertyVector* pv, ITopic t);

  /************************/
  /* other public methods */
  /************************/

 protected:

  /* Save current device configuration to XML file */
  void save(char* name, ISState swit);


 
  /***************************/
  /* RESPONSES FROM HARDWARE */
  /***************************/

  /* updates CCD temperature property items */
  void updateCCDTemp(CCD_Temp* temp);

  /**************************************/
  /* COR CONTROL TIMER DRIVEN FUNCTIONS */
  /**************************************/

  /* send a keep alive message and starts a timer */
  void ping();

  /*******************************/
  /* COR CONTROL USER'S REQUESTS */
  /*******************************/

  void connectReq(char* name, ISState swit);
  void disconnectReq(char* name, ISState swit);

  void setIP(char* name[], double number[], int n);
  void setPolling(char* name[], double number[], int n);
  void resetHw(char* name, ISState swit);

  /***************************************/
  /* COR CONTROL RESPONSES FROM HARDWARE */
  /***************************************/

  /* methods serving COR UDP responses */

  void corDetected(Incoming_Message* msg);
  void corConnected(Incoming_Message* msg);

  /********************************/
  /* COR CONTROL HELPER FUNCTIONS */
  /********************************/

  void setRemoteSocket();
  void setBroadcast(bool flag);
  void sendConnectReq();
  void sendKeepAlive();

  /* update Power and CCD sensors with incoming data from COR */
  void updateSensors(Incoming_Message* msg);
  

  /*********************************/
  /* THE USER INTERFACE PROPERTIES */
  /*********************************/

  SwitchPropertyVector* hub;
  SwitchPropertyVector* hardware;
  SwitchPropertyVector* configuration;
  NumberPropertyVector* ccdTemp;
  NumberPropertyVector* udpPort;
  NumberPropertyVector* ipAddress;
  NumberPropertyVector* polling;
  TextPropertyVector*   driver;
  TextPropertyVector*   firmware;

  /****************************/
  /* other private attributes */
  /****************************/

  int step;			/* COR sequencer step size */

  char ipCOR[16];
  char maskCOR[16];
  char ipPC[16];

  int timeoutCount;		/* # timeouts counting responses form ping */

  PluginAlarm* alarm;

  /* ******************************** */
  /* Other relationships with objects */
  /* ******************************** */
  
  CORState* curState;		/* current state */
  
};

#ifndef COR_STATE_H
#include "state.h"
#endif

/*---------------------------------------------------------------------------*/

inline void 
COR::nextState(CORState* st) 
{

  /******** too verbose ...
  device->formatMsg("Cambiando estado de %s a %s",
		    curState->getName(), st->getName());
		    device->xmlMessage();
  *************/
  curState = st;
}

/*---------------------------------------------------------------------------*/

inline void
COR::tick()
{
  curState->tick(this);
}

/*---------------------------------------------------------------------------*/

inline void
COR::timeout()
{
  curState->timeout(this);
}

/*---------------------------------------------------------------------------*/

inline void     
COR::update(PropertyVector* pv, ITopic topic) 
{
  curState->update(this, pv, topic);
}

/*---------------------------------------------------------------------------*/

inline void 
COR::update(SwitchPropertyVector* pv, char* name, ISState swit) 
{
  curState->update(this, pv, name, swit);
  pv->indiSetProperty();
}
  
/*---------------------------------------------------------------------------*/

inline void 
COR::update(NumberPropertyVector* pv, char* name[], double num[], int n) 
{
  curState->update(this, pv, name, num, n);
  pv->indiSetProperty();
}

/*---------------------------------------------------------------------------*/



#endif
