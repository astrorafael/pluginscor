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


#include <errno.h>
#include <string.h> 


#include "audine.h"


void 
Shutter::init()
{

  shutterLogic = STATIC_CAST(SwitchPropertyVector*, audine->device->find("SHUTTER_LOGIC"));
  assert(shutterLogic != NULL);

  shutterDelay = STATIC_CAST(NumberPropertyVector*, audine->device->find("SHUTTER_DELAY"));
  assert(shutterDelay != NULL);


  update();
}

/*---------------------------------------------------------------------------*/

double
Shutter::getDelay()
{
  double delay;

  if( audine->imagetyp->getValue("BIAS") || audine->imagetyp->getValue("FOCUS") )
    delay = 0.0;
  else
    delay = shutterDelay->getValue("DELAY");

  return(delay);
}

/*---------------------------------------------------------------------------*/

void
Shutter::update()
{
  bool inverted = false;


  if(shutterLogic->getValue("POSITIVE")) 
    inverted = false;
  else if(shutterLogic->getValue("NEGATIVE"))
    inverted = true;
  else {
    assert("Unhandled property item" == 0);
  }

  // mask shutterMode for possible usage of other bits in the future
  audine->req.body.imageReq.shutterMode &= 0xFC;

  if(audine->imagetyp->getValue("BIAS")) {

    if(inverted)
      audine->req.body.imageReq.shutterMode |= 0x3; 
    else
      audine->req.body.imageReq.shutterMode |= 0x2; 

  } else if(audine->imagetyp->getValue("DARK")) {

    if(inverted)
      audine->req.body.imageReq.shutterMode |= 0x3; 
    else
      audine->req.body.imageReq.shutterMode |= 0x2;

  } else if(audine->imagetyp->getValue("FLAT")) {

    if(inverted)
      audine->req.body.imageReq.shutterMode |= 0x1; 
    else
      audine->req.body.imageReq.shutterMode |= 0x0; 

  } else if(audine->imagetyp->getValue("FOCUS")) {

    if(inverted)
      audine->req.body.imageReq.shutterMode |= 0x2; 
    else
      audine->req.body.imageReq.shutterMode |= 0x3;

  } else if(audine->imagetyp->getValue("OBJECT")) {

    if(inverted)
      audine->req.body.imageReq.shutterMode |= 0x1; 
    else
      audine->req.body.imageReq.shutterMode |= 0x0; 

  } else {

    assert("Unhandled property item" == 0);
  }

  // converts from milliseconds to 1/10th seconds

  u_int16 delay = STATIC_CAST(u_int16, getDelay())/100;
  audine->req.body.imageReq.delay = delay;

  // sets inital FITS keywords

  audine->fits.set("SHUTDLY", delay/10.0, "[s] shutter delay");

}

/*---------------------------------------------------------------------------*/

void
Shutter::updateDelay(char* name[], double number[], int n)
{

  assert (n == 1);
  shutterDelay->setValue("DELAY", number[0]);

  u_int16 delay = STATIC_CAST(u_int16, getDelay())/100;
  audine->req.body.imageReq.delay = delay;
  audine->fits.set("SHUTDLY", delay/10.0, "[s] shutter delay");
  shutterDelay->indiSetProperty();

}

/*---------------------------------------------------------------------------*/

void 
Shutter::updateLogic(char* name, ISState swit)
{
  shutterLogic->setValue(name, swit); 
  shutterLogic->indiSetProperty();
  update();
}

/*---------------------------------------------------------------------------*/



