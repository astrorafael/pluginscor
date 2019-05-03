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

#include <memory.h>

#include "powercmd.h"
#include "corpower.h"

/*---------------------------------------------------------------------------*/

Outgoing_Message SetPower::msg;

/*---------------------------------------------------------------------------*/

SetPower::SetPower(CORPower* corpower)
  : log(0), cor(corpower), busy(false)
{
  log = LogFactory::instance()->forClass("SetPower");
  actuatorsTrg = STATIC_CAST(SwitchPropertyVector*, cor->getDevice()->find("ACTUATORS_TRG"));
  assert(actuatorsTrg != NULL);

}

/*---------------------------------------------------------------------------*/

void
SetPower::request()
{
  msg.body.powerReq.relays = cor->getActuatorsStatus();
  cor->sendMessage(&msg, MSG_LEN(Power_Req_Msg));
  actuatorsTrg->busyStatus();
  actuatorsTrg->indiSetProperty();
  busy = true;
  log->debug(IFUN,"Enviando peticion de estado de potencia al COR");
}

/*---------------------------------------------------------------------------*/

void
SetPower::handle(const void* data, int len)
{
   /* this will be routed to the current command */

  Incoming_Message* msg = STATIC_CAST(Incoming_Message*, data);
  log->debug(IFUN,"Power response\n");
  
  /* sets lights in input property vectors */
  
  cor->updateInput(0, msg->body.powerResp.group[0]);
  cor->updateInput(1, msg->body.powerResp.group[1]);
  cor->updateInput(2, msg->body.powerResp.group[2]);
  cor->updateInput(3, msg->body.powerResp.group[3]);
  
  /* sets the relay outputs in actuators property vector */
   
  cor->updatePowerStatus(msg->body.powerResp.relays);
  busy=false;
}


/*---------------------------------------------------------------------------*/

void
SetPower::response()
{
  actuatorsTrg->okStatus();
  actuatorsTrg->off();
  actuatorsTrg->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
SetPower::timeout()
{
  actuatorsTrg->formatMsg("Perdido el contacto con el modulo de potencia");
  actuatorsTrg->indiMessage();
  actuatorsTrg->alertStatus();
  actuatorsTrg->off();
  actuatorsTrg->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

Outgoing_Message PowerStatus::msg;

/*---------------------------------------------------------------------------*/

PowerStatus::PowerStatus(CORPower* corpower)
  : log(0), cor(corpower), busy(false)
{
  log = LogFactory::instance()->forClass("PowerStatus");

  actuatorsStat = DYNAMIC_CAST(LightPropertyVector*, cor->getDevice()->find("ACTUATORS_STATUS"));
  assert(actuatorsStat != NULL);

  static const char* inputName[] = {"INPUTS_G1", "INPUTS_G2", "INPUTS_G3", "INPUTS_G4"}; 
  for(int i=0; i<4; i++) {
    inputsG[i] = DYNAMIC_CAST(LightPropertyVector*, cor->getDevice()->find(inputName[i]));
    assert(inputsG[i] != NULL);
  }
}

/*---------------------------------------------------------------------------*/

void
PowerStatus::request()
{
  log->debug(IFUN,"\n");

  /* HE AQUI EL PROBLEMA: Como al iniciar el servidor, ACTUATOR_STATUS
     esta a cero, pues inevitablemente apagamos los reles cada vez que
     re-arrancamos el servidor. */
  /* HAY QUE CAMBIAR EL PROTOCOLO COR */
  msg.body.powerReq.relays = cor->getActuatorsStatus();
  cor->sendMessage(&msg, MSG_LEN(Power_Req_Msg));
  busy = true;
}

/*---------------------------------------------------------------------------*/

void
PowerStatus::handle(const void* data, int len)
{

  Incoming_Message* msg = STATIC_CAST(Incoming_Message*, data);
  log->debug(IFUN,"Power response\n");
  
  /* sets lights in input property vectors */
  
  cor->updateInput(0, msg->body.powerResp.group[0]);
  cor->updateInput(1, msg->body.powerResp.group[1]);
  cor->updateInput(2, msg->body.powerResp.group[2]);
  cor->updateInput(3, msg->body.powerResp.group[3]);
  
  /* sets the relay outputs in actuators property vector */
   
  cor->updatePowerStatus(msg->body.powerResp.relays);
  busy = false;
}

/*---------------------------------------------------------------------------*/

void
PowerStatus::timeout()
{

  log->debug(IFUN,"\n");

  for(int i=0; i<4; i++) {
    inputsG[i]->alertStatus();
    inputsG[i]->indiSetProperty();
  }

  actuatorsStat->formatMsg("Perdido el contacto con el modulo de potencia");
  actuatorsStat->indiMessage();
  actuatorsStat->alertStatus();
  actuatorsStat->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

