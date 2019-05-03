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
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#ifndef AUDINE_H
#include "audine.h"
#endif

#include "perscount.h"

/*---------------------------------------------------------------------------*/

#define REPLACEMENT_CHAR '-'

static void
replaceSpaces(const char* src, char* dest, int len)
{
  const char* p;
  char* q;
  int i;

  for(i=0, p=src, q=dest; *p != 0 && i<len ; p++, q++, i++) {
    if(isspace(*p))
      *q = REPLACEMENT_CHAR;	
    else
      *q = *p;     
  }
  *q = 0;
}

/*---------------------------------------------------------------------------*/

Storage::Storage(Audine* ccd) : log(0), fp(0), imageSize(0), byteCount(0), 
    recvByteCount(0), audine(ccd), error(false), fileCount(0), series() 
{
  log = LogFactory::instance()->forClass("Storage");
}

/*---------------------------------------------------------------------------*/

void
Storage::init()
{

  /****************************/
  /* non resetable properties */
  /****************************/

  focusBuffer  = DYNAMIC_CAST(NumberPropertyVector*, audine->device->find("FOCUS_BUFFER"));
  assert(focusBuffer != NULL);


  storage  = DYNAMIC_CAST(TextPropertyVector*, audine->device->find("STORAGE"));
  assert(storage != NULL);

  eventFIFO  = DYNAMIC_CAST(TextPropertyVector*, audine->device->find("EVENT_FIFO"));
  assert(eventFIFO != NULL);

  storageSeries  = DYNAMIC_CAST(SwitchPropertyVector*, audine->device->find("STORAGE_SERIES"));
  assert(storageSeries != NULL);

  storageFlip  = DYNAMIC_CAST(SwitchPropertyVector*, audine->device->find("STORAGE_FLIP"));
  assert(storageFlip != NULL);

  prefix  = storage->getValue("PREFIX");
  flipUD  = storageFlip->getValue("FLIP_UP_DOWN");
  flipLR  = storageFlip->getValue("FLIP_LEFT_RIGHT");
  N       = STATIC_CAST(int,focusBuffer->getValue("SIZE"));

  createSubdir(getenv("HOME"),"ccd"); // the initial subdirectory created
  calcJD();			// Today's Julian Day
  createSubdir(storage->getValue("DIR"), jdStr);

  initFIFO();

#define SERIES_FILE "/.series"

  // The series counter is created in a defaut location in the default constructor
  // we want to preserve the existing value in the new directory
  // so we set the starting value first.
  // when we change locations it reads the existing value 
  // in that directory if one exists
  // otherwise it gets initialized with the 255 value
  
  series.set(255);  		
  // another way to concat strings ...
  series.init(std::string(storage->getValue("DIR")).append(SERIES_FILE).c_str());
  
 
}

/*---------------------------------------------------------------------------*/

void
Storage::createSubdir(const char* baseDir, const char* subDir)
{
  int  res, len;
  char dirpath[256];

  snprintf(dirpath, sizeof dirpath, "%s/%s", baseDir, subDir);
  len = strlen(dirpath);
  len--;			// last index
  if(dirpath[len] == '/')	// gets rid of trailing /
    dirpath[len] = 0;

  // creates dir with 0755 permissions
  // ok if already exists

  res = mkdir(dirpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
  if (res == -1 && errno != EEXIST) {
    log->error(IFUN,"%s\n", strerror(errno));
    storage->formatMsg("Error Almacenamiento: %s", strerror(errno));
    storage->forceChange();
  } else {
    storage->setValue("DIR", dirpath); 
  }
  dirname = storage->getValue("DIR");
  storage->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
Storage::calcJD()
{
  int jd;
  time_t t;
  struct tm* timestruct;
  double dJD;

  time(&t);
  timestruct = gmtime(&t); 
  dJD = UTtoJD(timestruct);
  jd = STATIC_CAST(int, dJD);
  snprintf(jdStr, sizeof jdStr, "%d", jd);
}

/*---------------------------------------------------------------------------*/

void
Storage::initFIFO()
{
  int res;

  snprintf(fifoname, sizeof fifoname, "%s/.xephem", getenv("HOME"));

  // creates dir with 0755 permissions
  // ok if already exists

  res = mkdir(fifoname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
  if (res == -1 && errno != EEXIST) {
    log->error(IFUN,"%s\n", strerror(errno));
    eventFIFO->formatMsg("Error Almacenamiento: %s", strerror(errno));
    eventFIFO->forceChange();
  } else {
    eventFIFO->setValue("DIR", fifoname); 
  }
  eventFIFO->indiSetProperty();

  // builds whole path

  snprintf(fifoname, sizeof fifoname, "%s/%s",
	   eventFIFO->getValue("DIR"), eventFIFO->getValue("NAME"));

  // creates FIFO mode 644 but does not open it

  res = mknod(fifoname, S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, 0);
  if (res == -1 && errno != EEXIST) {
    log->error(IFUN,"%s\n", strerror(errno));
    audine->device->formatMsg("Error creando la FIFO XEphem: %s",strerror(errno));
    audine->device->indiMessage();
  }

  fifo = -1;
}


/*---------------------------------------------------------------------------*/

void
Storage::openFIFO()
{

  if(fifo != -1)
    return;
  
  fifo = open(fifoname, O_NONBLOCK | O_WRONLY);
  if(fifo == -1 && errno == ENXIO) {
    audine->device->formatMsg("FIFO %s not open by XEphem", fifoname);
    audine->device->indiMessage();
    log->warn(IFUN,"XEphem no ha abierto la FIFO %s\n", fifoname);
  } else if(fifo == -1) {
    audine->device->formatMsg("Error FIFO %s", strerror(errno));
    audine->device->indiMessage();
    log->error(IFUN,"error FIFO %s\n", strerror(errno));
  } else {
    log->verbose(IFUN,"all is ok fd = %d\n",fifo);
  }
}

/*---------------------------------------------------------------------------*/

void
Storage::updateFlip(char* name, ISState swit)
{
  storageFlip->setValue(name, swit);
  storageFlip->indiSetProperty();
  flipUD = storageFlip->getValue("FLIP_UP_DOWN");
  flipLR = storageFlip->getValue("FLIP_LEFT_RIGHT");
}

/*---------------------------------------------------------------------------*/

void
Storage::updateSeries(char* name, ISState swit)
{
  storageSeries->setValue(name, swit);
  storageSeries->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
Storage::update(char* name[], char* text[], int n)
{

  for(int i=0; i<n; i++) {

    if(strlen(text[i]) != 0) {	// only deal with non empty new values

      if(!strcmp(name[i], "DIR")) {
	createSubdir(text[i],"");
      } else {
	storage->setValue(name[i],text[i]);
      }
    }
  }

  storage->indiSetProperty();
  prefix  = storage->getValue("PREFIX");

}

/*---------------------------------------------------------------------------*/

void
Storage::update(char* name[], double number[], int n)
{

  for(int i=0; i<n; i++) {
    focusBuffer->setValue(name[i], number[i]);
  }

  focusBuffer->indiSetProperty();
  N  = STATIC_CAST(int,focusBuffer->getValue("SIZE"));

}

/*---------------------------------------------------------------------------*/

void
Storage::updatePrefix(const char* name)
{
  char newName[64];

  replaceSpaces(name, newName, sizeof(newName)-1);
  storage->setValue("PREFIX", newName);
  storage->indiSetProperty();
  prefix  = storage->getValue("PREFIX");
}

/*---------------------------------------------------------------------------*/

void
Storage::cancel()
{
  int res;

  if(fp != NULL) {
    fclose(fp);
    res = unlink(curFile);		// borra el fichero
    assert(res == 0);
  }
  fp = NULL;
}

/*---------------------------------------------------------------------------*/

void
Storage::nextFile()
{
  if(error) {
    log->warn(IFUN,"Ignoring CCD data\n");
    return;
  }

  byteCount = 0;		// resets per-file running counts
  recvByteCount = 0;

  generatePath();		// generates 'curFile' path according to context

  fp = fopen(curFile, "w");
  if(fp == NULL) {
    error = true;
    audine->device->formatMsg("Almacenamiento: %s",strerror(errno));
    audine->device->indiMessage();
  }  

  /* writes a temporary header, complete except for exposure dates & times */

  audine->fits.set("DATE", timestamp(), "file creation time");
  audine->fits.save(fp);

  if(flipUD) {
    fseek(fp, imageSize, SEEK_CUR); // file pointer to the end of image
  }
}

/*---------------------------------------------------------------------------*/

void
Storage::start(int x, int y)
{

  if(error) {
    log->warn(IFUN,"Storage: Ignoring start of sequence\n");
    return;
  }
   
  fileCount = 1;		// initializes running counts
  byteCount = 0;
  recvByteCount = 0;
  imageSize = sizeof(pixel_t) * x * y;
  width = x;

  openFIFO();
  updateSeriesCounter();			// always before nextFile()
  nextFile();
}

/*---------------------------------------------------------------------------*/

void
Storage::saveNoFlip(const void* data, int byteLen)
{
  Incoming_Message* msg = STATIC_CAST(Incoming_Message*, data);
 
  int nwritten;
  int pixelCount = (byteLen - IMG_HEAD)/2;
  
  // FITS needs a byte swap for x86
  // swap performed in-place

  u_char* px;
  u_char temp;
  int i;

  for(i=0, px = msg->body.imgData.data; i< pixelCount; i++) {
    temp    = *px;
    *px     = *(px+1);
    *(px+1) = temp;
    px += sizeof(pixel_t);
  }

  /* writes chunk of data */

  recvByteCount += 2*pixelCount;
  nwritten = fwrite(msg->body.imgData.data, sizeof(pixel_t), pixelCount, fp);
  byteCount += 2*nwritten;
}

/*---------------------------------------------------------------------------*/

void
Storage::saveFlipLR(const void* data, int byteLen)
{
  Incoming_Message* msg = STATIC_CAST(Incoming_Message*, data);

  int nwritten;
  int pixelCount = (byteLen - IMG_HEAD)/2;
  int nrows      = pixelCount / width;
  int N          = width*2;	// row size in bytes
 
  // performs row flipping and FITS byte swap 
  // at the same time for a given chunk of data
  
  u_char* first;
  u_char* last;
  u_char temp;

  int i,j;

  for(i=0; i<nrows; i++) {

    first = msg->body.imgData.data + i*N;
    last  = msg->body.imgData.data + (i+1)*N - 1;

    for(j=0; j<width; j++, first++, last--) {
      temp    = *first;
      *first  = *last;
      *last   = temp;
    }
  }


  /* writes chunk of data */

  recvByteCount += 2*pixelCount;
  nwritten = fwrite(msg->body.imgData.data, sizeof(pixel_t), pixelCount, fp);
  byteCount += 2*nwritten;
}

/*---------------------------------------------------------------------------*/

void
Storage::saveFlipUD(const void* data, int byteLen)
{
  Incoming_Message* msg = STATIC_CAST(Incoming_Message*, data);
 
  int nwritten;
  int pixelCount = (byteLen - IMG_HEAD)/2;
  int nrows      = pixelCount / width;
  int N          = width*2;	// row size in bytes

  // rewinds file this amount from the given last position

   fseek(fp, -(byteLen-IMG_HEAD), SEEK_CUR); 

  u_char* px;
  u_char temp;
  int i, j;

  // saves last row in chunk in first place
  // FITS needs a byte swap for x86
  // swap performed in-place

  for(i=nrows-1; i>-1; i--) {
    px = msg->body.imgData.data + i*N;
    for(j=0; j< width; j++) {
      temp    = *px;
      *px     = *(px+1);
      *(px+1) = temp;
      px += sizeof(pixel_t);
    }
    recvByteCount += N;
    nwritten = fwrite(msg->body.imgData.data+i*N, sizeof(pixel_t), width, fp);
    byteCount += 2*nwritten;
  }

  fseek(fp, -(byteLen-IMG_HEAD), SEEK_CUR); 
}

/*---------------------------------------------------------------------------*/

void
Storage::saveFlipLRUD(const void* data, int byteLen)
{
  Incoming_Message* msg = STATIC_CAST(Incoming_Message*, data);
 
  int nwritten;
  int pixelCount = (byteLen - IMG_HEAD)/2;
  int nrows      = pixelCount / width;
  int N          = width*2;	// row size in bytes

  // rewinds file this amount from the given last position

  fseek(fp, -(byteLen-IMG_HEAD), SEEK_CUR); 

  u_char* first;
  u_char* last;
  u_char temp;

  int i, j;

  // saves last row in chunk in first place
  // LR flip and FITS byte swapping performed in-place

  for(i=nrows-1; i>-1; i--) {

    first = msg->body.imgData.data + i*N;
    last  = msg->body.imgData.data + (i+1)*N - 1;

    for(j=0; j<width; j++, first++, last--) {
      temp    = *first;
      *first  = *last;
      *last   = temp;
    }

    recvByteCount += N;
    nwritten = fwrite(msg->body.imgData.data+i*N, sizeof(pixel_t), width, fp);
    byteCount += 2*nwritten;
  }

  // decreases the seek pointer by this amount

  fseek(fp, -(byteLen-IMG_HEAD), SEEK_CUR); 
}

/*---------------------------------------------------------------------------*/

void
Storage::handle(const void* data, int byteLen)
{
  
  if(error) {
    log->warn(IFUN,"Ignoring CCD data\n");
    return;
  }

  if(!flipLR && !flipUD) {
    saveNoFlip(data, byteLen);
  } else if(flipLR && !flipUD) {
    saveFlipLR(data, byteLen);
  } else if(!flipLR && flipUD) {
    saveFlipUD(data, byteLen);
  } else {
    saveFlipLRUD(data, byteLen);
  }
}

/*---------------------------------------------------------------------------*/

void
Storage::notifyXEphem()
{
  int res;

  if(fifo != -1) {
    res = write(fifo, curFile, strlen(curFile));
    if(res == -1 && errno == EPIPE) {
      log->warn(IFUN,"FIFO closed by XEphem\n");
      audine->device->formatMsg("Aviso: XEphem ha cerrado la FIFO");
      audine->device->indiMessage();
      close(fifo);
      fifo = -1;
      openFIFO();
    } else if(res == -1) {
      log->error(IFUN,"XEphem FIFO error %s\n",strerror(errno));
    }
  }
}

/*---------------------------------------------------------------------------*/

void
Storage::end()
{ 
  int rembytes = imageSize % FITSHeader::RECORDSZ;

  assert(imageSize == byteCount);
  assert(imageSize == recvByteCount);

  if (rembytes) {
    while (rembytes++ <  FITSHeader::RECORDSZ)
      putc (0, fp);
  }

  fseek(fp, 0L, SEEK_SET);	// re-writtes header with correct date and time
  audine->fits.save(fp);
  fclose(fp);

  // notifies XEphem on new image to display
  notifyXEphem();

}

/*---------------------------------------------------------------------------*/

void
Storage::generatePath()

{

  // generates  next file name.
  //
  // 'focus' image type form a circular buffer of files
  // to let other apps read a file while writting the next one.
  //
  // for other image types it depends on the STORAGE_SERIES Property
  // If its value is 'Never' then we do not insert a 2 digit hex series 
  // value as part of the file path
  // If its value is 'Always', we do always insert it.
  // Value 'two or more' do it whenerver 2 or more images are being taken.

  if(audine->getImageType() == Audine::FOCUS) {
    snprintf(curFile, sizeof(curFile), "%s/%s_%02d.fit",
	     dirname, prefix, fileCount++ % N);
  } else {			// other image types

    if(storageSeries->getValue("ALWAYS")) {
      snprintf(curFile, sizeof(curFile), "%s/%s_%02X_%03d.fit",
	       dirname, prefix, series.value(), fileCount++);
     } else if(storageSeries->getValue("NEVER")) {
       snprintf(curFile, sizeof(curFile), "%s/%s.fit",
	       dirname, prefix);
     } else if(audine->imgseq.getSequenceSize() > 1) { // two or more ...
       snprintf(curFile, sizeof(curFile), "%s/%s_%02X_%03d.fit",
		dirname, prefix, series.value(), fileCount++);
     } else {
       snprintf(curFile, sizeof(curFile), "%s/%s.fit",
	       dirname, prefix);
     }
  }
  
  log->verbose(IFUN,"file to generate is %s\n",curFile);
}

/*---------------------------------------------------------------------------*/

void
Storage::updateSeriesCounter()
{
  
  // discard al thses three cases
  // else update the series counter

  if(audine->getImageType() == Audine::FOCUS)
    return;

  if(storageSeries->getValue("NEVER"))
    return;
  
  if( storageSeries->getValue("TWO_OR_MORE") && (audine->imgseq.getSequenceSize() == 1) )
    return;

  series.up();
}

/*---------------------------------------------------------------------------*/
