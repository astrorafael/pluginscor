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


/* after lots of number crunching and least squares fitting we have
   come to these factors. There is no need for more precision.
   Data is duplicated for both slow and fast ADC sequences. 
   Do not have data for high speed sequences yet
*/

/*---------------------------------------------------------------------------*/

static const
Timeout timeParam[] = {
  {				// slow ADC
    0.057,			// Tv in milliseconds
    0.006,			// Tsk in milliseconds
    0.023,			// Th in milliseconds
    3.5				// K1 in milliseconds
  },
  {				// fast ADC
    0.057,			// Tv in milliseconds
    0.006,			// Tsk in milliseconds
    0.023,			// Th in milliseconds
    3.5				// K1 in milliseconds
  }
};


/*---------------------------------------------------------------------------*/

ImageSequencer::ImageSequencer(Audine* aud) :
    log(0), audine(aud) 
{
  log = LogFactory::instance()->forClass("ImageSequencer");
  timer = audine->createTimer(); // creates and register itself
  alarm = audine->createAlarm(); // idem
  timer->setPeriod(TICK);
}

/*---------------------------------------------------------------------------*/

ImageSequencer::~ImageSequencer() 
{
  delete log;
  delete alarm;
  delete timer;
}

/*---------------------------------------------------------------------------*/

void 
ImageSequencer::init()
{

  /************************/
  /* resetable properties */
  /************************/

  exposure  = STATIC_CAST(SwitchPropertyVector*, audine->device->find("EXPOSURE"));
  assert(exposure != NULL);
  exposure->on("STOP");

  expCounters = STATIC_CAST(NumberPropertyVector*, audine->device->find("EXP_COUNTERS"));
  assert(expCounters != NULL);
  expCounters->setValue("EXPTIME",0);
  expCounters->setValue("DELAY",0);
  expCounters->setValue("COUNT",0);
  expCounters->setValue("PROGRESS",0);

  expLimits = STATIC_CAST(NumberPropertyVector*, audine->device->find("EXP_LIMITS"));
  assert(expLimits != NULL);
  expLimits->setValue("EXPTIME",0);
  expLimits->setValue("DELAY",0);
  expLimits->setValue("COUNT",1);

  /************************/
  /* final initialization */
  /************************/

  updateMessage();
}

/*---------------------------------------------------------------------------*/

void 
ImageSequencer::restart()
{
  exposure->on("STOP");
  exposure->indiSetProperty();
  cancelFromWait(false);
}

/*---------------------------------------------------------------------------*/

void 
ImageSequencer::updateMessage()
{
  double exptime  = expLimits->getValue("EXPTIME");
  u_int16 seconds = STATIC_CAST(u_int16, exptime); 
  u_int16 msecs   = STATIC_CAST(u_int16, 1000.0*(exptime - seconds)); 

  audine->req.body.imageReq.tSecExp = seconds; 
  audine->req.body.imageReq.tMsecExp = msecs; 
}

/*---------------------------------------------------------------------------*/

void 
ImageSequencer::updateImageType(const char* imageType) 
{
  if(imageType == Audine::BIAS) {
    expLimits->setValue("EXPTIME", 0);
    expLimits->setValue("DELAY", 0);
    updateMessage();
    expLimits->indiSetProperty();
  } else if (imageType == Audine::DARK) {
    expLimits->setValue("DELAY", 0);
    updateMessage();
    expLimits->indiSetProperty();
  }
}

/*---------------------------------------------------------------------------*/

void 
ImageSequencer::updateExpLimits(char* name[], double number[], int n)
{
  for(int i=0; i<n; i++) {
    expLimits->setValue(name[i], number[i]);
  }
  expLimits->indiSetProperty();

  // updates Audine request message
  updateMessage();

}

/*---------------------------------------------------------------------------*/

void
ImageSequencer::restartFromWait()
{
  // reload counters with limits except for the "loop" count

  expCounters->setValue("DELAY",   expLimits->getValue("DELAY"));
  expCounters->setValue("EXPTIME", expLimits->getValue("EXPTIME"));
  expCounters->setValue("PROGRESS", 0);
  expCounters->indiSetProperty();

  timer->start();

  int t = STATIC_CAST(int, expLimits->getValue("DELAY"));

  alarm->start(1000*t);

}

/*---------------------------------------------------------------------------*/

void
ImageSequencer::startFromWait()
{
  expCounters->setValue("COUNT",   expLimits->getValue("COUNT"));
  restartFromWait();
}

/*---------------------------------------------------------------------------*/

void
ImageSequencer::restartFromExp()
{

  // reload counters with limits except for the "loop" count

  expCounters->setValue("DELAY",   0);
  expCounters->setValue("EXPTIME", expLimits->getValue("EXPTIME") + audine->shutter.getDelay()/1000);
  expCounters->setValue("PROGRESS",0);
  expCounters->indiSetProperty();

  computeTimeouts();

  timer->stop();
  timer->start();

  alarm->start(expTimeout);

  audine->sendImageMsg();

}

/*---------------------------------------------------------------------------*/

void
ImageSequencer::startFromExp()
{
  expCounters->setValue("COUNT",   expLimits->getValue("COUNT"));
  restartFromExp();
}

/*---------------------------------------------------------------------------*/

void
ImageSequencer::cancelFromWait(bool userReq)
{
  if(userReq) {
    timer->stop();
    alarm->cancel();
  }

  // reload counters with default values

  expCounters->setValue("COUNT",   0);
  expCounters->setValue("DELAY",   0);
  expCounters->setValue("EXPTIME", 0);
  expCounters->setValue("PROGRESS",0);
  expCounters->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
ImageSequencer::cancelFromExp(bool userReq)
{
  if(userReq) {
    timer->stop();
    alarm->cancel();
  }
  
  // reload counters with default values

  expCounters->setValue("COUNT",   0);
  expCounters->setValue("DELAY",   0);
  expCounters->setValue("EXPTIME", 0);
  expCounters->setValue("PROGRESS",0);
  expCounters->indiSetProperty();

  audine->sendCancelImageMsg();
}

/*---------------------------------------------------------------------------*/

void
ImageSequencer::decDelay()
{
  double delay = expCounters->getValue("DELAY");
  delay -=  TICK/1000.0;
  expCounters->setValue("DELAY", delay);
  expCounters->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
ImageSequencer::decExptime()
{
  double exptime = expCounters->getValue("EXPTIME");
  exptime -= TICK/1000.0 ;
  expCounters->setValue("EXPTIME", exptime);
  expCounters->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
ImageSequencer::decCount()
{
  double count = expCounters->getValue("COUNT");
  count -= 1.0 ;
  expCounters->setValue("COUNT", count);
  expCounters->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
ImageSequencer::initRx(int width, int height)
{
  timer->stop();
  alarm->cancel();
  byteCounter = 0; 
  imageSize = sizeof(pixel_t)*width*height; 

  // inits a new timeout for readout stage
  // the timeout is predicted by audchip taking into account
  // experimental data, image size, binning, etc.

  alarm->start(readTimeout);
}

/*---------------------------------------------------------------------------*/

void 
ImageSequencer::handle(const void* data, int len)
{

  byteCounter += (len - IMG_HEAD);
  expCounters->setValue("PROGRESS", 100 * byteCounter/imageSize);
  expCounters->indiSetProperty();

}

/*---------------------------------------------------------------------------*/

void 
ImageSequencer::handleFinal(const void* data, int len)
{
  static char ts[32];
  time_t t;
  double duration, exptime, readtime;
  
  time (&t);			// takes 'end of exposure' timestamp
  

  Incoming_Message* msg = STATIC_CAST(Incoming_Message*, data);

  alarm->cancel();
  decCount();


  // fixes duration FITS keywords from COR timestamps

  exptime   = msg->body.imgEnd.readTime - msg->body.imgEnd.expTime;
  readtime  = msg->body.imgEnd.endTime  - msg->body.imgEnd.readTime;
  duration  = msg->body.imgEnd.endTime  - msg->body.imgEnd.expTime;

  duration += audine->shutter.getDelay();

  t -= STATIC_CAST(time_t,duration/1000); // PC's timestamp at exposure start
  strftime (ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", gmtime(&t));

  audine->fits.set("DATE-OBS", ts, "start date & time");
  audine->fits.set("EXPTIME",  exptime/1000,  "[s] exposure time");
  audine->fits.set("READTIME", readtime/1000, "[s] readout time");
  audine->fits.set("DARKTIME", duration/1000, "[s] dark current time");
}

/*---------------------------------------------------------------------------*/


void
ImageSequencer::computeTimeouts()
{
  int Kc,  NR, h, w, adc;
  double Tclear, Texp, Tread, NClear;
  

  audine->chip.getMaxDim(&Kc, &NR);
  Kc = Kc/Audine::SEQ_KCOL +1;

  NClear = audine->ccdClean->getValue("NUMBER");
  adc = audine->chip.getADC();

  Tclear = NClear*NR*(timeParam[adc].Tv+Kc*timeParam[adc].Tsk/4);
  log->debug(IFUN,"Tclear = %g\n",Tclear);


  // adds a 20% in the total estimate
  // and assures a minimun estimate of 3 seconds

  Texp = Tclear +  audine->shutter.getDelay() 
    +  1000*expLimits->getValue("EXPTIME");

  log->debug(IFUN,"Texp antes = %g\n",Texp);

  Texp *= 1.2;
  Texp = (Texp > 3000 ) ? Texp : 3000 ;

  log->debug(IFUN,"Texp despues = %g\n",Texp);

  // adds a 20% in the total estimate
  // and assures a minimun estimate of 3 seconds

  audine->chip.getDim(&w, &h);
  
  Tread = timeParam[adc].Th*h*w + timeParam[adc].K1*h;
  log->debug(IFUN,"Tread antes = %g\n",Tread);
  Tread *= 1.2;
  Tread = (Tread > 3000 ) ? Tread : 3000 ;
  log->debug(IFUN,"Tread despues = %g\n",Tread);

  expTimeout  = STATIC_CAST(int, Texp);
  readTimeout = STATIC_CAST(int, Tread);
  log->debug(IFUN,"expTimeout = %d\n",expTimeout);
  log->debug(IFUN,"readTimeout = %d\n",readTimeout);
}

/*---------------------------------------------------------------------------*/
