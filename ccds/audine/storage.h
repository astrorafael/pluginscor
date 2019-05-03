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


#ifndef AUDINE_STORAGE_H
#define AUDINE_STORAGE_H

#include "perscount.h"


class Audine;			/* forward reference */

/*
 * This class handles all local storage operations needed for the Audine
 */

class Storage  {

  typedef PersistentCounter<unsigned char> Counter8bit;

 public:

  Storage(Audine* ccd);
 ~Storage() { delete log; }

  /* ****************** */
  /* THE INDI INTERFACE */
  /* ****************** */

  void update(char* name[], char* text[], int n);

  void update(char* name[], double number[], int n);

  void updateFlip(char* name, ISState swit);

  void updateSeries(char* name, ISState swit);

  /*********************************************/
  /* the private interface for image sequencer */
  /*********************************************/

  void init();

  /* prepares the sequence of files */
  void start(int width, int height);

  /* saves a chunk of the pixel array in FITS format */
  void handle(const void* data, int byteLength);

  /* closes current image and prepares next if any */
  void next() { end(); nextFile(); }

  /* closes current image */
  void end();

  /* cancels writting of current image */
  void cancel();

  /* suggest a new prefix to user when image type is changed */
  void updatePrefix(const char* name);

  /* tries to open writer side of XEphem FIFO */
  void openFIFO();

 private:

  NumberPropertyVector* focusBuffer;
  TextPropertyVector* storage;
  TextPropertyVector* eventFIFO;
  SwitchPropertyVector* storageFlip;
  SwitchPropertyVector* storageSeries;

  Log* log;
  FILE* fp;
  int imageSize;		/* predicted image size in bytes */
  int byteCount;		/* running count of written bytes */
  int recvByteCount;		/* running count of recived bytes */
  Audine* audine;

  bool error;
  int fileCount;		/* serves as a suffix for the file name */
  int width;			/* current image width for a sequence of images */

  bool flipLR;			/* flag: save image flipped Left to Right */
  bool flipUD;			/* flag: save image flipped upside down */

  const char* dirname;		/* caches directory entry */
  const char* prefix;		/* caches file prefix */

  char fifoname[256];		/* XEphem's watch FIFO name */
  char curFile[256];		/* current FITS file path */
  int fifo;			/* XEphem FIFO file descriptor */

  char jdStr[16];		/* Julian day (integer) as a string */

  int N;			/* cache of focus buffer 'size' property */

  Counter8bit series;	/* the series counter for image sequencing */

  /******************/
  /* HELPER METHODS */
  /******************/

  void nextFile();
  void notifyXEphem();

  void initFIFO();

  void createSubdir(const char* basedir, const char* subdir);
  void calcJD();		/* today's date as JD string */

  void saveNoFlip(const void* data, int byteLen); /* save as received */
  void saveFlipLR(const void* data, int byteLen); /* save flipped L/R */
  void saveFlipUD(const void* data, int byteLen); /* save flipped U/D */
  void saveFlipLRUD(const void* data, int byteLen); /* save flipped on both axis */

  /* generates a file path according to various rules */
  void generatePath();

  /* conditionally update the series counter */
  void updateSeriesCounter();
};

#endif
