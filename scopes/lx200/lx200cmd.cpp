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

#include <memory.h>

#include "lx200cmd.h"
#include "lx200.h"

/*---------------------------------------------------------------------------*/

Outgoing_Message LX200Command::msg;
char             LX200Command::buffer[MAXBUF];
unsigned int     LX200Command::buflen = 0;

/*---------------------------------------------------------------------------*/

LX200Command::LX200Command(LX200Simple* lx200,  const char* pfx,
			   const char* tag)
  : log(0), teles(lx200), prefix(pfx), parameter(0), busy(false)
{
  log = LogFactory::instance()->forClass(tag);
}

/*---------------------------------------------------------------------------*/

/* sends a telescope command to COR, assuming it is fully formatted */
/* puts the command in the busy state until a response is fully received */
/* except from the commands that do not require response */

void
LX200Command::request()
{

  /* sends a message to LX200 through COR */

  unsigned int n = strlen(prefix);
  unsigned int m = (parameter) ? strlen(parameter) : 0;

  if(n)	
    memcpy(&msg.body.periReq.data,     prefix,    n);

  if(m)
    memcpy(&msg.body.periReq.data[0] + n,parameter, m);

  msg.body.periReq.data[n+m] = '#'; // terminating character
  msg.body.periReq.data[n+m+1] = 0; // just for printing

  teles->sendMessage(&msg, n+m+1);
  log->debug(IFUN,"enviando %s\n", &msg.body.periReq.data);
  
  buflen = 0;			// empties the response buffer
  busy = true;			// busy by default
}

/*---------------------------------------------------------------------------*/

void
LX200Command::handle(const void* data, int len)
{

 /* appends new data to the existing buffer */
  Incoming_Message* msg = STATIC_CAST(Incoming_Message*, data);

  int n = len-sizeof(Header);
  memcpy(buffer+buflen, msg->body.periResp.data, n);
  buflen += n;			// suppose no overflow
  buffer[buflen] = 0;		// marks the end of string
  log->debug(IFUN,"received so far = %s\n",buffer);
}


/*---------------------------------------------------------------------------*/

bool
LX200Command::parseBool(bool* flag)
{
  if(buflen == 0) {
    *flag = false;
    return(false);
  }

  if(buffer[0] == '1')
    *flag = true;
  else
    *flag = false;

  return(true);
}

/*---------------------------------------------------------------------------*/

bool 
LX200Command::parseMessage(char** msg)
{
   /* is response complete ? */
  if(buffer[buflen-1] != '#')
    return(false);

  *msg = strndup(buffer, buflen-1); // strips #
  return(true);
}

/*---------------------------------------------------------------------------*/

void
LX200Command::timeout(PropertyVector* pv)
{
  pv->formatMsg("ERROR: %s: Command timeout (%s)", pv->getName(), prefix);
  pv->indiMessage();
  pv->alertStatus();
  pv->indiSetProperty();
  log->error(IFUN,"Command timeout (%s)\n", prefix);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

