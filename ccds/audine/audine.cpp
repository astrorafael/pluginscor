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
#include <memory.h>
#include <errno.h>
#include <stdlib.h>

#include "audine.h"


/*---------------------------------------------------------------------------*/
/* SHARED LIBRARY ENTRY POINT */

Plugin* CreatePlugin(Device* device, const char* args)
{
  int perif;

  if((perif = cor_str2ccd(args)) == -1) {
    return(0);
  }

  return (new Audine(device, perif));
}

/*---------------------------------------------------------------------------*/


const char Audine::OBJECT[] = "object";
const char Audine::DARK[]   = "dark";
const char Audine::FLAT[]   = "flat";
const char Audine::BIAS[]   = "zero";
const char Audine::FOCUS[]  = "focus";

/*---------------------------------------------------------------------------*/

Audine::Audine(Device* dev, unsigned char perif) :
  PluginBase(dev, "Audine"), perifNum(perif), chip(this), shutter(this),
  imgseq(this), storage(this) 
{
  object = 0;
  eqCoords = 0;
  telesData = 0;
  mountData = 0;
  optics = 0;
  firstComment = true;
}

/*---------------------------------------------------------------------------*/

void 
Audine::dummy(SwitchPropertyVector* pv, char* name, ISState swit)
{

  pv->forceChange();
  pv->formatMsg("dummy() called for %s[%s]",pv->getName(), name);
  pv->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
Audine::dummy(NumberPropertyVector* pv, char* name[], double number[], int n)
{
  pv->forceChange();
  pv->formatMsg("dummy() called for %s, len=%d",pv->getName(),n);
  pv->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
Audine::dummy(TextPropertyVector* pv, char* name[], char* text[], int n)
{
  pv->forceChange();
  pv->formatMsg("dummy() called for %s, len=%d",pv->getName(),n);
  pv->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
Audine::init()
{
  
  log->info(IFUN,"%s initialization\n",device->getName());

  /************************/
  /* resetable properties */
  /************************/


  pattern = DYNAMIC_CAST(SwitchPropertyVector*, device->find("PATTERN"));
  assert(pattern != NULL);
  pattern->on("OFF");

  configuration  = DYNAMIC_CAST(SwitchPropertyVector*, device->find("CONFIGURATION"));
  assert(configuration != NULL);
  configuration->off();

  ccdStatus  = DYNAMIC_CAST(LightPropertyVector*, device->find("CCD_STATUS"));
  assert(ccdStatus != NULL);
  ccdStatus->idle(); ccdStatus->ok("IDLE");

  imagetyp = DYNAMIC_CAST(SwitchPropertyVector*, device->find("IMAGETYP"));
  assert(imagetyp != NULL);
  imagetyp->on("FOCUS");

  /****************************/
  /* non resetable properties */
  /****************************/



  wcsSeed  = DYNAMIC_CAST(NumberPropertyVector*, device->find("WCS_SEED"));
  assert(wcsSeed != NULL);

  ccdClean = DYNAMIC_CAST(NumberPropertyVector*, device->find("CCD_CLEAN"));
  assert(ccdClean != NULL);


  fitsTextData = DYNAMIC_CAST(TextPropertyVector*, device->find("FITS_TEXT_DATA"));
  assert(fitsTextData != NULL);


  /************************/
  /* final initialization */
  /************************/

  updateImageType();
  updateWCSFITS();

  chip.init();
  shutter.init();
  imgseq.init();
  storage.init();

  /* THIS HAS TO DISSAPEAR. WE CANNOT ASUME ALL PROPERTIES ARE IDLE */
  /* IN CCD CHIP PROPERTY STATE IS USED TO ENABLE/DISABLE USER OPERATION */
  /* EVEN IN IDLE STATUS */

  device->idleStatus();

  curState = AudineIdle::instance();

  req.header.peripheal = perifNum;
  req.header.origin    = ORIGIN_PC;
  req.body.imageReq.nClear = STATIC_CAST(u_char, ccdClean->getValue("NUMBER"));

  demux->add(this, perifNum);	// image data
  demux->add(this, perifNum+1);	// end of image data and timestamps

  log->debug(IFUN,"perifNum = 0x%x\n",perifNum);
}

/*---------------------------------------------------------------------------*/

void
Audine::doSave(char* name, ISState swit)
{
  if(swit == ISS_ON) 
    device->xmlSave(); 
  configuration->off("SAVE");
}

/*---------------------------------------------------------------------------*/

void
Audine::doCCDClean(char* name[], double number[], int n)
{
  assert (n == 1);
  ccdClean->setValue(name[0], number[0]);
  req.body.imageReq.nClear = STATIC_CAST(u_char, number[0]);
  ccdClean->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
Audine::doPattern(char* name, ISState swit)
{
  pattern->setValue(name, swit);
  pattern->indiSetProperty();

  if(pattern->getValue("ON")) {

    if(req.body.imageReq.binning < 10) 
      req.body.imageReq.binning += 10;

  } else {

    if(req.body.imageReq.binning > 10) 
      req.body.imageReq.binning -= 10;
  }
}

/*---------------------------------------------------------------------------*/

void 
Audine::sendImageMsg()
{
  
  req.body.imageReq.cancel = 0;

  log->debug(IFUN,"req.body.imageReq.cols       = %d\n",req.body.imageReq.cols);
  log->debug(IFUN,"req.body.imageReq.rows       = %d\n",req.body.imageReq.rows);
  log->debug(IFUN,"req.body.imageReq.binning    = %d\n",req.body.imageReq.binning);
  log->debug(IFUN,"req.body.imageReq.tMsecExp   = %d\n",req.body.imageReq.tMsecExp);
  log->debug(IFUN,"req.body.imageReq.tExpSec    = %d\n",req.body.imageReq.tSecExp);
  log->debug(IFUN,"req.body.imageReq.x1         = %d\n",req.body.imageReq.x1);
  log->debug(IFUN,"req.body.imageReq.y1         = %d\n",req.body.imageReq.y1);
  log->debug(IFUN,"req.body.imageReq.x2         = %d\n",req.body.imageReq.x2);
  log->debug(IFUN,"req.body.imageReq.y2         = %d\n",req.body.imageReq.y2);
  log->debug(IFUN,"req.body.imageReq.cancel     = %d\n",req.body.imageReq.cancel);
  log->debug(IFUN,"req.body.imageReq.nClear     = %d\n",req.body.imageReq.nClear);
  log->debug(IFUN,"req.body.imageReq.shutterMode= %d\n",req.body.imageReq.shutterMode);
  log->debug(IFUN,"req.body.imageReq.delay      = %d\n",req.body.imageReq.delay);
  log->debug(IFUN,"req.body.imageReq.readSeqLen = %d\n",req.body.imageReq.readSeqLen);
  log->debug(IFUN,"req.body.imageReq.clearSeqLen= %d\n",req.body.imageReq.clearSeqLen);
  log->debug(IFUN,"req.body.imageReq.VSeqLen    = %d\n",req.body.imageReq.VSeqLen);


  mux->sendMessage(&req, MSG_LEN(Image_Req_Msg));
 
}

/*---------------------------------------------------------------------------*/

void 
Audine::sendCancelImageMsg()
{
  req.body.imageReq.cancel = 1;
  log->debug(IFUN,"req.body.imageReq.cancel     = %d\n",req.body.imageReq.cancel);

  mux->sendMessage(&req, MSG_LEN(Image_Req_Msg));
}

/*---------------------------------------------------------------------------*/

void
Audine::updateFITSTextData(char* name[], char* text[], int n)
{
  const char* comm;

  for(int i=0; i<n; i++)
    fitsTextData->setValue(name[i], text[i]);
 
  fitsTextData->indiSetProperty();

  comm   = fitsTextData->getValue("COMMENT");

  // This logic preserves the original COMMENT keywords
  // with the author credits

  // if the comment content is not empty then
  // creates a new user comment if none has been created
  // else replaces the existing user comments
  // if the comment contents is empty and a user comment
  // had been created, delete it

  if(strlen(comm) != 0) {	// creating a comment

    if(firstComment) {
      fits.appendVoid("COMMENT", comm);
      firstComment = false;
    }
    else {
      fits.setVoid("COMMENT",comm);
    }

  } else if(!firstComment) { 			// deleting a comment

      fits.erase("COMMENT");
      firstComment = true;
  }

}

/*---------------------------------------------------------------------------*/

void 
Audine::updateWCSFITS()
{
  /* signs of CDELT1, CDELT2 could be affected by image orientation */

  double scale =  wcsSeed->getValue("SCALE")/3600.0;

  if(imagetyp->getValue("OBJECT") && scale != 0.0) {
    fits.set("CDELT1", scale);
    fits.set("CDELT2", scale);
    fits.set("CROTA2", wcsSeed->getValue("ROTA"));
  } else {
    fits.erase("CDELT1");
    fits.erase("CDELT2");
    fits.erase("CROTA2");
  }
}

/*---------------------------------------------------------------------------*/

void
Audine::updateWCSSeed(char* name[], double number[], int n)
{
  for(int i=0; i<n; i++)
    wcsSeed->setValue(name[i], number[i]);
 
  wcsSeed->indiSetProperty();
  updateWCSFITS();
}

/*---------------------------------------------------------------------------*/

void
Audine::updateImageType()
{

  const char* irafImageType;

  if(imagetyp->getValue("BIAS")) {
    imageType = Audine::BIAS;
  } else if(imagetyp->getValue("DARK")) {
    imageType = Audine::DARK;
  } else if(imagetyp->getValue("FLAT")) {
    imageType = Audine::FLAT;
  } else if(imagetyp->getValue("FOCUS")) {
    imageType = Audine::FOCUS;
  } else if(imagetyp->getValue("OBJECT")) {
    imageType = Audine::OBJECT;
  } else {
    assert("Unknown image type" == 0);
  }


  // writes IMAGETYP KEYWORD

  if(imageType == Audine::FOCUS)
    irafImageType = Audine::OBJECT;
  else
    irafImageType = imageType;

  fits.set("IMAGETYP", irafImageType, "IRAF image type");

  // writes RA, DEC, OBJECT keywords

  if(imageType == Audine::OBJECT) {

    if(object) {
      fits.set("OBJECT", object->getValue("NAME"));
    }

    if(eqCoords) {
      char buf[16+1];
      fs_sexa(buf,eqCoords->getValue("RA"),2,3600);
      fits.set("RA", buf, "aproximate center");
      fs_sexa(buf,eqCoords->getValue("DEC"),2,3600);
      fits.set("DEC",buf, "aproximate center");
    }

    if(optics) {
      fits.set("DIAMETER", optics->getValue("DIAMETER")/100.0, "[m]");
      fits.set("FOCALLEN", optics->getValue("FOCALLEN")/100.0, "[m]");
    }

    if(telesData) {
      fits.set("OBSERVER", telesData->getValue("OBSERVER"));
      fits.set("LOCATION", telesData->getValue("LOCATION"));
    }
    
    if(mountData) {
      fits.set("TELESCOP", mountData->getValue("MODEL"));
    }


  } else {

    fits.erase("OBJECT");
    fits.erase("RA");
    fits.erase("DEC");
    fits.erase("DIAMETER");
    fits.erase("FOCALLEN");
    fits.erase("OBSERVER");
    fits.erase("LOCATION");
    fits.erase("TELESCOP");

  }
}

/*---------------------------------------------------------------------------*/

void 
Audine::updateImageType(char* name, ISState swit)
{
  imagetyp->setValue(name, swit);
  imagetyp->indiSetProperty();
  updateImageType();
  updateWCSFITS();
  shutter.update();
  imgseq.updateImageType(imageType);

}

/*---------------------------------------------------------------------------*/

bool
Audine::updateFromFirmware(PropertyVector* pvorig)
{
  char buffer[68+1];

  if(!pvorig->equals("FIRMWARE"))
    return(false);
  
  TextPropertyVector* firmware = DYNAMIC_CAST(TextPropertyVector*, pvorig);

  snprintf(buffer, sizeof buffer, "%s %s %s",
	   firmware->getValue("PROGRAM"),
	   firmware->getValue("DATE"),
	   firmware->getValue("TIME"));
  fits.set("FIRMWARE", buffer, "COR firmware version");
  return(true);
}

/*---------------------------------------------------------------------------*/

bool
Audine::updateFromTelescope(PropertyVector* pvorig)
{

  if(pvorig->equals("TARGET")) {
    object = DYNAMIC_CAST(TextPropertyVector*, pvorig);
    if(imageType == Audine::OBJECT) {
      const char* name =  object->getValue("NAME");
      fits.set("OBJECT", name);
      storage.updatePrefix(name);
    }
    return(true);
  }

  if(pvorig->equals("EQUATORIAL_COORD")) {
    eqCoords = DYNAMIC_CAST(NumberPropertyVector*, pvorig);
    if(imageType == Audine::OBJECT) {
      char buf[16+1];
      fs_sexa(buf,eqCoords->getValue("RA"),3,3600);
      fits.set("OBJCTRA", buf);
      fs_sexa(buf,eqCoords->getValue("DEC"),3,3600);
      fits.set("OBJCTDEC",buf);
    }
    return(true);
  }

  if(pvorig->equals("OPTICS")) {
    optics = DYNAMIC_CAST(NumberPropertyVector*, pvorig);
    if(imageType == Audine::OBJECT) {
      fits.set("DIAMETER", optics->getValue("DIAMETER")/100.0, "[m]");
      fits.set("FOCALLEN", optics->getValue("FOCALLEN")/100.0, "[m]");
    }
    return(true);
  }

  if(pvorig->equals("FITS_TEXT_DATA")) {
    telesData = DYNAMIC_CAST(TextPropertyVector*, pvorig);
    if(imageType == Audine::OBJECT) {
      fits.set("OBSERVER", telesData->getValue("OBSERVER"));
      fits.set("LOCATION", telesData->getValue("LOCATION"));
      // No incluimos esta keyword porque la cogemos de la property MOUNT
      // Esto lo deberia arreglar porque otros drivers no implementarian
      // la property mount y la FITS key. TELESCO se queda sin asignar
      // Por otra parte, si lo dejo, machacaria lo que dice MOUNT.MODEL

      //fits.set("TELESCOP", telesData->getValue("TELESCOP"));
    }
    return(true);
  }

  if(pvorig->equals("MOUNT")) {
    mountData = DYNAMIC_CAST(TextPropertyVector*, pvorig);
    if(imageType == Audine::OBJECT) {
      fits.set("TELESCOP", mountData->getValue("MODEL"), "Mount/software model");
    }
    return(true);
  }



  return(false);
}

/*---------------------------------------------------------------------------*/



