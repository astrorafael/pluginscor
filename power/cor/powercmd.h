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

#ifndef CORPOWER_COMMANDS_H
#define CORPOWER_COMMANDS_H

#include <indicor/api.h>

class CORPower;			/* forward reference */

/*
 * Set power outputs by user request
 */

class SetPower : public Command
{

 public:

  /* constructors and destructor */

  SetPower(CORPower* corpower);
  virtual ~SetPower();

  /* executes the command request.  */  
  virtual void request();

  /* analyzes data comming from COR and processes it */
  /* can be called one or several times */
  /* appends new data to the existing buffer */
  virtual void handle(const void* data, int len);

  /* execute actions following a complete reception of data */
  virtual void response();

  /* command timeout while waiting for a complete response */
  virtual void timeout();	

  /* true if command is waiting for a complete response*/
  /* there may be commands that do not need it at all */
  /* a default implementation is provided */
  virtual bool isBusy() const;
  
  /* sets a new timeout in milliseconds */
  virtual void setTimeout(unsigned int value);

 private:

  static Outgoing_Message msg;		/* message to COR */

  Log*      log;		/* log object */
  CORPower* cor;		/* where commands act upon */
  bool     busy;		/* busy flag */
  SwitchPropertyVector* actuatorsTrg;
};

/*---------------------------------------------------------------------------*/

inline
SetPower::~SetPower()
{
  delete log;
}

/*---------------------------------------------------------------------------*/

inline bool
SetPower::isBusy() const
{  
  return(busy);
}

/*---------------------------------------------------------------------------*/

inline void
SetPower::setTimeout(unsigned int value)
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*
 * Queries power inputs & outputs
 * This command is meant to be used in a periodic poll
 */


class PowerStatus : public Command
{

 public:

  /* constructors and destructor */

  PowerStatus(CORPower* corpower);
  virtual ~PowerStatus();

  /* executes the command request.  */  
  virtual void request();

  /* analyzes data comming from COR and processes it */
  /* can be called one or several times */
  /* appends new data to the existing buffer */
  virtual void handle(const void* data, int len);

  /* execute actions following a complete reception of data */
  virtual void response();

  /* command timeout while waiting for a complete response */
  virtual void timeout();	

  /* true if command is waiting for a complete response*/
  /* there may be commands that do not need it at all */
  /* a default implementation is provided */
  virtual bool isBusy() const;
  
  /* sets a new timeout in milliseconds */
  virtual void setTimeout(unsigned int value);

 private:

  static Outgoing_Message msg;		/* message to COR */

  Log*      log;		/* log object */
  CORPower* cor;		/* where commands act upon */
  bool     busy;		/* busy flag */
  LightPropertyVector* actuatorsStat;
  LightPropertyVector*    inputsG[4];
};

/*---------------------------------------------------------------------------*/

inline
PowerStatus::~PowerStatus()
{
  delete log;
}

/*---------------------------------------------------------------------------*/

inline bool
PowerStatus::isBusy() const
{  
  return(busy);
}

/*---------------------------------------------------------------------------*/

inline void
PowerStatus::setTimeout(unsigned int value)
{
}

/*---------------------------------------------------------------------------*/

inline void
PowerStatus::response()
{
}


#endif
