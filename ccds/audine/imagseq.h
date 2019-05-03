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

#ifndef AUDINE_IMAGSEQ_H
#define AUDINE_IMAGSEQ_H


/* estimated paremeters to predict timeouts 
 * these parameters don't depend on CCD model but on:
 *  - the ADC speed 
 *  - the target COR C implementation 
 */ 

struct Timeout {
  double Tv;			/* vertical timing */
  double Tsk;			/* horizontal timing when skipping pixels */
  double Th;			/* horizontal timing when digitizing */
  double K1;			/* row constant */
};


/*
 * The Audine image sequencer
 */


class ImageSequencer  {

  friend class AudineAlert;
  friend class AudineOk;
  
 public:

  static const unsigned int TICK = 500;	/* ticks period in milliseconds */

  ImageSequencer(Audine* aud);
  ~ImageSequencer();

  /* image sequencer initialization */
  void init();

  /*************************/
  /* user interface events */
  /*************************/

  /* action when EXP_LIMITS numbers are set */
  void updateExpLimits(char* name[], double number[], int n);

  /* action when the EXPOSURE button is pressed */
  void updateExposure(char* name, ISState swit);

  /***************************/
  /* 'COR data comimg' event */
  /***************************/

  void handle(unsigned int event, const void* data, int len);

  /**********************************/
  /* other auxiliary methods needed */
  /**********************************/

  /* restarts sequencer state from Alarm state to Ok state */
  void restart();

  /* starts the image sequencer from the Audine wait state */
  void startFromWait();

  /* another round when the number of counts is > 0 */
  void restartFromWait();

  /* starts the image sequencer from the Audine exposure state */
  void startFromExp();

  /* another round when the number of counts is > 0 */
  void restartFromExp();

  /* cancels the image sequencer from the Audine wait state */
  void cancelFromWait(bool userReq);

  /* cancels the image sequencer from the Audine exposure state */
  void cancelFromExp(bool userReq);

  /* stops tick timer */
  void stopTickTimer() { timer->stop(); }

  /* stops timeout timer */
  void stopTimeoutTimer() { alarm->cancel(); }

  /* decrements counters in EXP_COUNTERS property */
  void decDelay();
  void decExptime();
  void decCount();

  /* used by state transition engine */
  bool hasDelay() {return (expLimits->getValue("DELAY") > 0.0); }

  /* used by state transition engine */
  int getCount()  {return (STATIC_CAST(int,expCounters->getValue("COUNT"))); }

  /* used by the storage manager */
  int getSequenceSize()  {return (STATIC_CAST(int,expLimits->getValue("COUNT"))); }

  /* updates Audine image start message */
  void updateMessage();

  /* inits the Rx process. width and height are the expected image dimens. */
  void initRx(int width, int height);

  /* handles UDP image message from COR */
  void handle(const void* data, int len);

  /* handles UDP final image message from COR */
  void handleFinal(const void* data, int len);

  /* estimate exposure and readout timeouts given several factors */
  void computeTimeouts();

  /* zeroes exposure counters when BIAS is selected */
  void updateImageType(const char* imageType);

 private:
  
  Log* log;

  /*********************************/
  /* THE USER INTERFACE PROPERTIES */
  /*********************************/

  SwitchPropertyVector* exposure; /* exposure start/stop */

  NumberPropertyVector* expCounters;
  NumberPropertyVector* expLimits;

  /********************/
  /* other attributes */
  /********************/

  Audine* audine;
  int byteCounter;		/* runninig counter of bytes received */
  int imageSize;		/* image size in bytes */

  int expTimeout;	 /* estimated exposure timeout in milliseconds */
  int readTimeout;	  /* estimated readout timeout in milliseconds */

  PluginTimer* timer;
  PluginAlarm* alarm;
};

#endif
