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

#include "corpower.h"
#include "powercmd.h"

/*---------------------------------------------------------------------------*/

Plugin* CreatePlugin(Device* device, const char* args)
{
  return (new CORPower(device));
}

/*---------------------------------------------------------------------------*/

void 
CORPower::init()
{  
  log->info(IFUN,"Device initialization\n");

  actuatorsStat = DYNAMIC_CAST(LightPropertyVector*, device->find("ACTUATORS_STATUS"));
  assert(actuatorsStat != NULL);
  actuatorsStat->idle();

  
  actuatorsArm  = DYNAMIC_CAST(SwitchPropertyVector*, device->find("ACTUATORS_ARM"));
  assert(actuatorsArm != NULL);
  actuatorsArm->off();


  actuatorsTrg  = DYNAMIC_CAST(SwitchPropertyVector*, device->find("ACTUATORS_TRG"));
  assert(actuatorsTrg != NULL);
  actuatorsTrg->off();

  static const char* inputName[] = {"INPUTS_G1", "INPUTS_G2", "INPUTS_G3", "INPUTS_G4"}; 
  for(int i=0; i<4; i++) {
    inputsG[i] = DYNAMIC_CAST(LightPropertyVector*, device->find(inputName[i]));
    assert(inputsG[i] != NULL);
    inputsG[i]->idleStatus();  
    inputsG[i]->idle();
  }
  
  /* create commands */
  statusCmd   = createQCommand(new PowerStatus(this));
  setPowerCmd = createQCommand(new SetPower(this));

  statusCmd->setTimeout(5000);
  /* register to lower layers */
  demux->add(this, PERI_POWER);
  hubTimer->add(this);

  device->idleStatus();
}

/*---------------------------------------------------------------------------*/

void 
CORPower::tick()
{

  // do not send commands if not connected by COR
  if(actuatorsStat->getState() == IPS_IDLE)
    return;

  // schedules command for operation ignoring duplication errors
  bool res = queue->add(statusCmd);
  log->debug(IFUN,"queue->add() devuelve %d\n",res);
}

/*---------------------------------------------------------------------------*/

void 
CORPower::sendMessage(Outgoing_Message* msg, int len)
{
  msg->header.peripheal = PERI_POWER;  
  mux->sendMessage(msg, len);
}


/*---------------------------------------------------------------------------*/

void 
CORPower::handle(unsigned int event, const void* data, int len)
{
  Command* cmd = queue->current();

  log->debug(IFUN,"\n");
  // we ignore stale data if not cmd in execution. 
  if(!cmd) {
    log->warn(IFUN,"No hay comando para dato\n");
    return;
  }
  
  cmd->handle(data, len);    
}

/*---------------------------------------------------------------------------*/

void 
CORPower::forbidden(PropertyVector* pv)
{
  pv->formatMsg("operacion sobre %s no permitida", pv->getName());
  pv->indiMessage();
  pv->forceChange();
  pv->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
CORPower::update(SwitchPropertyVector* pv, char* name, ISState swit) 
{

  /* ignores requests in the wrong state */

  if(pv->getState() != IPS_OK) {
    pv->formatMsg("%s: Operacion no permitida en este estado",pv->getName());
    pv->indiMessage();
    pv->forceChange();
    pv->indiSetProperty();
    return;
  }
      
  if(pv->equals("ACTUATORS_ARM"))
    arm(name, swit);
  else if(pv->equals("ACTUATORS_TRG"))
    trigger(name, swit);
  else {
    forbidden(pv);
  }
}
  

/*---------------------------------------------------------------------------*/

void
CORPower::arm(char* name, ISState swit)
{
  actuatorsArm->setValue(name, swit);	// updates property
  actuatorsArm->indiSetProperty();

  armed = 0x00;
  armed |= actuatorsArm->getValue("OUTPUT1") ? 0x01 : 0x00 ;
  armed |= actuatorsArm->getValue("OUTPUT2") ? 0x02 : 0x00 ;
  armed |= actuatorsArm->getValue("OUTPUT3") ? 0x04 : 0x00 ;
  armed |= actuatorsArm->getValue("OUTPUT4") ? 0x08 : 0x00 ;
  armed |= actuatorsArm->getValue("OUTPUT5") ? 0x10 : 0x00 ;
  armed |= actuatorsArm->getValue("OUTPUT6") ? 0x20 : 0x00 ;
  armed |= actuatorsArm->getValue("OUTPUT7") ? 0x40 : 0x00 ;
  armed |= actuatorsArm->getValue("OUTPUT8") ? 0x80 : 0x00 ;  
}


/*---------------------------------------------------------------------------*/

void
CORPower::trigger(char* name, ISState swit)
{

  if(!strcmp(name,"SWITCH_ON"))
    actuStat |=  armed;
  else if(!strcmp(name,"SWITCH_OFF"))
    actuStat &= ~armed;
  
  queue->add(setPowerCmd);	/* send command to COR */

  /* anyway, returns arm switches to default position */
  
  armed = 0x00;
  actuatorsArm->off();
  actuatorsArm->indiSetProperty();
}


/*---------------------------------------------------------------------------*/

void 
CORPower::updatePowerStatus(u_char status)
{
  actuStat = status;
  actuatorsStat->ok();
  
  log->debug(IFUN,"reles = 0x%x\n",status);
  if (status & 0x01)
     actuatorsStat->alert("OUTPUT1");
  if (status & 0x02)
     actuatorsStat->alert("OUTPUT2");
  if (status & 0x04)
     actuatorsStat->alert("OUTPUT3");
  if (status & 0x08)
     actuatorsStat->alert("OUTPUT4");
  if (status & 0x10)
     actuatorsStat->alert("OUTPUT5");
  if (status& 0x20)
     actuatorsStat->alert("OUTPUT6");
  if (status & 0x40)
     actuatorsStat->alert("OUTPUT7");
  if (status & 0x80)
     actuatorsStat->alert("OUTPUT8");
     
  actuatorsStat->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
CORPower::updateInput(int i, u_char input)
{
  inputsG[i]->ok();
  
  if (input & 0x01)
    inputsG[i]->alert("INPUT1");
  if (input & 0x02)
    inputsG[i]->alert("INPUT2");
  if (input & 0x04)
    inputsG[i]->alert("INPUT3");
  if (input & 0x08)
    inputsG[i]->alert("INPUT4");
  if (input & 0x10)
    inputsG[i]->alert("INPUT5");
  if (input& 0x20)
    inputsG[i]->alert("INPUT6");
  
  inputsG[i]->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
CORPower::update(PropertyVector* pvorig, ITopic topic)
{

  assert( !strcmp("COR",pvorig->getParent()->getName()));
  
  if(!pvorig->equals("HUB"))
    return;

  IPState curState = actuatorsStat->getState();
  IPState corState = pvorig->getState();

  /* handles COR first connenction/reconnection */
  if( (curState == IPS_IDLE || curState == IPS_ALERT) && corState == IPS_OK) {
    hubTimer->add(this);
    device->okStatus();		
    actuatorsArm->off();
    actuatorsTrg->off();
    device->indiSetProperty();  

    /* handles COR disconnection */
  } else if(corState == IPS_IDLE) {
    hubTimer->remove(this);
    actuatorsArm->off();
    actuatorsTrg->off();
    actuatorsStat->idle();
    inputsG[0]->idle();
    inputsG[1]->idle();
    inputsG[2]->idle();
    inputsG[3]->idle();
    device->idleStatus();	
    device->indiSetProperty();  

    /* handles COR alert */
  } else  if(corState == IPS_ALERT) {
    hubTimer->remove(this);
    actuatorsArm->off();
    actuatorsTrg->off();
    device->alertStatus();	
    device->indiSetProperty();  
  } 
}

/*---------------------------------------------------------------------------*/

