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

#include "fitshead.h"


#define MAX(a,b) ((a > b) ? (a) : (b))
#define MIN(a,b) ((a < b) ? (a) : (b))

/*---------------------------------------------------------------------------*/

const char*
FITSHeader::formatKey(const char* key)
{
  static char _key[KEYSZ+1];		/* keyword scratch pad buffer */

  strncpy(_key, key, KEYSZ);
  _key[KEYSZ] = 0;		// make sure string is null terminated
  return(_key);
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::format(const char* key, bool val, const char* comment)
{
  int len;
  char c = (val) ? 'T' : 'F';

  if(comment)
    snprintf(pad, sizeof pad, "%-8s= %20c / %s",formatKey(key), c, comment);
  else
    snprintf(pad, sizeof pad, "%-8s= %20c", formatKey(key), c);

  len = strlen(pad);
  if(len < CARDSZ) {
    memset(&pad[len], BLANK, CARDSZ - len);
    pad[CARDSZ] = 0;
  }

}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::format(const char* key, int val, const char* comment)
{
  int len;

  if(comment)
    snprintf(pad, sizeof pad, "%-8s= %20d / %s", formatKey(key), val, comment);
  else
    snprintf(pad, sizeof pad, "%-8s= %20d", formatKey(key), val);

  len = strlen(pad);
  if(len < CARDSZ) {
    memset(&pad[len], BLANK, CARDSZ - len);
    pad[CARDSZ] = 0;
  }
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::format(const char* key, double val, const char* comment)
{

  int len;

  if(comment)
    snprintf(pad, sizeof pad, "%-8s= %20g / %s", formatKey(key), val, comment);
  else
    snprintf(pad, sizeof pad, "%-8s= %20g", formatKey(key), val);

  len = strlen(pad);
  if(len < CARDSZ) {
    memset(&pad[len], BLANK, CARDSZ - len);
    pad[CARDSZ] = 0;
  }  
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::format(const char* key, const char* val, const char* comment)
{
  int len;
  char strbuf[68+1];

  strncpy(strbuf, val, 68);	// make sure val string can be held between ' '

  if(comment) {

    len = strlen(val);
    len = MAX(len, 8);

    if(len <19) {		// comment is justified
      snprintf(pad, sizeof pad, "%-8s= '%-8s'%*c / %s",
	       formatKey(key), strbuf, 18-len, BLANK, comment);
    } else {			// comment just follows string
      snprintf(pad, sizeof pad, "%-8s= '%-8s' / %s",
	       formatKey(key), strbuf, comment);
    }
  }

  else				// no comment
    snprintf(pad, sizeof pad, "%-8s= '%-8s'", formatKey(key), strbuf);

  len = strlen(pad);
  if(len < CARDSZ) {
    memset(&pad[len], BLANK, CARDSZ - len);
    pad[CARDSZ] = 0;
  }
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::formatVoid(const char* key, const char* comment)
{
  int len;

  snprintf(pad, sizeof pad, "%-8s%s", formatKey(key), comment);

  len = strlen(pad);
  if(len < CARDSZ) {
    memset(&pad[len], BLANK, CARDSZ - len);
    pad[CARDSZ] = 0;
  }
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::end()
{
  sprintf(pad, "%-80s","END");
  memcpy(header, pad, CARDSZ);
  lastCard = 0;
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::append()
{
  if(lastCard == 2*NUMCARDS -1)
    return;			// siently ignores appending to a full header

  // move the END card to a higher location
  memcpy(&header[CARDSZ*(lastCard+1)], &header[CARDSZ*lastCard], CARDSZ);

  // move the new card to the last used location
  memcpy(&header[CARDSZ*lastCard], pad, CARDSZ);

  lastCard++;
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::init()
{
  end();			// generates the END keyword
  set("SIMPLE", true, "File does conform to FITS standard");
  set("BITPIX", 16, "Number of bits per data pixel");
  set("NAXIS", 2, "Number of data axes");
  set("NAXIS1", 1, "columns");	// fake value to make room
  set("NAXIS2", 1, "rows");	// fake value to make room
  appendVoid("COMMENT", "Written by INDI driver 'PACORRO'");
  appendVoid("COMMENT", "Programa de Adquisicion del COR de Rafael gOnzalez");

  for(int i=lastCard+1; i<2*NUMCARDS; i++)
    memset(&header[i*CARDSZ],BLANK, CARDSZ);
}

/*---------------------------------------------------------------------------*/

int
FITSHeader::find(const char* key)
{
  int i;
  char buf[KEYSZ+1];

  snprintf(buf, sizeof(buf), "%-8s", key);
  
  // starting from the end of header.
  // convenient to edit the last COMMENT or HISTORY card

  for(i=lastCard; i > -1; i--) {
    if(!strncmp(&header[CARDSZ*i], buf, KEYSZ)) {
      break;
    }
  }

  return(i);
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::set(const char* key, bool val, const char* comment)
{
  int card;

  format(key, val, comment);
  card = find(key);
  if(card == -1)
    append();
  else {
    memcpy(&header[card*CARDSZ], pad, CARDSZ);
  }
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::set(const char* key, int val, const char* comment)
{
  int card;

  format(key, val, comment);
  card = find(key);
  if(card == -1)
    append();
  else {
    memcpy(&header[card*CARDSZ], pad, CARDSZ);
  }
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::set(const char* key, double val, const char* comment)
{
  int card;

  format(key, val, comment);
  card = find(key);
  if(card == -1)
    append();
  else {
    memcpy(&header[card*CARDSZ], pad, CARDSZ);
  }
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::set(const char* key, const char* val, const char* comment)
{
  int card;

  format(key, val, comment);
  card = find(key);
  if(card == -1)
    append();
  else {
    memcpy(&header[card*CARDSZ], pad, CARDSZ);
  }
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::setVoid(const char* key, const char* comment)
{
  int card;

  formatVoid(key, comment);
  card = find(key);
  if(card == -1)
    append();
  else {
    memcpy(&header[card*CARDSZ], pad, CARDSZ);
  }
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::appendVoid(const char* key, const char* comm)
{  
  formatVoid(key, comm);
  append();
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::erase(const char* key)
{
  int card;

  if(lastCard < 0)		// empty header
    return;

  card = find(key);		// not found
  if(card == -1)
    return;

  // shift down cards
  for(int i=card+1; i<= lastCard; i++)
    memcpy(&header[(i-1)*CARDSZ], &header[i*CARDSZ], CARDSZ);

  // clears old top keyword
  memset(&header[lastCard*CARDSZ], BLANK, CARDSZ);
  lastCard--;
}

/*---------------------------------------------------------------------------*/

void 
FITSHeader::save(FILE* fp)
{
  int n;

  n = (lastCard < NUMCARDS) ? NUMCARDS : 2*NUMCARDS;
  fwrite(header, CARDSZ, n, fp);
 
}

/*---------------------------------------------------------------------------*/


