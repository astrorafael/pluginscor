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


#include "audine.h"


/*---------------------------------------------------------------------------*/

AudineState* AudineIdle::_instance  = 0;
AudineState* AudineWait::_instance  = 0;
AudineState* AudineExp::_instance   = 0;
AudineState* AudineRead::_instance  = 0;
AudineState* AudineAlert::_instance = 0;
AudineState* AudineOk::_instance    = 0;

/*---------------------------------------------------------------------------*/

AudineState* 
AudineIdle::instance()
{
  if(! _instance)
    _instance = new AudineIdle();
  return(_instance);
}

AudineState* 
AudineOk::instance()
{
  if(! _instance)
    _instance = new AudineOk();
  return(_instance);
}

AudineState* 
AudineWait::instance()
{
  if(! _instance)
    _instance = new AudineWait();
  return(_instance);
}

AudineState* 
AudineExp::instance()
{
  if(! _instance)
    _instance = new AudineExp();
  return(_instance);
}

AudineState* 
AudineRead::instance()
{
  if(! _instance)
    _instance = new AudineRead();
  return(_instance);
}

AudineState* 
AudineAlert::instance()
{
  if(! _instance)
    _instance = new AudineAlert();
  return(_instance);
}

/*---------------------------------------------------------------------------*/

/*****************************************************************************/
/* ************************** ABSTRACT STATE *********************************/
/*****************************************************************************/


AudineState::AudineState(const char* tag) 
{
  log = LogFactory::instance()->forClass(tag);
}

void 
AudineState::forbidden(PropertyVector* pv)
{
  pv->formatMsg("operacion para %s no permitida en estado %s",
		pv->getName(), getName());
  pv->forceChange();
  pv->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
AudineState::handle(Audine* ccd, unsigned int event, const void* data, int len)
{
  log->debug(IFUN,"ignored COR data in Audine state %s\n", getName());
}

/*---------------------------------------------------------------------------*/

void
AudineState::update(Audine* ccd, PropertyVector* pv, ITopic t)
{
  if(!ccd->chip.updateTemp(pv)) {
    ccd->device->formatMsg("cambios en %s ignorados en estado %s",
			   pv->getName(), getName());
    ccd->device->indiMessage();
  }
}

/*---------------------------------------------------------------------------*/

void 
AudineState::tick(Audine* ccd)
{
  ccd->device->formatMsg("tick() ignorado en estado %s", getName());
  ccd->device->indiMessage();
}

/*---------------------------------------------------------------------------*/

void 
AudineState::timeout(Audine* ccd)
{
  ccd->device->formatMsg("timeout() ignorado en estado %s", getName());
  ccd->device->indiMessage();
}

/*---------------------------------------------------------------------------*/

void
AudineState::defaultUpd(Audine* ccd, PropertyVector* pvorig, ITopic t)
{
  /* test first for temperature updates */
  if(ccd->chip.updateTemp(pvorig))
    return;

  /* test for updated information from telescope */
  if(ccd->updateFromTelescope(pvorig))
    return;

  /* test for updated firmware information */
  if(ccd->updateFromFirmware(pvorig)) {
    return;
  }

  assert(pvorig->equals("HUB"));
  assert(t == IT_STATE);

  SwitchPropertyVector* pv = DYNAMIC_CAST(SwitchPropertyVector*,pvorig);
  SwitchProperty* sel = DYNAMIC_CAST(SwitchProperty*, pv->find("CONNECT"));

  /*
   * if disconnect request (COR pv state is IDLE), Audine goes to Idle state
   * if disconnect indication (COR pv is ALERT), Audine goes to the Alert state
   */

  if(!sel->getValue()) {  
    if(pv->getState() == IPS_IDLE) {
      nextState(ccd, AudineIdle::instance());
    } else {
      nextState(ccd, AudineAlert::instance());
    }
  } 

}

/*****************************************************************************/
/* **************************** IDLE STATE ***********************************/
/*****************************************************************************/

/*---------------------------------------------------------------------------*/

void
AudineIdle::setVisualState(Audine* ccd)
{
  ccd->ccdStatus->idle();	// all lights to gray
  ccd->ccdStatus->ok("IDLE");	// set IDLE status to green
  ccd->device->idleStatus();	// All CCD device to IDLE status
  ccd->device->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
AudineIdle::update(Audine* ccd, SwitchPropertyVector* pv,
		  char* name, ISState swit) 
{
  if(pv->equals("CONFIGURATION"))
    ccd->doSave(name, swit);
  else 
    forbidden(pv);
}

/*---------------------------------------------------------------------------*/

void 
AudineIdle::update(Audine* ccd, NumberPropertyVector* pv,
		 char* name[], double number[], int n) 
{
  forbidden(pv);
}

/*---------------------------------------------------------------------------*/

void 
AudineIdle::update(Audine* ccd, TextPropertyVector* pv,
	       char* name[], char* text[], int n)
{
  forbidden(pv);
}


/*---------------------------------------------------------------------------*/

void
AudineIdle::update(Audine* ccd, PropertyVector* pvorig, ITopic t)
{

  /* test first for temperature updates */
  if(ccd->chip.updateTemp(pvorig))
    return;
  
  /* test for updated information from telescope */
  if(ccd->updateFromTelescope(pvorig))
    return;

  /* test for updated firmware information */
  if(ccd->updateFromFirmware(pvorig)) 
    return;
  

  /* for the time being no need to check where it comes from */
  
  assert(pvorig->equals("HUB"));
  assert(t == IT_STATE);

  SwitchPropertyVector* pv = DYNAMIC_CAST(SwitchPropertyVector*, pvorig);
  SwitchProperty* sel = DYNAMIC_CAST(SwitchProperty*, pv->find("CONNECT"));

  /* if connect is on, then Audine goes to Ok state */

  if(sel->getValue()) {  
    nextState(ccd, AudineOk::instance());
  } 
}


/*****************************************************************************/
/* ****************************  OK  STATE ***********************************/
/*****************************************************************************/

/*---------------------------------------------------------------------------*/

void
AudineOk::setVisualState(Audine* ccd)
{
  ccd->ccdStatus->idle();	// all lights to gray
  ccd->ccdStatus->ok("OK");	// set OK light to green
  ccd->device->okStatus();	// all device Status is OK
  ccd->imgseq.exposure->off("START");	// button to original position
  ccd->imgseq.exposure->on("STOP");	// button to original position
  ccd->device->indiSetProperty();
  ccd->chip.updateAreaSelection(); // except for some area selections
}

/*---------------------------------------------------------------------------*/

void
AudineOk::exposure(Audine* ccd, SwitchPropertyVector* pv,
		  char* name, ISState swit) 
{
  assert(swit == ISS_ON);
  assert(!strcmp(name,"START"));
  int width, height;

  pv->setValue(name, swit);
  pv->busyStatus();
  pv->indiSetProperty();

  // marks the start of the exposure process

  ccd->chip.getDim(&width, &height);
  ccd->storage.start(width, height);

  log->verbose(IFUN,"expected dim cols x rows = %d x %d\n",width,height);

  // and continues to the proper state

  if(ccd->imgseq.hasDelay()) {
    ccd->imgseq.startFromWait();
    nextState(ccd, AudineWait::instance());
  } else {
    ccd->imgseq.startFromExp();
    nextState(ccd, AudineExp::instance());
  }  
}

/*---------------------------------------------------------------------------*/

void
AudineOk::update(Audine* ccd, PropertyVector* pvorig, ITopic t)
{
  defaultUpd(ccd, pvorig, t);
}

/*---------------------------------------------------------------------------*/

 void 
 AudineOk::update(Audine* ccd, SwitchPropertyVector* pv,
		  char* name, ISState swit) 
{

  if(pv->equals("AREA_SELECTION")) 
    ccd->chip.updateAreaSelection(name, swit);
  else if(pv->equals("AREA_PRESETS"))
    ccd->chip.updateAreaPresets(name, swit);
  else if(pv->equals("ADC_SPEED"))
    ccd->chip.updateADCSpeed(name, swit);
  else if(pv->equals("BINNING"))
    ccd->chip.updateBinning(name, swit);
  else if(pv->equals("CHIP"))
    ccd->chip.updateChip(name, swit);
  else if(pv->equals("EXPOSURE"))
    exposure(ccd, pv, name, swit);	// warning
  else if(pv->equals("IMAGETYP")) {
    ccd->updateImageType(name, swit);
    ccd->storage.updatePrefix(ccd->getImageType());
  }
  else if(pv->equals("PATTERN"))
    ccd->doPattern(name, swit);
  else if(pv->equals("SHUTTER_LOGIC"))
    ccd->shutter.updateLogic(name, swit);
  else if(pv->equals("CONFIGURATION"))
    ccd->doSave(name, swit);
  else if(pv->equals("STORAGE_FLIP"))
    ccd->storage.updateFlip(name, swit);
  else if(pv->equals("STORAGE_SERIES"))
    ccd->storage.updateSeries(name, swit);
  else {
    forbidden(pv);
  }
}

/*---------------------------------------------------------------------------*/

void 
AudineOk::update(Audine* ccd, NumberPropertyVector* pv,
		 char* name[], double number[], int n) 
{
  if(pv->equals("AREA_DIM_RECT")) 
    ccd->chip.updateAreaDimRect(name, number, n);
  else if(pv->equals("AREA_PRESET_SIZE"))
    ccd->chip.updateAreaPresetSize(name, number, n);
  else if(pv->equals("EXP_LIMITS"))
    ccd->imgseq.updateExpLimits(name, number, n);
  else if(pv->equals("PHOT_PARS"))
    ccd->chip.updatePhotPars(name, number, n);
  else if(pv->equals("WCS_SEED"))
    ccd->updateWCSSeed(name, number, n);
  else if(pv->equals("CCD_CLEAN"))
    ccd->doCCDClean(name, number, n);
  else if(pv->equals("SHUTTER_DELAY"))
    ccd->shutter.updateDelay(name, number, n);
  else if(pv->equals("FOCUS_BUFFER"))
    ccd->storage.update(name, number, n);
  else {
    forbidden(pv);
  }
}

/*---------------------------------------------------------------------------*/
 
void 
AudineOk::update(Audine* ccd, TextPropertyVector* pv,
	       char* name[], char* text[], int n)
{
  if(pv->equals("FITS_TEXT_DATA")) 
    ccd->updateFITSTextData(name, text, n);
  else if(pv->equals("STORAGE"))
    ccd->storage.update(name, text, n);
  else {
    forbidden(pv);
  }
}
 
/*****************************************************************************/
/* **************************** WAIT STATE ***********************************/
/*****************************************************************************/

/*---------------------------------------------------------------------------*/

void
AudineWait::setVisualState(Audine* ccd)
{
  ccd->ccdStatus->idle();	// all lights to gray
  ccd->ccdStatus->busy("WAIT");	// set WAIT light to yellow
  ccd->ccdStatus->busyStatus();	// Property status to BUSY
  ccd->ccdStatus->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
AudineWait::update(Audine* ccd, PropertyVector* pvorig, ITopic t)
{
  defaultUpd(ccd, pvorig, t);
}

/*---------------------------------------------------------------------------*/

void
AudineWait::exposure(Audine* ccd, SwitchPropertyVector* pv,
		  char* name, ISState swit) 
{
  assert(swit == ISS_ON);
  assert(!strcmp(name,"STOP"));

  pv->setValue(name, swit);
  pv->indiSetProperty();

  ccd->imgseq.cancelFromWait(true);
  ccd->storage.cancel();
  nextState(ccd, AudineOk::instance());
}

/*---------------------------------------------------------------------------*/

 void 
 AudineWait::update(Audine* ccd, SwitchPropertyVector* pv,
		  char* name, ISState swit) 
{
  if(pv->equals("EXPOSURE"))
    exposure(ccd, pv, name, swit);	// warning
  else {
    forbidden(pv);
  }
}

/*---------------------------------------------------------------------------*/

void
AudineWait::tick(Audine* ccd)
{
  log->verbose(IFUN,"\n");
  ccd->imgseq.decDelay();
}

/*---------------------------------------------------------------------------*/

void
AudineWait::timeout(Audine* ccd)
{
  ccd->imgseq.stopTickTimer();
  ccd->imgseq.restartFromExp();
  nextState(ccd, AudineExp::instance());

}

/*****************************************************************************/
/* **************************** EXP  STATE ***********************************/
/*****************************************************************************/
 
/*---------------------------------------------------------------------------*/

void
AudineExp::setVisualState(Audine* ccd)
{
  ccd->ccdStatus->idle();	// all lights to gray
  ccd->ccdStatus->busy("EXP");	// set EXP light to yellow
  ccd->ccdStatus->busyStatus();	// Property status to BUSY
  ccd->ccdStatus->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
AudineExp::update(Audine* ccd, PropertyVector* pvorig, ITopic t)
{
  defaultUpd(ccd, pvorig, t);
}

/*---------------------------------------------------------------------------*/

void
AudineExp::exposure(Audine* ccd, SwitchPropertyVector* pv,
		  char* name, ISState swit) 
{
  assert(swit == ISS_ON);
  assert(!strcmp(name,"STOP"));

  pv->setValue(name, swit);
  pv->indiSetProperty();

  ccd->imgseq.cancelFromExp(true);
  ccd->storage.cancel();
  nextState(ccd, AudineOk::instance());
}

/*---------------------------------------------------------------------------*/

 void 
 AudineExp::update(Audine* ccd, SwitchPropertyVector* pv,
		  char* name, ISState swit) 
{
  if(pv->equals("EXPOSURE"))
    exposure(ccd, pv, name, swit);	
  else {
    forbidden(pv);
  }
}

/*---------------------------------------------------------------------------*/

void
AudineExp::tick(Audine* ccd)
{
  log->verbose(IFUN,"\n");
  ccd->imgseq.decExptime();
}

/*---------------------------------------------------------------------------*/

void
AudineExp::timeout(Audine* ccd)
{
  ccd->imgseq.stopTickTimer();
  ccd->imgseq.cancelFromExp(false);
  ccd->storage.cancel();
  nextState(ccd, AudineAlert::instance());
}

/*---------------------------------------------------------------------------*/

void 
AudineExp::handle(Audine* ccd, unsigned int event, const void* data, int len)
{
  int width, height;

  assert(event == STATIC_CAST(unsigned int,ccd->perifNum));
  ccd->chip.getDim(&width, &height);
  //ccd->imgseq.initRx(ccd->chip.expectedUDPMsgs());
  ccd->imgseq.initRx(width, height);
  ccd->storage.handle(data, len);
  ccd->imgseq.handle(data, len);
  nextState(ccd, AudineRead::instance());
}

/*****************************************************************************/
/* **************************** READ  STATE *********************************/
/*****************************************************************************/

/*---------------------------------------------------------------------------*/

void
AudineRead::setVisualState(Audine* ccd)
{
  ccd->ccdStatus->idle();	 // all lights to gray
  ccd->ccdStatus->busy("READ");  // set READ light to yellow
  ccd->ccdStatus->busyStatus();  // Property status to ALERT
  ccd->ccdStatus->indiSetProperty();
}

/*---------------------------------------------------------------------------*/


void 
AudineRead::handle(Audine* ccd, unsigned int event, const void* data, int len)
{
  bool waitNeeded;
  int  imageCount;

  if(event == STATIC_CAST(unsigned int, ccd->perifNum)) {

    // Stores next chunk of data and advances pointer

    ccd->storage.handle(data, len);
    ccd->imgseq.handle(data, len);
  
  } else if(event == STATIC_CAST(unsigned int, ccd->perifNum+1)) {

    // end-of-image received. 
    // take exposure end timestamp and calculates exposure start
    // timestamp from exposure lenght ad shutter delay.
    // Saves the data on a fits file
    // If not the last image, goes to the wait or expo state 
    // and starts al over again

    // TAKE EXPOSURE AND SAVE TO FITS HEADER


    waitNeeded = ccd->imgseq.hasDelay();
    ccd->imgseq.handleFinal(data, len);
    imageCount = ccd->imgseq.getCount();
    ccd->device->formatMsg("Quedan %d imagenes",imageCount);
    ccd->device->indiMessage();
      
    if(imageCount > 0 && waitNeeded) {  

      ccd->storage.next();      
      ccd->imgseq.restartFromWait();
      nextState(ccd, AudineWait::instance());

    } else if(imageCount > 0 && !waitNeeded) {
      
      ccd->storage.next();      
      ccd->imgseq.restartFromExp();
      nextState(ccd, AudineExp::instance());
    
    }  else {			// no more images

      ccd->storage.end();      
      nextState(ccd, AudineOk::instance());

    } 

  } else {

    assert((event == unsigned(ccd->perifNum)) || (event == unsigned(ccd->perifNum+1)));
  }

}

/*---------------------------------------------------------------------------*/

void
AudineRead::tick(Audine* ccd)
{
  log->debug(IFUN,"ignored\n");
}

/*---------------------------------------------------------------------------*/

void
AudineRead::timeout(Audine* ccd)
{
  nextState(ccd, AudineAlert::instance());
}

/*---------------------------------------------------------------------------*/

void
AudineRead::update(Audine* ccd, PropertyVector* pvorig, ITopic t)
{
  /* test only for temperature updates, if any */
  if(ccd->chip.updateTemp(pvorig))
    return;

  /* test for updated information from telescope */
  if(ccd->updateFromTelescope(pvorig))
    return;
}

/*****************************************************************************/
/* **************************** ALERT  STATE *********************************/
/*****************************************************************************/

/*---------------------------------------------------------------------------*/

void
AudineAlert::setVisualState(Audine* ccd)
{
  ccd->ccdStatus->idle();	  // all lights to gray
  ccd->ccdStatus->alert("ALERT"); // set ALERT light to red
  ccd->device->idleStatus();      // set all device statuses to IDLE
  ccd->ccdStatus->alertStatus();  // except this property to ALERT
  ccd->imgseq.exposure->off("START");	// button to original position
  ccd->imgseq.exposure->on("STOP");	// button to original position
  ccd->device->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
AudineAlert::update(Audine* ccd, PropertyVector* pvorig, ITopic t)
{

  /* for the time being no need to check where it comes from */
  /* It always comes from the COR object */

  /* test first for temperature updates */
  if(ccd->chip.updateTemp(pvorig))
    return;

  /* test for updated information from telescope */
  if(ccd->updateFromTelescope(pvorig))
    return;

  /* test for updated firmware information */
  if(ccd->updateFromFirmware(pvorig)) 
    return;

  /* handle COR connecton changes
   * if connect is On and state is IPS_OK, 
   * then COR reconnected sucssfully
   */

  assert(pvorig->equals("HUB"));
  assert(t == IT_STATE);

  SwitchPropertyVector* pv = DYNAMIC_CAST(SwitchPropertyVector*,pvorig);
  SwitchProperty* sel = DYNAMIC_CAST(SwitchProperty*, pv->find("CONNECT"));


  if(sel->getValue() && (pv->getState() == IPS_OK) ) {  
    ccd->imgseq.restart();
    nextState(ccd, AudineOk::instance());
  } 
}
