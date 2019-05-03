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

#ifndef COR_POWER_H
#define COR_POWER_H

#include <indicor/api.h>

/*******************************/
/* THE PLUGIN FACTORY FUNCTION */
/*******************************/

BEGIN_C_DECLS

Plugin* CreatePlugin(Device*,  const char* args);

END_C_DECLS

class CORPower : public PluginBase
{
 public:

  CORPower(Device* dev ) : 
    PluginBase(dev, "CORPower") {}
  virtual ~CORPower() {}

  
  /* ****************** */
  /* THE INDI INTERFACE */
  /* ****************** */

  virtual void init();

  virtual void 
    update(SwitchPropertyVector* pv, char* name, ISState swit);
  
 
  /*************************/
  /* The UDP event handler */
  /*************************/

  virtual void handle(unsigned int event, const void* data, int len);

  /**************************************/
  /* COR CONTROL TIMER DRIVEN FUNCTIONS */
  /**************************************/
  
  /* periodic poll method */
  void tick();

  /***************************************************/
  /* events comming from other Devices (observables) */
  /***************************************************/

  virtual void
    update(PropertyVector* pv, ITopic t);

  /**************************/
  /* interface for commands */
  /**************************/

  /* sends a message with length len, including header+payload */
  void sendMessage(Outgoing_Message* msg, int len);

  /* updates power properties because of COR responses */
  void updatePowerStatus(u_char relayStatus);

  /* updates lights in a given input group */
  void updateInput(int group, u_char input);

  /* as a 8-bit bitfield */
  unsigned char getActuatorsStatus() const;

 private:

  /*********************************/
  /* POWER CONTROL USER'S REQUESTS */
  /*********************************/

  /* methods serving user requests */

  void arm(char* name, ISState swit);
  void trigger(char* name, ISState swit);
  void forbidden(PropertyVector* pv);

  /*********************************/
  /* THE USER INTERFACE PROPERTIES */
  /*********************************/

  SwitchPropertyVector* actuatorsTrg;
  SwitchPropertyVector* actuatorsArm;
  LightPropertyVector*  actuatorsStat;
  LightPropertyVector*  inputsG[4];

  /*******************************************/
  /* commands used to drive the power device */
  /*******************************************/

  Command*   statusCmd;		/* periodic query for inputs/outputs */
  Command* setPowerCmd;		/* set new outputs */

  /****************************/
  /* other private attributes */
  /****************************/

  unsigned char armed;		/* armed actuators */
  unsigned char actuStat;	/* actuators status as a bitfield */

    
};

/*---------------------------------------------------------------------------*/

inline unsigned char
CORPower::getActuatorsStatus() const
{
  return(actuStat);
}

/*---------------------------------------------------------------------------*/


#endif
