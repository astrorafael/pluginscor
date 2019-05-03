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


#ifndef AUDINE_CHIP_H
#define AUDINE_CHIP_H

#include <string>

#pragma pack(1)			/* byte alignment */

struct CCDPoint {
  int x;
  int y;
};

typedef CCDPoint CCDArea;	/* area in terms of x,y dimensions */

struct ClockSeq {		/* variable-length clock sequence */
  const u_char* start;
  const u_char length;
};


struct CCDData {
  CCDPoint over1;		/* overscan size before active area */
  CCDArea active;		/* active area size */
  CCDPoint over2;		/* overscan size after active area */
  float pixSize;		/* pixel size in microns */
  int   dummies;		/* number of shift register dummy pixels */
  ClockSeq clear;		/* clearing sequence */
  ClockSeq vert;		/* V1 & V2 phases */
  ClockSeq read[4][2];		/* readout sequences for slow(0)/fast(1) ADC
				 and the four binning modes (0..3)*/
};

#pragma pack(0)			/* alignment by default */

/* indexes to CCDData table */

#define CCD_KAF400  0		
#define CCD_KAF1600 1
#define CCD_KAF3000 2
#define CCD_KAF3200 3		/* not yet supported */

/* indexes to sequence subtable */

#define ADC_100KBS 0
#define ADC_200KBS 1

/* OUT OF BOUNDS IN SELECTE AREA ENUMERATION */

#define OOB_OK  0x00
#define OOB_X   0x01		/* out of bounds in X dimension */
#define OOB_Y   0x02		/* out of bounds in Y dimension */
#define OOB_XY  0x03		/* out of bounds in X and Y */

class Audine;			/* forward reference */


/* This class represents the chip topological features and 
 * handles all geometry operations delegated from the parent Audine
 * object.
 */

class CCDChip {

 public:

   CCDChip(Audine* aud); 
  ~CCDChip() { delete log; }

  /* chip initialization from current device tree */
  void init();

  /* action when CHIP switch is selected */
  void updateChip(char* name, ISState swit);

  /* action when ADC_SPEED switch is selected */
  void updateADCSpeed(char* name, ISState swit);

  /* action when BINNING switch is selected */
  void updateBinning(char* name, ISState swit);

  /* action when AREA_SELECTION switch is selected */
  void updateAreaSelection(char* name, ISState swit);

  /* action when AREA_PRESETS switch is selected */
  void updateAreaPresets(char* name, ISState swit);

  /* action when PRESET_SIZE numbers are set */
  void updateAreaPresetSize(char* name[], double number[], int n);

  /* action when AREA_DIM_RECT numbers are set */
  void updateAreaDimRect(char* name[], double number[], int n);

  /* action when PHOT_PARS are updated */
  void updatePhotPars(char* name[], double number[], int n);

  /* action when COR calleb by changing its CCD_TEMP */
  bool updateTemp(PropertyVector* pvorig);

  /* action when Audine goes from Idle to Ok state */
  void updateAreaSelection();

  /* returns selected ADC index */
  int getADC() { return(adc); }	

  /* gets the maximun original dimensions for the selected CCD model */
  void getMaxDim(int* width, int* height);

  /* gets the dimensions as seen by the GUI for the selected CCD model */
  void getDim(int* width, int* height);

  /* gets the selected CCD model's name */
  const char* getModel() { return(ccdModel->getLastOn()->getName()); }

 private:

  Log* log;

  /*********************************/
  /* THE USER INTERFACE PROPERTIES */
  /*********************************/

  SwitchPropertyVector* adcSpeed;
  SwitchPropertyVector* areaSelection;
  SwitchPropertyVector* areaPresets;
  SwitchPropertyVector* binning;
  SwitchPropertyVector* ccdModel;

  NumberPropertyVector* areaDim;
  NumberPropertyVector* areaDimRect;
  NumberPropertyVector* areaPresetSize;
  NumberPropertyVector* ccdTemp;
  NumberPropertyVector* photPars;

  /********************/
  /* other attributes */
  /********************/


  Audine* audine;

  int bin;			/* cached binning mode as integer */
  int model;			/* index to CCD data models */
  int adc;			/* index to sequence subtable based o ADC speed */
  int udpMsgs;		    /* expected # of UDP packets to receive */

  /******************/
  /* HELPER METHODS */
  /******************/

  /* bring to consistent state all related properties at startup */
  void sync();
  
  /* performs an out-of-bounds checking for in put parameters */
  int outOfBounds(int x1, int y1, int width, int height);

  /* updates AREA_DIM_RECT property to full frame */
  void setAreaDimFullFrame(int binning);

  /* updates AREA_DIM_RECT property to full frame with overscan */
  void setAreaDimOverscan(int binning);

  /* updates AREA_DIM_RECT property to selected rect */
  /* returns an OOB_x flag */
  int setAreaDimRect(int x1, int y1, int width, int height, int binning);

  /* factored method for the five methods below */
  /* returns an OOB_x flag */
  int setAreaDimCommon(int width, int binn, int x1, int y1, int x2, int y2);

  /* updates AREA_DIM_RECT property to very fast preset rect */
  /* returns an OOB_x flag */
  int setAreaDimVFastRect(int width, int binning);

  /* updates AREA_DIM_RECT property to fast preset rect */
  /* returns an OOB_x flag */
  int setAreaDimFastRect(int width, int binning);

  /* updates AREA_DIM_RECT property to center preset rect */
  /* returns an OOB_x flag */
  int setAreaDimCenterRect(int width, int binning);

  /* updates AREA_DIM_RECT property to slow preset rect */
  /* returns an OOB_x flag */
  int setAreaDimSlowRect(int width, int binning);

  /* updates AREA_DIM_RECT property to very slow preet rect */
  /* returns an OOB_x flag */
  int setAreaDimVSlowRect(int width, int binning);

  /* collects all five methods above into a single method */
  int setAreaPresets(int width, int binning, const char* name);

  /* update status of related properties according to area selection */
  /* return an out-of-bounds constant (OOB_xxx) */
  int setAreaSelection(const char* name);

  /* updates audine request message */
  void updateMessage();

  /* for BIASSEC keyword in full frame with overscan images */
  void computeBiasSection(int binning);

  /* for TRIMSEC keyword in full frame with overscan images */
  void computeTrimSection(int binning);

};



#endif
