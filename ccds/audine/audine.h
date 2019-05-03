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

#ifndef AUDINE_H
#define AUDINE_H

typedef short pixel_t; /* Audine pixel's data type is signed, 16 bit */

#include <indicor/api.h>

#include "fitshead.h"
#include "chip.h"
#include "shutter.h"
#include "imagseq.h"
#include "storage.h"

/*******************************/
/* THE PLUGIN FACTORY FUNCTION */
/*******************************/

BEGIN_C_DECLS

Plugin* CreatePlugin(Device*,  const char* args);

END_C_DECLS


class AudineState;		/* forward reference */

class Audine : public PluginBase
{

 public:
  
  /* image type strings */

  static const char OBJECT[];
  static const char DARK[];
  static const char FLAT[];
  static const char BIAS[];
  static const char FOCUS[];

  static const unsigned int SEQ_KCOL = 5; /* PROVISIONAL */

 protected:

  friend class AudineState;	/* base class */
  friend class AudineIdle;	/* when COR is not connected */
  friend class AudineOk;	/* when COR is Ok */
  friend class AudineWait;	/* busy, waiting between exposures */
  friend class AudineExp;	/* busy, performing an exposure */
  friend class AudineRead;	/* busy, reading out CCD */
  friend class AudineAlert;	/* something wrong happened */

  friend class CCDChip;		/* Audine part */
  friend class Shutter;		/* Audine part */
  friend class ImageSequencer;	/* Audine part */
  friend class Storage;		/* Audine part */

 public:


  Audine(Device* dev, unsigned char perif);
  virtual ~Audine() {}

  
  /* ****************** */
  /* THE INDI INTERFACE */
  /* ****************** */

  virtual void init();		/* initialization is not delegated to states */

#if 0				/* not yet implemented */
  virtual void 
    update(BLOBPropertyVector* pv, char* name[], char* blob[], int n);
#endif

  virtual void 
    update(TextPropertyVector* pv, char* name[], char* text[], int n);
  
  virtual void 
    update(SwitchPropertyVector* pv, char* name, ISState swit);
  
  virtual void 
    update(NumberPropertyVector* pv, char* name[], double num[], int n);


  /*************************/
  /* The UDP event handler */
  /*************************/

  virtual void handle(unsigned int event, const void* data, int len);


  /*****************************************************/
  /* events comming from other Plugin (observables) */
  /*****************************************************/
     
  virtual void update(PropertyVector* v, ITopic topic);


  /*****************************************/
  /* AUDINE CONTROL TIMER DRIVEN FUNCTIONS */
  /*****************************************/

  /* seconds tick counter */
  void tick();

  /* safety watch dog for exposure */
  void timeout(); 

 protected:

  void nextState(AudineState* st);  

  /*************************/
  /* AUDINE HELPER METHODS */
  /*************************/

  /* these three will dissapear */

  void 
    dummy(TextPropertyVector* pv, char* name[], char* text[], int n);
  
  void 
    dummy(NumberPropertyVector* pv, char* name[], double num[], int n);

  void 
    dummy(SwitchPropertyVector* pv, char* name, ISState swit);
  

  /* Save current device configuration to XML file */
  /* method independent of state */
  void doSave(char* name, ISState swit);

  /* action when a connect event comes from COR */
  void doConnected();

  /* action when CCD_CLEAN number is updated */
  void doCCDClean(char* name[], double num[], int n);

  /* action to do when PATTERN swicth is updated */
  void doPattern(char* name, ISState swit);

  /* action to do when FITS_TEXT_DATA property is updated */
  void updateFITSTextData(char* name[], char* text[], int n);
  
  /* action when IMAGETYP swicth is selected */
  void updateImageType(char* name, ISState swit);

  /* action when WCS_SEED are updated */
  void updateWCSSeed(char* name[], double number[], int n);

  /* send image request message */
  void sendImageMsg();

  /* send image request message */
  void sendCancelImageMsg();

  /* returns image type string to be used elsewhere (ie. FITS) */
  const char* getImageType() { return(imageType); }

  /* updates Image Type FITS keyword when its switch property changes */
  void updateImageType();

  /* updates WCS seed FITS keywords when user values or image type changes */
  void updateWCSFITS();

  /* updates more FITS keywords coming from telescope info */
  bool updateFromTelescope(PropertyVector* pvorig);

  /* updates more FITS keywords coming from firmware info */
  bool updateFromFirmware(PropertyVector* pvorig);

  /*********************************/
  /* THE USER INTERFACE PROPERTIES */
  /*********************************/


  LightPropertyVector*  ccdStatus; /* Audine visible status (lights) */

  SwitchPropertyVector* pattern; /* COR's test pattern generation */
  SwitchPropertyVector* configuration;
  SwitchPropertyVector* imagetyp; /* image type to take */

  NumberPropertyVector* ccdClean;
  NumberPropertyVector* wcsSeed; /* image scale and rotation  */

  TextPropertyVector*   fitsTextData;


  /****************************/
  /* other private attributes */
  /****************************/

  unsigned char perifNum;	/* either PERIF_CCD_PPAL or PERIF_CCD_GUIDE */
  Outgoing_Message req;	   /* COR image request message to be built */
  bool firstComment;	   /* flag to manage user 'COMMENT' keyword */
  const char* imageType;	/* the IMAGETYP FITS keywords value */


  /*********************/
  /* contained objects */
  /*********************/

  CCDChip chip;		 /* The CCD chip handling all geometry req. */
  Shutter shutter;		/* Audine shutter logic */
  ImageSequencer imgseq;	/* Image sequencer logic */
  Storage storage;		/* FITS file storage manager */
  FITSHeader fits;		/* FITS header for this camera */

  /* ******************************** */
  /* Other relationships with objects */
  /* ******************************** */
  
  AudineState* curState;	/* Camera current State */

  /* refererences to foreign properties. Learnt by notifications
     and used to update FITS keywords at the proper time */

  TextPropertyVector* object;	/* from telecope */
  TextPropertyVector* telesData; /* from telescope */
  TextPropertyVector* mountData; /* from telescope */

  NumberPropertyVector* eqCoords; /* from telescope */
  NumberPropertyVector* optics;	/* from telecope */
  

};

#ifndef AUDINE_STATE_H
#include "state.h"
#endif

/*---------------------------------------------------------------------------*/

inline void 
Audine::nextState(AudineState* st) 
{

  device->formatMsg("Cambiando estado de %s a %s",
		    curState->getName(), st->getName());
  device->indiMessage();
  curState = st;
}

/*---------------------------------------------------------------------------*/

inline void
Audine::tick()
{
  curState->tick(this);
}

/*---------------------------------------------------------------------------*/

inline void
Audine::timeout()
{
  curState->timeout(this);
}

/*---------------------------------------------------------------------------*/

inline void 
Audine::handle(unsigned int event, const void* data, int len) 
{
  curState->handle(this, event, data, len);
}

/*---------------------------------------------------------------------------*/

inline void     
Audine::update(PropertyVector* pv, ITopic topic) 
{
  curState->update(this, pv, topic);
}

/*---------------------------------------------------------------------------*/

#if 0				/* not yet implemented */
inline void 
Audine::update(BLOBPropertyVector* pv, char* name[], char* blob[], int n) 
{
  curState->update(this, pv, name, blob, n);
  pv->indiSetProperty();
}
#endif

/*---------------------------------------------------------------------------*/

inline void 
Audine::update(TextPropertyVector* pv, char* name[], char* text[], int n) 
{
  curState->update(this, pv, name, text, n);
  pv->indiSetProperty();
}
  
/*---------------------------------------------------------------------------*/

inline void 
Audine::update(SwitchPropertyVector* pv, char* name, ISState swit) 
{
  curState->update(this, pv, name, swit);
  pv->indiSetProperty();
}
  
/*---------------------------------------------------------------------------*/

inline void 
Audine::update(NumberPropertyVector* pv, char* name[], double num[], int n) 
{
  curState->update(this, pv, name, num, n);
  pv->indiSetProperty();
}

/*---------------------------------------------------------------------------*/



#endif
