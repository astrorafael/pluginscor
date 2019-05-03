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


/* Xilinx sequence for binning 1x1, slow ADC */
static
const u_char seq_1x1s[] = {
  0x09,0x09,0x0A,0x08,0x08,0x08,0x08,
  0x0C,0x1C,0x28,0x38,0x48,0x58,0x68,0x79,0x09,0x09,0x09,
  0x09,0x09,0x09,0x01,0x89,0x09
};

/* Xilinx sequence for binning 2x2, slow ADC */

static
const u_char seq_2x2s[] = {
  0x09,0x09,0x0A,0x08,0x08,0x08,0x08,
  0x0C,0x1C,0x28,0x38,0x48,0x58,0x69,0x78,0x09,0x09,0x09,
  0x09,0x09,0x09,0x01,0x89,0x09
};

/* Xilinx sequence for binning 3x3, slow ADC */

static
const u_char seq_3x3s[]= {
  0x09,0x09,0x0A,0x08,0x08,0x08,0x08,
  0x0C,0x1C,0x28,0x38,0x48,0x58,0x69,0x78,0x09,0x08,0x09,
  0x09,0x09,0x09,0x01,0x89,0x09
};

/* Xilinx sequence for binning 4x4, slow ADC */

static
const u_char seq_4x4s[]= {
  0x09,0x09,0x0A,0x08,0x08,0x08,0x08,
  0x0C,0x1C,0x28,0x38,0x49,0x58,0x69,0x78,0x09,0x08,0x09,
  0x09,0x09,0x09,0x01,0x89,0x09
};


/* byte sequence to clear the CCD */
static 
const u_char seq_clear[]=
{
  0x09,0x09,0x0A,0x08,0x09,0x08,0x0A,0x08,
  0x09,0x09,0x0A,0x08,0x09,0x09,0x0A,0x08,
  0x09,0x09,0x0A,0x08,0x89
};


/* vertical phase shift sequence, 5us each pulse, 20us total */
static 
const u_char seq_V5us[]= {
  0x01, 0x02, 0x01,0x00
};

/* vertical phase shift sequence, 10us each pulse, 40us total */
static 
const u_char seq_V10us[]= {
  0x01, 0x01, 0x02, 0x02, 0x01,0x01, 0x00, 0x00
};


/* Vertical register pulse width for various chip models:
 *
 * 400C, 400L = 4us.
 * 401L = 5us.
 * 402ME = 2us
 * 1600, 1602E, 1603ME = 5us.
 * 3000CE = 10us
 * 3200ME = 5us
 *
 * 'corplus' firmware in COR generates a minimun pulse width of 5us.
 */

static const 
CCDData ccdData[] = {
  // KAF400, 401 & 402
  {
    {14, 4},			// overscan point 1
    { 768,  512},		// active width
    {14, 4},			// overscan point 2
    9.0,			// pixel size [um]
    10,				// shift register dummy pixels
    {seq_clear, sizeof(seq_clear)}, // clearing sequence
    {seq_V5us,  sizeof(seq_V5us)}, // vertical sequence
    {
      {{seq_1x1s, sizeof(seq_1x1s)},{seq_1x1s, sizeof(seq_1x1s)}}, // bin 1x1
      {{seq_2x2s, sizeof(seq_2x2s)},{seq_2x2s, sizeof(seq_2x2s)}}, // bin 2x2
      {{seq_3x3s, sizeof(seq_3x3s)},{seq_3x3s, sizeof(seq_3x3s)}}, // bin 3x3
      {{seq_4x4s, sizeof(seq_4x4s)},{seq_4x4s, sizeof(seq_4x4s)}}  // bin 4x4
    }
  },
  
  // KAF 1600, 1602 & 1603ME
  {
    {14, 4},			// overscan point 1
    {1536, 1024},		// active width and height
    {14, 4},			// overscan point 2
    9.0,			// pixel size [um]
    10,				// shift register dummy pixels
    {seq_clear, sizeof(seq_clear)}, // clearing sequence
    {seq_V5us,  sizeof(seq_V5us)}, // vertical sequence
    {
      {{seq_1x1s, sizeof(seq_1x1s)},{seq_1x1s, sizeof(seq_1x1s)}}, // bin 1x1
      {{seq_2x2s, sizeof(seq_2x2s)},{seq_2x2s, sizeof(seq_2x2s)}}, // bin 2x2
      {{seq_3x3s, sizeof(seq_3x3s)},{seq_3x3s, sizeof(seq_3x3s)}}, // bin 3x3
      {{seq_4x4s, sizeof(seq_4x4s)},{seq_4x4s, sizeof(seq_4x4s)}}  // bin 4x4
    }
  }, 

  // KAF3000CE
  {
    {44,17},			// overscan point 1
    {2016, 1512},		// active width and height
    {20,14},			// overscan point 2
    9.0,			// pixel size [um]
    10,				// shift register dummy pixels
    {seq_clear, sizeof(seq_clear)}, // clearing sequence
    {seq_V10us, sizeof(seq_V10us)}, // vertical sequence
    {
      {{seq_1x1s, sizeof(seq_1x1s)},{seq_1x1s, sizeof(seq_1x1s)}}, // bin 1x1
      {{seq_2x2s, sizeof(seq_2x2s)},{seq_2x2s, sizeof(seq_2x2s)}}, // bin 2x2
      {{seq_3x3s, sizeof(seq_3x3s)},{seq_3x3s, sizeof(seq_3x3s)}}, // bin 3x3
      {{seq_4x4s, sizeof(seq_4x4s)},{seq_4x4s, sizeof(seq_4x4s)}}  // bin 4x4
    }
  }, 

  // KAF3200ME
  {
    {46,34},			// overscan point 1
    {2184, 1472},		// active width and height
    {37, 4},			// overscan point 2
    6.8 ,			// pixel size
    8,				// shift register dummy pixels
    {seq_clear, sizeof(seq_clear)}, // clearing sequence
    {seq_V10us, sizeof(seq_V10us)}, // vertical sequence
    {
      {{seq_1x1s, sizeof(seq_1x1s)},{seq_1x1s, sizeof(seq_1x1s)}}, // bin 1x1
      {{seq_2x2s, sizeof(seq_2x2s)},{seq_2x2s, sizeof(seq_2x2s)}}, // bin 2x2
      {{seq_3x3s, sizeof(seq_3x3s)},{seq_3x3s, sizeof(seq_3x3s)}}, // bin 3x3
      {{seq_4x4s, sizeof(seq_4x4s)},{seq_4x4s, sizeof(seq_4x4s)}}  // bin 4x4
    }
  } 
};

/*---------------------------------------------------------------------------*/


CCDChip::CCDChip(Audine* aud) :
  log(0), adcSpeed(0), areaSelection(0), areaPresets(0), binning(0), 
  ccdModel(0),
  areaDim(0), areaDimRect(0), areaPresetSize(0), ccdTemp(0), 
  audine(aud), bin(1), model(0), adc(0),  udpMsgs(0)
{
  log = LogFactory::instance()->forClass("CCDChip");
}

/*---------------------------------------------------------------------------*/

void
CCDChip::init()
{
  /************************/
  /* resetable properties */
  /************************/

  areaPresets  = DYNAMIC_CAST(SwitchPropertyVector*, audine->device->find("AREA_PRESETS"));  
  
  assert(areaPresets != NULL);

  /****************************/
  /* non resetable properties */
  /****************************/

  ccdModel  = DYNAMIC_CAST(SwitchPropertyVector*, audine->device->find("CHIP"));
  assert(ccdModel != NULL);

  adcSpeed  = DYNAMIC_CAST(SwitchPropertyVector*, audine->device->find("ADC_SPEED"));
  assert(adcSpeed != NULL);

  binning  = DYNAMIC_CAST(SwitchPropertyVector*, audine->device->find("BINNING"));
  assert(binning != NULL);

  areaSelection  = DYNAMIC_CAST(SwitchPropertyVector*, audine->device->find("AREA_SELECTION")); 

  assert(areaSelection != NULL);

  areaPresets  = DYNAMIC_CAST(SwitchPropertyVector*, audine->device->find("AREA_PRESETS"));  
  assert(areaPresets != NULL);

  areaPresetSize = DYNAMIC_CAST(NumberPropertyVector*, audine->device->find("AREA_PRESET_SIZE"));  

  assert(areaPresetSize != NULL);

  areaDim = DYNAMIC_CAST(NumberPropertyVector*, audine->device->find("AREA_DIM"));
  assert(areaDim != NULL);

  areaDimRect = DYNAMIC_CAST(NumberPropertyVector*, audine->device->find("AREA_DIM_RECT"));
  assert(areaDimRect != NULL);

  ccdTemp = DYNAMIC_CAST(NumberPropertyVector*, audine->device->find("CCD_TEMP"));
  assert(ccdTemp != NULL);

  photPars  = DYNAMIC_CAST(NumberPropertyVector*, audine->device->find("PHOT_PARS"));
  assert(photPars != NULL);

  /************************/
  /* final initialization */
  /************************/

  sync();

}


void
CCDChip::sync()
{

 /* syncs chip */

  if(ccdModel->getValue("KAF400"))
    model = CCD_KAF400;
  else if(ccdModel->getValue("KAF1600"))
    model = CCD_KAF1600;
  else if(ccdModel->getValue("KAF3000"))
    model = CCD_KAF3000;
  else if(ccdModel->getValue("KAF3200"))
    model = CCD_KAF3200;
  else
    model = 1;


  /* syncs ADC speed */

  if(adcSpeed->getValue("100KBS")) {
    adc = ADC_100KBS;
  } else if (adcSpeed->getValue("200KBS")) {
    adc = ADC_200KBS;
  } else {
    adc = ADC_100KBS;
  }

  /* syncs binning */

  if(binning->getValue("1X1")) {
    bin = 1;
  } else if(binning->getValue("2X2")) {
    bin = 2;
  } else if(binning->getValue("3X3")) {
    bin = 3;
  } else if(binning->getValue("4X4")) {
    bin = 4;
  } else {
    bin = 1;
  }

  /* syncs pixel size and maximun dimensions */

  areaDim->setValue("PIXSZ", bin*ccdData[model].pixSize);
  areaDim->setValue("DIMX", ccdData[model].active.x/bin);
  areaDim->setValue("DIMY", ccdData[model].active.y/bin);

  /* syncs related properties areaDimRect, areaSelection, etc */

  if( areaSelection->getValue("FULL_FRAME")) {

    setAreaDimFullFrame(bin);

  } else if (areaSelection->getValue("FULL_FRAME_OV")) {

    setAreaDimOverscan(bin);

  } else if (areaSelection->getValue("PRESETS")) {

    int width = STATIC_CAST(int, areaPresetSize->getValue("SIZE"));
    SwitchProperty* sw = areaPresets->getLastOn();
    assert (sw != 0);
    setAreaPresets(width, bin, sw->getName());
  }

  updateMessage();

  // sets FITS headers related to this module

  audine->fits.set("PIXSZ1",  bin*ccdData[model].pixSize,
			      "[um] equivalent pixel size");
  audine->fits.set("PIXSZ2",  bin*ccdData[model].pixSize,
			      "[um] equivalent pixel size");

  audine->fits.set("CCDBIN1", bin, "binned columns");
  audine->fits.set("CCDBIN2", bin, "binned rows");

  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s CG3 %s + COR",
	   audine->getDevice()->getName(), getModel());
  audine->fits.set("INSTRUME",buffer);

  // GAIN & RDNOISE included only if GAIN <> 0

  double gain    = photPars->getValue("GAIN");
  double rdnoise = photPars->getValue("RDNOISE")*bin*bin;

  if (gain != 0.0) {

    audine->fits.set("GAIN", gain, "[e-/ADU] CCD Gain");
    audine->fits.set("RDNOISE", rdnoise,
				"[e-] binning-dependant readout noise");
  }
}

/*---------------------------------------------------------------------------*/

void
CCDChip::updateMessage()
{

  /* prepares request when model, adc or binning change */

  audine->req.body.imageReq.cols = STATIC_CAST(int16, ccdData[model].over1.x +
    ccdData[model].active.x  + ccdData[model].over2.x);

  audine->req.body.imageReq.rows = STATIC_CAST(int16, ccdData[model].over1.y +
    ccdData[model].active.y  + ccdData[model].over2.y);

  audine->req.body.imageReq.binning = bin;
  if(audine->pattern->getValue("ON"))
    audine->req.body.imageReq.binning += 10;

  audine->req.body.imageReq.clearSeqLen = ccdData[model].clear.length;
  memcpy(audine->req.body.imageReq.clearSeq,
	 ccdData[model].clear.start, ccdData[model].clear.length);

  audine->req.body.imageReq.VSeqLen = ccdData[model].vert.length;
  memcpy(audine->req.body.imageReq.VSeq,
	 ccdData[model].vert.start, ccdData[model].vert.length);

  audine->req.body.imageReq.readSeqLen = 
    ccdData[model].read[bin-1][adc].length;


  memcpy(audine->req.body.imageReq.readSeq,
	 ccdData[model].read[bin-1][adc].start,
	 ccdData[model].read[bin-1][adc].length);

}

/*---------------------------------------------------------------------------*/

bool
CCDChip::updateTemp(PropertyVector* pvorig)
{

  log->verbose(IFUN,"observing property %s\n",pvorig->getName());

  if(!pvorig->equals("CCD_TEMP"))
    return(false);

  NumberProperty* orig;
  NumberPropertyVector* pv = DYNAMIC_CAST(NumberPropertyVector*, pvorig);
  double cold, hot, vpelt;
  
  orig = DYNAMIC_CAST(NumberProperty*, pv->find("COLD"));
  assert(orig != NULL);
  cold = orig->getValue();
  ccdTemp->setValue("COLD", cold);
  
  orig = DYNAMIC_CAST(NumberProperty*, pv->find("HOT"));
  assert(orig != NULL);
  hot = orig->getValue();
  ccdTemp->setValue("HOT", hot);
  
  orig = DYNAMIC_CAST(NumberProperty*, pv->find("VPELT"));
  assert(orig != NULL);
  vpelt = orig->getValue();
  ccdTemp->setValue("VPELT", vpelt);

  ccdTemp->indiSetProperty();
  
  audine->fits.set("CCD-TEMP", cold, "[C] cold finger temperature");
  audine->fits.set("HOT-TEMP",  hot, "[C] heatsink temperature");
  audine->fits.set("VPELT",   vpelt, "[V] peltier voltage");

  return(true);

}

/*---------------------------------------------------------------------------*/

int
CCDChip::outOfBounds(int x1, int y1, int x2, int y2)
{
  int result = OOB_OK;

  assert( (model == 0) || (model == 1) || (model == 2) || (model == 3));

  if(x1 < ccdData[model].over1.x)
    result |=  OOB_X;

  if(y1 < ccdData[model].over1.y)
    result |=  OOB_Y;
  
  if(x2 > ccdData[model].active.x + ccdData[model].over1.x)
    result |=  OOB_X;

  if(y2 > ccdData[model].active.y + ccdData[model].over1.y)
    result |=  OOB_Y;

  if(x2 < x1)
    result |=  OOB_X;

  if(y2 < y1)
    result |=  OOB_Y;

  return(result);
}

/*---------------------------------------------------------------------------*/

void
CCDChip::setAreaDimOverscan(int binning)
{
  int dimx, dimy, x2, y2;

  assert((binning == 1) || (binning == 2) || (binning == 3) || (binning == 4));
  assert( (model == 0) || (model == 1) || (model == 2) || (model == 3));

  x2 = ccdData[model].over1.x + ccdData[model].active.x + 
    ccdData[model].over2.x;

  y2 = ccdData[model].over1.y + ccdData[model].active.y + 
    ccdData[model].over2.y;

  dimx = x2 / binning;
  dimy = y2 / binning;

  areaDimRect->setValue("ORIGX",0);
  areaDimRect->setValue("ORIGY",0);
  areaDimRect->setValue("DIMX",  dimx);
  areaDimRect->setValue("DIMY", dimy);

  audine->fits.set("NAXIS1", dimx, "columns");
  audine->fits.set("NAXIS2", dimy, "rows" );

  audine->req.body.imageReq.x1 = 0;
  audine->req.body.imageReq.y1 = 0;
  audine->req.body.imageReq.x2 = STATIC_CAST(int16, x2);
  audine->req.body.imageReq.y2 = STATIC_CAST(int16, y2);

  areaDim->setValue("DIMX", dimx);
  areaDim->setValue("DIMY", dimy);

  // NOT NEEDED
  // udpMsgs = computeUDPPackets(0, 0, x2, y2, binning);

  computeBiasSection(binning);
  computeTrimSection(binning);


  // just in case clients don't handle property redefinition

  areaDim->indiSetProperty();
  areaDimRect->indiSetProperty();

  DYNAMIC_CAST(NumberProperty*, areaDimRect->find("DIMX"))->setMax(dimx);
  DYNAMIC_CAST(NumberProperty*, areaDimRect->find("DIMY"))->setMax(dimy);

  // redefines the property because changed limits

  areaDimRect->indiDelProperty();
  areaDimRect->indiDefProperty();
}

/*---------------------------------------------------------------------------*/

void
CCDChip::setAreaDimFullFrame(int binning)
{
  int x1, y1, x2, y2, origx, origy, dimx, dimy;
  int factor, rem;
  

  factor = binning * Audine::SEQ_KCOL;

  assert((binning == 1) || (binning == 2) || (binning == 3) || (binning == 4));
  assert( (model == 0) || (model == 1) || (model == 2) || (model == 3));

  x1 = ccdData[model].over1.x;
  rem = x1 % factor;
  x1 += (factor - rem) % factor;

  y1 = ccdData[model].over1.y;
  rem = y1 % binning;
  y1 += (binning - rem) % binning;

  x2 = ccdData[model].over1.x + ccdData[model].active.x;
  rem = x2 % binning;
  x2 -= rem;

  y2 = ccdData[model].over1.y + ccdData[model].active.y;
  rem = y2 % binning;
  y2 -= rem;

  dimx = (x2 - x1) / binning ;
  dimy = (y2 - y1) / binning ;

  origx = (x1 -  ccdData[model].over1.x)/binning;
  origy = (y1 -  ccdData[model].over1.y)/binning;

  audine->req.body.imageReq.x1 = STATIC_CAST(int16, x1);
  audine->req.body.imageReq.y1 = STATIC_CAST(int16, y1);
  audine->req.body.imageReq.x2 = STATIC_CAST(int16, x2);
  audine->req.body.imageReq.y2 = STATIC_CAST(int16, y2);;

  areaDimRect->setValue("ORIGX",origx);
  areaDimRect->setValue("ORIGY",origy);
  areaDimRect->setValue("DIMX",dimx);
  areaDimRect->setValue("DIMY",dimy);

  areaDim->setValue("DIMX", ccdData[model].active.x/binning);
  areaDim->setValue("DIMY", ccdData[model].active.y/binning);

  audine->fits.set("NAXIS1",  dimx);
  audine->fits.set("NAXIS2",  dimy);

  audine->fits.erase("BIASSEC");
  audine->fits.erase("TRIMSEC");

  // NOT NEEDED
  // udpMsgs = computeUDPPackets(x1,y1,x2,y2,binning);

  // just in case clients don't handle property redefinition

  areaDim->indiSetProperty();	
  areaDimRect->indiSetProperty();	

  DYNAMIC_CAST(NumberProperty*, areaDimRect->find("DIMX"))->setMax(dimx);
  DYNAMIC_CAST(NumberProperty*, areaDimRect->find("DIMY"))->setMax(dimy);

  // redefines the property

  areaDimRect->indiDelProperty();
  areaDimRect->indiDefProperty();
}

/*---------------------------------------------------------------------------*/

int 
CCDChip::setAreaDimRect(int xorig, int yorig, int width, int height, int binning)
{

  int x1, y1, x2, y2;
  int pixwidth, pixheight;
  int result;

  assert((binning == 1) || (binning == 2) || (binning == 3) || (binning == 4));
  assert( width != 0);
  assert( height != 0);

  x1 = ccdData[model].over1.x;
  x1 += xorig * binning;

  y1 = ccdData[model].over1.y;
  y1 += yorig * binning;

  pixwidth  = width  * binning;
  pixheight = height * binning;

  x2 = x1 + pixwidth;
  y2 = y1 + pixheight;

  log->debug(IFUN,"x1= %d\n", x1 );
  log->debug(IFUN,"y1= %d\n", y1 ); 
  log->debug(IFUN,"x2= %d\n", x2 );
  log->debug(IFUN,"y2= %d\n", y2 ); 

  // safety check
  result = outOfBounds(x1, y1, x2, y2);
  if(result != OOB_OK)
    return(result);


  areaDimRect->setValue("ORIGX", xorig);
  areaDimRect->setValue("ORIGY", yorig);
  areaDimRect->setValue("DIMX", width);
  areaDimRect->setValue("DIMY", height);

  audine->fits.set("NAXIS1", width, "columns");
  audine->fits.set("NAXIS2", height, "rows");

  audine->fits.erase("BIASSEC");
  audine->fits.erase("TRIMSEC");

  areaDim->setValue("DIMX", ccdData[model].active.x/binning);
  areaDim->setValue("DIMY", ccdData[model].active.y/binning);

  audine->req.body.imageReq.x1 = STATIC_CAST(int16, x1);
  audine->req.body.imageReq.y1 = STATIC_CAST(int16, y1);
  audine->req.body.imageReq.x2 = STATIC_CAST(int16, x2);
  audine->req.body.imageReq.y2 = STATIC_CAST(int16, y2);

  // NOT NEEDED
  // udpMsgs = computeUDPPackets(x1, y1, x2, y2, binning);

  areaDim->indiSetProperty();
  areaDimRect->indiSetProperty();

  return(result);
}

/*---------------------------------------------------------------------------*/

int 
CCDChip::setAreaDimCommon(int width, int binn, int x1, int y1, int x2, int y2)
{
  int result, origx, origy;

  // safety check

  log->debug(IFUN,"x1= %d\n", x1 );
  log->debug(IFUN,"y1= %d\n", y1 ); 
  log->debug(IFUN,"x2= %d\n", x2 );
  log->debug(IFUN,"y2= %d\n", y2 ); 

  result = outOfBounds(x1, y1, x2, y2);
  if(result != OOB_OK)
    return(result);

  origx = (x1 -  ccdData[model].over1.x)/binn;
  origy = (y1 -  ccdData[model].over1.y)/binn;

  areaPresetSize->setValue("SIZE", width); 
  areaDimRect->setValue("ORIGX",origx);
  areaDimRect->setValue("ORIGY",origy);
  areaDimRect->setValue("DIMX",width);
  areaDimRect->setValue("DIMY",width);

  audine->fits.set("NAXIS1", width);
  audine->fits.set("NAXIS2", width);

  audine->fits.erase("BIASSEC");
  audine->fits.erase("TRIMSEC");

  areaDim->setValue("DIMX", ccdData[model].active.x/binn);
  areaDim->setValue("DIMY", ccdData[model].active.y/binn);

  audine->req.body.imageReq.x1 = STATIC_CAST(int16, x1);
  audine->req.body.imageReq.y1 = STATIC_CAST(int16, y1);
  audine->req.body.imageReq.x2 = STATIC_CAST(int16, x2);
  audine->req.body.imageReq.y2 = STATIC_CAST(int16, y2);
  
  // NOT NEEDED
  // udpMsgs = computeUDPPackets(x1, y1, x2, y2, binn);

  return(result);
}

/*---------------------------------------------------------------------------*/

int 
CCDChip::setAreaDimVFastRect(int width, int binning)
{
  
  int x1, y1, x2, y2;
  int pixwidth, factor, rem;
  int result;

  pixwidth = width * binning;
  factor = Audine::SEQ_KCOL * binning;

  x1 = ccdData[model].over1.x;
  rem = x1 % factor;
  x1 += (factor - rem) % factor;

  y1 = ccdData[model].over1.y;
  rem = y1 % binning;
  y1 += (binning - rem) % binning;

  x2 = x1 + pixwidth;
  y2 = y1 + pixwidth;

  result = setAreaDimCommon(width, binning, x1, y1, x2, y2);
  return(result);

}

/*---------------------------------------------------------------------------*/

int
CCDChip::setAreaDimFastRect(int width, int binning)
{
  
  int x0, x1, y1, x2, y2, rem;
  int pixwidth, factor;
  int result;

  pixwidth = width  * binning;
  factor = Audine::SEQ_KCOL * binning;

  x0 = ccdData[model].over1.x;
  rem = x0 % factor;
  x0 += (factor - rem) % factor;

  x1 = ccdData[model].active.x - pixwidth;
  rem = x1 % x0;
  x1 -= rem;

  y1 = ccdData[model].over1.y;
  rem = y1 % binning;
  y1 += (binning - rem) % binning;

  x2 = x1 + pixwidth;
  y2 = y1 + pixwidth;

  result = setAreaDimCommon(width, binning, x1, y1, x2, y2);
  return(result);
}

/*---------------------------------------------------------------------------*/

int 
CCDChip::setAreaDimSlowRect(int width, int binning)
{
  
  int x1, y0, y1, x2, y2, rem;
  int pixwidth, factor;
  int result;

  pixwidth = width  * binning;
  factor = Audine::SEQ_KCOL * binning;

  x1 = ccdData[model].over1.x;
  rem = x1 % factor;
  x1 += (factor - rem) % factor;

  y0 = ccdData[model].over1.y;
  rem = y0 % binning;
  y0 += (binning - rem) % binning;

  y1 = ccdData[model].active.y - pixwidth;
  rem = y1 % y0;
  y1 -= rem; 
  
  x2 = x1 + pixwidth;
  y2 = y1 + pixwidth;

  result = setAreaDimCommon(width, binning, x1, y1, x2, y2);
  return(result);
}

/*---------------------------------------------------------------------------*/

int 
CCDChip::setAreaDimVSlowRect(int width, int binning)
{
  
  int x0, y0, x1, y1, x2, y2, rem;
  int pixwidth, factor;
  int result;

  pixwidth = width  * binning;
  factor = Audine::SEQ_KCOL * binning;

  x0 = ccdData[model].over1.x;
  rem = x0 % factor;
  x0 += (factor - rem) % factor;

  y0 = ccdData[model].over1.y;
  rem = y0 % binning;
  y0 += (binning - rem) % binning;

  x1 = ccdData[model].active.x - pixwidth;
  rem = x1 % x0;
  x1 -= rem;

  y1 = ccdData[model].active.y - pixwidth;
  rem = y1 % y0;
  y1 -= rem; 

  x2 = x1 + pixwidth;
  y2 = y1 + pixwidth;

  result = setAreaDimCommon(width, binning, x1, y1, x2, y2);
  return(result);
}

/*---------------------------------------------------------------------------*/

int 
CCDChip::setAreaDimCenterRect(int width, int binning)
{
  
  int x0, y0, x1, y1, x2, y2, rem;
  int pixwidth, factor;
  int result;

  pixwidth = width  * binning;
  factor = Audine::SEQ_KCOL * binning;

  x0 = ccdData[model].over1.x;
  rem = x0 % factor;
  x0 += (factor - rem) % factor;

  y0 = ccdData[model].over1.y;
  rem = y0 % binning;
  y0 += (binning - rem) % binning;

  x1 = (ccdData[model].active.x - pixwidth)/2;
  rem = x1 % x0;
  x1 -= rem;

  y1 = (ccdData[model].active.y - pixwidth)/2;
  rem = y1 % y0;
  y1 -= rem; 

  x2 = x1 + pixwidth;
  y2 = y1 + pixwidth;

  result = setAreaDimCommon(width, binning, x1, y1, x2, y2);
  return(result);
}

/*---------------------------------------------------------------------------*/

int
CCDChip::setAreaPresets(int width, int binning, const char* name)
{
  assert((binning == 1) || (binning == 2) || (binning == 3) || (binning == 4));
  assert(width != 0);
  assert( (model == 0) || (model == 1) || (model == 2) || (model == 3));
  
  int result = OOB_XY;

  if(!strcmp(name,"CORNER1")) {
    result = setAreaDimVFastRect(width, binning);
  } else if(!strcmp(name,"CORNER2")) {
    result = setAreaDimFastRect(width, binning);
  } else if(!strcmp(name,"CORNER3")) {
    result = setAreaDimSlowRect(width, binning);
  } else if(!strcmp(name,"CORNER4")) {
    result = setAreaDimVSlowRect(width, binning);
  } else if(!strcmp(name,"CENTER")) {
    result = setAreaDimCenterRect(width, binning);
  } else {
    assert("area preset is not CORNER1, CORNER2, CORNER3, CORNER4, CENTER" == 0);
  }

  if(result == OOB_OK) {
    areaDim->indiSetProperty();
    areaDimRect->indiSetProperty();
    areaPresetSize->indiSetProperty();
  }

  return(result);
}

/*---------------------------------------------------------------------------*/

void 
CCDChip::updateChip(char* name, ISState swit)
{
  ccdModel->setValue(name, swit);

  ccdModel->formatMsg("Chip set to %s",name);
  ccdModel->indiSetProperty();
  
  if(ccdModel->getValue("KAF400"))
    model = CCD_KAF400;
  else if(ccdModel->getValue("KAF1600"))
    model = CCD_KAF1600;
  else if(ccdModel->getValue("KAF3000"))
    model = CCD_KAF3000;
  else if(ccdModel->getValue("KAF3200"))
    model = CCD_KAF3200;
  else
    model = 1;

  updateMessage();

  SwitchProperty* sw = areaSelection->getLastOn();
  assert (sw != 0);

  setAreaSelection(sw->getName());

  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s CG3 %s + COR",
	   audine->getDevice()->getName(), getModel());
  audine->fits.set("INSTRUME",buffer);
}

/*---------------------------------------------------------------------------*/

int
CCDChip::setAreaSelection(const char* name)
{
  int result = OOB_OK;

  if( !strcmp(name,"FULL_FRAME")) {

    setAreaDimFullFrame(bin);
    areaDimRect->idleStatus();
    areaPresets->idleStatus();
    areaPresetSize->idleStatus();

  }  else if(!strcmp(name,"FULL_FRAME_OV")) {

    setAreaDimOverscan(bin);
    areaDimRect->idleStatus();
    areaPresets->idleStatus();
    areaPresetSize->idleStatus();

  } else if (!strcmp(name,"USER_DEFINED")) {
  
    areaDimRect->okStatus();
    areaPresets->idleStatus();
    areaPresetSize->idleStatus();
    audine->fits.erase("BIASSEC");
    audine->fits.erase("TRIMSEC");

  } else if (!strcmp(name,"PRESETS")) {

    int width = STATIC_CAST(int, areaPresetSize->getValue("SIZE"));

    SwitchProperty* sw = areaPresets->getLastOn();
    assert (sw != 0);

    result = setAreaPresets(width, bin, sw->getName());
    if(result == OOB_OK) {
      areaDimRect->idleStatus();
      areaPresets->okStatus();
      areaPresetSize->okStatus();
    }
  
  } else {
    assert("Area selection is not PRESETS, USER_DEFINED, FULL_FRAME, FULL_FRAME_OV" == 0);
  }

  areaDim->setValue("PIXSZ", bin*ccdData[model].pixSize);
  areaDim->indiSetProperty(); 
  areaDimRect->indiSetProperty();
  areaPresets->indiSetProperty();
  areaPresetSize->indiSetProperty();

  return(result);
}

/*---------------------------------------------------------------------------*/

void 
CCDChip::updateAreaSelection(char* name, ISState swit)
{

  if(setAreaSelection(name) != OOB_OK) {
    areaSelection->forceChange();
    areaSelection->formatMsg("%s Error: dimensiones fisicas del chip excedidas",
			     areaSelection->getName());
  } else {
    areaSelection->setValue(name, swit);
  }
  areaSelection->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
CCDChip::updateAreaSelection()
{
  SwitchProperty* sw = areaSelection->getLastOn();
  assert(sw != 0);

  if( sw->equals("FULL_FRAME")) {

    areaDimRect->idleStatus();
    areaPresets->idleStatus();
    areaPresetSize->idleStatus();

  } else if(sw->equals("FULL_FRAME_OV")) {

    areaDimRect->idleStatus();
    areaPresets->idleStatus();
    areaPresetSize->idleStatus();

  } else if (sw->equals("USER_DEFINED")) {
  
    areaDimRect->okStatus();
    areaPresets->idleStatus();
    areaPresetSize->idleStatus();

  } else if (sw->equals("PRESETS")) {

    areaDimRect->idleStatus();
    areaPresets->okStatus();
    areaPresetSize->okStatus();
    
  } else {
    assert("Area selection is not PRESETS, USER_DEFINED, FULL_FRAME, FULL_FRAME_OV" == 0);
  }

  areaDimRect->indiSetProperty();
  areaPresets->indiSetProperty();
  areaPresetSize->indiSetProperty();

}

/*---------------------------------------------------------------------------*/

void 
CCDChip::updateBinning(char* name, ISState swit)
{
  int newBin = 0;


    /* determines new binning from the input */

    if(swit == ISS_ON) {
      if(!strcmp(name,"1X1")) {
	newBin = 1;
      } else if(!strcmp(name,"2X2")) {
	newBin = 2;
      } else if(!strcmp(name,"3X3")) {
	newBin = 3;
      } else if(!strcmp(name,"4X4")) {
	newBin = 4;
      } else {
	assert(newBin != 0);
      }
    }

  if(areaSelection->getValue("FULL_FRAME")) {

    binning->setValue(name, swit);
    bin = newBin;
    setAreaDimFullFrame(newBin);

  } else if(areaSelection->getValue("FULL_FRAME_OV")) {

    binning->setValue(name, swit);
    bin = newBin;
    setAreaDimOverscan(newBin);

  } else if(areaSelection->getValue("PRESETS")) {

  /* 
   * checks for chip dimensions not exceeded when changing binning mode
   * and rescales rectangle accordingly
   */

    int newWidth, oldWidth;
    SwitchProperty* sw = areaPresets->getLastOn();
    assert (sw != 0);

    oldWidth = STATIC_CAST(int, areaPresetSize->getValue("SIZE"));
    newWidth = (bin*oldWidth)/newBin;

    if(setAreaPresets(newWidth, newBin, sw->getName()) != OOB_OK) {
      binning->forceChange();
      binning->formatMsg("%s Error: dimensiones fisicas del chip excedidas",
		    binning->getName());
    } else {			// commit settings
      binning->setValue(name, swit);
      bin = newBin;
    }

  } else if(areaSelection->getValue("USER_DEFINED")) {

    int x1, y1, width, height;

    x1     = (STATIC_CAST(int,areaDimRect->getValue("ORIGX"))*bin)/newBin;
    y1     = (STATIC_CAST(int,areaDimRect->getValue("ORIGY"))*bin)/newBin;
    width  = (STATIC_CAST(int,areaDimRect->getValue("DIMX")) *bin)/newBin;
    height = (STATIC_CAST(int,areaDimRect->getValue("DIMY")) *bin)/newBin;

    if(setAreaDimRect(x1,y1,width,height,newBin) != OOB_OK) {
      binning->forceChange();
      binning->formatMsg("%s Error: dimensiones fisicas del chip excedidas",
		    binning->getName());
    } else {			// commit settings
      binning->setValue(name, swit);
      bin = newBin;
    }
  }

  updateMessage();

  audine->fits.set("CCDBIN1", bin, "binned columns");
  audine->fits.set("CCDBIN2", bin, "binned rows");
  audine->fits.set("PIXSZ1",  bin*ccdData[model].pixSize,
			      "[um] equivalent pixel size");
  audine->fits.set("PIXSZ2",  bin*ccdData[model].pixSize,
			      "[um] equivalent pixel size");

  double gain = photPars->getValue("GAIN");
  double rdnoise = photPars->getValue("RDNOISE")*bin*bin;

  if (gain != 0.0) {
    audine->fits.set("GAIN", gain, "[e-/ADU] CCD Gain");
    
    audine->fits.set("RDNOISE",  rdnoise,
		     "[e-] binning-dependant readout noise");
  }

  areaDim->setValue("PIXSZ", bin*ccdData[model].pixSize);
  areaDim->indiSetProperty();
  binning->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
CCDChip::updateAreaPresetSize(char* name[], double number[], int n)
{
  int newWidth;

  assert(n == 1);

  if(areaPresetSize->getState() != IPS_OK) {
    areaPresetSize->forceChange();
    areaPresetSize->formatMsg("%s Error: Presets no activados",
		  areaPresetSize->getName());
    return;
  }
  
  newWidth = STATIC_CAST(int, areaPresetSize->getValue("SIZE"));
  SwitchProperty* sw = areaPresets->getLastOn();
  assert(sw != 0);

  int width = STATIC_CAST(int, number[0]);

  if(setAreaPresets(width, bin, sw->getName()) != OOB_OK) {
    areaPresetSize->forceChange();
    areaPresetSize->formatMsg("%s Error: dimensiones fisicas del chip excedidas",
		  areaPresetSize->getName());
  } 
}

/*---------------------------------------------------------------------------*/

void 
CCDChip::updateAreaPresets(char* name, ISState swit)
{
  int width;

  if(areaPresets->getState() != IPS_OK) {
    areaPresets->forceChange();
    areaPresets->formatMsg("%s Error: Presets no activados",
		  areaPresets->getName());
    return;
  }

  width = STATIC_CAST(int, areaPresetSize->getValue("SIZE"));

  if(setAreaPresets(width, bin, name) != OOB_OK) {
    areaPresets->forceChange();
    areaPresets->formatMsg("%s Error: dimensiones fisicas del chip excedidas",
			   areaPresets->getName());
  } else { // commit settings
    areaPresets->setValue(name, swit);
  }
  areaPresets->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void 
CCDChip::updateAreaDimRect(char* name[], double number[], int n)
{
  int x1, y1, width, height;
  int i;

  assert (n == 4);

  if(areaDimRect->getState() != IPS_OK) {
    areaDimRect->forceChange();
    areaDimRect->formatMsg("%s Error: Opcion area definible por usuario no activada",
		  areaDimRect->getName());
    return;
  }
  
  x1 = 0, y1 = 0; width=0; height=0;

  for(i=0; i<n; i++) {
    if(!strcmp(name[i],"ORIGX"))
      x1 = STATIC_CAST(int,number[i]);
    else if(!strcmp(name[i],"ORIGY"))
      y1 = STATIC_CAST(int,number[i]);
    else if(!strcmp(name[i],"DIMX"))
      width = STATIC_CAST(int,number[i]);
    else if(!strcmp(name[i],"DIMY"))
      height = STATIC_CAST(int,number[i]);
    else {
      assert("areaDimRect member is not ORIGX, ORIGY, DIMX, DIMY" == 0);
    }
  }

  if(setAreaDimRect(x1,y1,width,height,bin) != OOB_OK) {
    areaDimRect->forceChange();
    areaDimRect->formatMsg("%s Error: dimensiones fisicas del chip excedidas",
		  areaDimRect->getName());
  } 
}

/*---------------------------------------------------------------------------*/
void
CCDChip::updateADCSpeed(char* name, ISState swit)
{
  adcSpeed->setValue(name, swit);

  if(adcSpeed->getValue("100KBS")) {
    adc = ADC_100KBS;
  } else if (adcSpeed->getValue("200KBS")) {
    adc = ADC_200KBS;
  } else {
    assert( (adc == ADC_100KBS) || (adc == ADC_200KBS));
    adc = ADC_100KBS;
  }

  updateMessage();
  adcSpeed->indiSetProperty();
}

/*---------------------------------------------------------------------------*/
void
CCDChip::updatePhotPars(char* name[], double number[], int n)
{
  for(int i=0; i<n; i++)
    photPars->setValue(name[i], number[i]);
 
  double gain    = photPars->getValue("GAIN");
  double rdnoise = photPars->getValue("RDNOISE")*bin*bin;

  if(gain != 0.0) {
    audine->fits.set("GAIN", gain, "[e-/ADU] CCD Gain");
    audine->fits.set("RDNOISE",rdnoise,"[e-] binning-dependant readout noise");
  } else {
    audine->fits.erase("GAIN");
    audine->fits.erase("RDNOISE");
  }

  photPars->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
CCDChip::computeBiasSection(int binning)
{
  char strbuf[18];

  int xmin, ymin, xmax, ymax;

  // Found by examination in IRAF the following good sections in KAF400
  // [796,796,4:520] in binning 1x1
  // [398,398,8:260] in binning 2x2
  // so we have generalized to use the latest column in the chip
  // however ymin equation is empirical

  xmax = STATIC_CAST(int,areaDim->getValue("DIMX"));
  xmin = xmax;
  ymax = STATIC_CAST(int,areaDim->getValue("DIMY"));
  ymin = (ccdData[model].over1.y + 8*binning)/binning; 
  ymin++;			// IRAF is one-based

  sprintf(strbuf,"[%d:%d,%d:%d]",xmin,xmax,ymin,ymax);
  audine->fits.set("BIASSEC", strbuf, "overscan portion of frame");
  
}

/*---------------------------------------------------------------------------*/

void
CCDChip::computeTrimSection(int binning)
{
  char strbuf[18];
  int add, xmin, xmax, ymin, ymax;


  log->verbose(IFUN,"\n");


  add =  binning - ccdData[model].over1.x%binning; // cols to add from 1..bin
  add %= binning;			// cols to add from 0 ... bin-1
  xmin = ccdData[model].over1.x + add;
  log->verbose(IFUN,"x1 = %d\n", xmin);
  xmin /=  binning;
  log->verbose(IFUN,"xmin = %d\n", xmin);

  
  add =  binning - ccdData[model].over1.y%binning; // rows to add from 1..bin
  add %= binning;			// rows to add from 0 ... bin-1
  ymin = ccdData[model].over1.y + add;
  log->verbose(IFUN,"y1 = %d\n", ymin);
  ymin /= binning;
  log->verbose(IFUN,"ymin = %d\n", ymin);


  xmax = (ccdData[model].over1.x+ccdData[model].active.x);
  xmax -= (xmax % binning);
  log->verbose(IFUN,"x2 = %d\n", xmax);
  xmax /= binning;
  log->verbose(IFUN,"xmax = %d\n", xmax);


  ymax = (ccdData[model].over1.y+ccdData[model].active.y);
  ymax -= (ymax % binning);
  log->verbose(IFUN,"ypixmax = %d\n", ymax);
  ymax /= binning;
  log->verbose(IFUN,"xmax = %d\n", ymax);

  // IRAF is one-index based and xmax,ymax were outside section ...

  sprintf(strbuf,"[%d:%d,%d:%d]",xmin+1,xmax,ymin+1,ymax);
  audine->fits.set("TRIMSEC", strbuf,"region to be extracted");

}

/*---------------------------------------------------------------------------*/

void
CCDChip::getMaxDim(int* width, int* height)
{

  *width = ccdData[model].over1.x + ccdData[model].active.x + 
    ccdData[model].over2.x;

  *height = ccdData[model].over1.y + ccdData[model].active.y + 
    ccdData[model].over2.y;
}

/*---------------------------------------------------------------------------*/

void
CCDChip::getDim(int* width, int* height)
{
  *width  = STATIC_CAST(int, areaDimRect->getValue("DIMX"));
  *height = STATIC_CAST(int, areaDimRect->getValue("DIMY"));
}

/*---------------------------------------------------------------------------*/


// THIS METHOD IS NO LONGER NEEDED
// WE RETAIN IT HERE TO DOCUMENT HOW COR CALCULATES THESE PARAMETERS

#if 0				

int
CCDChip::computeUDPPackets(int x1, int y1, int x2, int y2, int binning)
{
  int scannedX, scannedY, rowsUDP, msgsUDP, remRows;

  scannedX = (x2 - x1)/ binning;
  scannedY = (y2 - y1)/ binning;

  // a  BIG WARNING WITH CRISTOBAL'S PROGRAM & MAX_IMG_LEN value

  rowsUDP  = MAX_IMG_LEN/(2*scannedX);	
  msgsUDP  = scannedY / rowsUDP;
  remRows  = scannedY % rowsUDP;

  if(remRows)
    msgsUDP++;

  //bytesUDP = 2*rowsUDP*scannedX+3*sizeof(int16); 
  //remBytes = 2*remRows*scannedX+(UDP_BUF_SIZE-MAX_FOTO_LEN);
  
  return(msgsUDP);
}

#endif
