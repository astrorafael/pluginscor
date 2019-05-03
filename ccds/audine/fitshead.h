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

#ifndef FITSHEADER_H
#define FITSHEADER_H


#include <stdio.h>

class FITSHeader {
 
 public:

  
  /* public constants */

  static const int NUMCARDS = 36; /* FITS cards per block */
  static const int CARDSZ  = 80; /* single card width in bytes */
  static const int RECORDSZ = 2880; /* single block header size in bytes */
  static const char BLANK   = ' ';
  static const int KEYSZ    = 8; /* max size of keyword in bytes */
  static const int HEADERSZ = 2*RECORDSZ; /* total header size */
  static const int STRINGSZ = 68; /* max size of string valued cards */

  /* constructors and destructor */

  FITSHeader();
  ~FITSHeader();

 
  /* saves header into a FILE */

  void save(FILE* fp);

  /* ********************* */
  /* header management API */
  /* ********************* */

  /* inserts or replace a boolean FITS card given by 'key' */
  void set(const char* key, bool val, const char* comment = 0);

  /* inserts or replace an integer FITS card given by 'key' */
  void set(const char* key, int val, const char* comment = 0);

  /* inserts or replace a decimal FITS card given by 'key' */
  void set(const char* key, double val, const char* comment = 0);

  /* inserts or replace a string FITS card given by 'key' */
  void set(const char* key, const char* val, const char* comment = 0);

  /* inserts or replace a FITS 'COMMENT ' or 'HISTORY ' card */
  void setVoid(const char* key, const char* text);

  /* inserts a FITS 'COMMENT ' or 'HISTORY ' card */
  void appendVoid (const char* key, const char* text);

  /* deletes card given by key */
  void erase(const char* key);

 private:

  char header[HEADERSZ];	/* FITS header data */
  char pad[CARDSZ+1];		/* scratchpad buffer  */
  int  lastCard;	      /* index for last used card. -1=empty */


  /******************/
  /* helper methods */
  /******************/


  /* fints card with a given keyword name. -1 if not found else the index */
  int find(const char* key);

  /* formats a boolean FITS card given by 'key' */
  void format(const char* key, bool val, const char* comment = 0);

  /* formats an integer FITS card given by 'key' */
  void format(const char* key, int val, const char* comment = 0);

  /* formats a decimal FITS card given by 'key' */
  void format(const char* key, double val, const char* comment = 0);

  /* formats a string FITS card given by 'key' */
  void format(const char* key, const char* val, const char* comment = 0);

  /* formats a void FITS card */
  void formatVoid(const char* key, const char* comment);

  /* formats the keyword part in to the key buffer */
  const char* formatKey(const char* key);

  /* appends recently formatted FITS card to the header */
  void append();
  
  /* generates the first END keyword */
  void end();

  /* generates the 5 mandatory keyw. SIMPLE,BITPIX,NAXES,NAXIS1,NAXIS2,END */
  void init();
  
};

/*---------------------------------------------------------------------------*/

inline
FITSHeader::FITSHeader()
{
  init();
}

/*---------------------------------------------------------------------------*/

inline
FITSHeader::~FITSHeader()
{
}

/*---------------------------------------------------------------------------*/

#endif
