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

#ifndef LX200_BASIC_CMDS_H
#define LX200_BASIC_CMDS_H

/*---------------------------------------------------------------------------*/
/*                           BASIC LX200 COMMANDS                            */
/*---------------------------------------------------------------------------*/

#include <math.h>

#include "lx200cmd.h"

typedef MacroCommand LX200MacroCommand;

/*---------------------------------------------------------------------------*/
/*                           TESTING COMMANDS                                */
/*---------------------------------------------------------------------------*/

class RawCommand : public LX200Command {

 public:

  RawCommand(LX200Simple* teles);
  virtual ~RawCommand() {};

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void request();
  virtual void handle(const void* data, int len);
  virtual void response();
  virtual void timeout();

 private:

  TextPropertyVector* rawCmd;

};


/*---------------------------------------------------------------------------*/
/*                           POSITION COMMANDS                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// :GR# 
// Get Telescope RA 
// Returns: HH:MM.T# or HH:MM:SS# 
// Depending which precision is set for the telescope 
/*---------------------------------------------------------------------------*/


class GetRA : public LX200Command {

 public:

  GetRA(LX200Simple* teles);
  virtual ~GetRA() {};

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void handle(const void* data, int len);
  virtual void response();
  virtual void timeout();

 
 private:

  bool parseRA(float* ra);
  
  NumberPropertyVector* eqCoords;
  float ra;			/* parsed RA */

};

/*---------------------------------------------------------------------------*/

inline void
GetRA::handle(const void* data, int len)
{
  LX200Command::handle(data, len);
  busy = ! parseRA(&ra);
}

/*---------------------------------------------------------------------------*/

inline void
GetRA::response()
{ 

  // we do not send any value to the client because it should be
  // atomically done with DEC in the macro command
  eqCoords->setValue("RA",ra);
}

/*---------------------------------------------------------------------------*/

inline void
GetRA::timeout()
{
  LX200Command::timeout(eqCoords);
}


/*---------------------------------------------------------------------------*/
// :GD#
// Get Telescope Declination. 
// Returns: sDD*MM# or sDD*MM'SS# 
// Depending upon the current precision setting for the telescope.
/*---------------------------------------------------------------------------*/

class GetDEC : public LX200Command {

 public:

  GetDEC(LX200Simple* teles);
  virtual ~GetDEC() {};
  
  /*********************/
  /* redefined methods */
  /*********************/

  virtual void handle(const void* data, int len);
  virtual void response();
  virtual void timeout();
 

 private:
  
  bool parseDEC(float* ra);

  NumberPropertyVector* eqCoords;
  float dec;

};

/*---------------------------------------------------------------------------*/

inline void
GetDEC::handle(const void* data, int len)
{
  LX200Command::handle(data, len);
  busy = ! parseDEC(&dec);
}

/*---------------------------------------------------------------------------*/

inline void
GetDEC::timeout()
{ 
  LX200Command::timeout(eqCoords);
}

/*---------------------------------------------------------------------------*/

inline void
GetDEC::response()
{
  // we do not send any value to the client becacuse it should be
  // atomically done with AR in the macro command

  eqCoords->setValue("DEC",dec);

}


/*---------------------------------------------------------------------------*/
// :SrHH:MM.T# 
// :SrHH:MM:SS# 
// Set target object RA to HH:MM.T or HH:MM:SS depending on
// the current precision setting. 
// Returns: 
// 0 - Invalid
// 1 - Valid
/*---------------------------------------------------------------------------*/

class SetTargetRA : public LX200Command {
public:

  SetTargetRA(LX200Simple* teles);
  virtual ~SetTargetRA() {};

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void request(); 
  virtual void handle(const void* data, int len);
  virtual void timeout();
  virtual void response();


 private:

  char  raStr[15+1];		/* ra as formatted string */
  bool accepted;		/* telescope accepted RA ? */
  NumberPropertyVector* eqCoords;
  SwitchPropertyVector* coordFormat;
};

/*---------------------------------------------------------------------------*/

inline void
SetTargetRA::handle(const void* data, int len)
{
  LX200Command::handle(data, len);
  busy = ! parseBool(&accepted);
}

/*---------------------------------------------------------------------------*/

inline void
SetTargetRA::timeout()
{  
  LX200Command::timeout(eqCoords);
}

/*---------------------------------------------------------------------------*/
// :SdsDD*MM# 
// Set target object declination to sDD*MM or sDD*MM:SS 
// depending on the current precision setting 
// Returns: 
// 1 - Dec Accepted 
// 0 - Dec invalid 
/*---------------------------------------------------------------------------*/

class SetTargetDEC : public LX200Command {
public:

  SetTargetDEC(LX200Simple* teles);
  virtual ~SetTargetDEC() {};

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void request();
  virtual void handle(const void* data, int len);
  virtual void timeout();
  virtual void response();


 private:

  char  decStr[15+1];		/* dec as formatted string */
  bool accepted;		/* telescope accepted DEC ? */
  NumberPropertyVector* eqCoords;
  SwitchPropertyVector* coordFormat;
};

/*---------------------------------------------------------------------------*/

inline void
SetTargetDEC::handle(const void* data, int len)
{
  LX200Command::handle(data, len);
  busy = ! parseBool(&accepted);
}

/*---------------------------------------------------------------------------*/

inline void
SetTargetDEC::timeout()
{  
  LX200Command::timeout(eqCoords);
}

/*---------------------------------------------------------------------------*/
// :CM#
// Synchronizes the telescope's position with the currently selected
// database object's coordinates. 
// Returns: 
// LX200's - a "#" terminated string with the name of 
//           the object that was synced. 
// Autostars & LX200GPS - At static string: " M31 EX GAL MAG 3.5 SZ178.0'#" 
/*---------------------------------------------------------------------------*/

class SyncTelescope : public LX200Command {
 public:

  SyncTelescope(LX200Simple* teles);
  virtual ~ SyncTelescope() {};
  
  /*********************/
  /* redefined methods */
  /*********************/

  virtual void request(); 
  virtual void handle(const void* data, int len);
  virtual void response();
  virtual void timeout();

 private:
  
  NumberPropertyVector* eqCoords;
  char* msg;			/* returned msg from telescope */

};

/*---------------------------------------------------------------------------*/
inline void
SyncTelescope::request()
{ 
  LX200Command::request();
  msg = 0;			// no message received
}

inline void
SyncTelescope::handle(const void* data, int len)
{
  LX200Command::handle(data, len);
  busy = ! parseMessage(&msg);
}

/*---------------------------------------------------------------------------*/
// :U# 
// Toggle between low/hi precision positions 
// Low - RA displays and messages HH:MM.T sDD*MM 
// High - Dec/Az/El displays and messages HH:MM:SS sDD*MM:SS 
// Returns: Nothing 
/*---------------------------------------------------------------------------*/


class TogglePrecision : public LX200Command {
 public:

  TogglePrecision(LX200Simple* teles);
  virtual ~TogglePrecision() {};

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void response();
  virtual bool isBusy() const;
  virtual void timeout();

};

/*---------------------------------------------------------------------------*/

inline bool
TogglePrecision::isBusy() const
{
  return(false);
}

/*---------------------------------------------------------------------------*/

inline void
TogglePrecision::response()
{
}

/*---------------------------------------------------------------------------*/

inline void
TogglePrecision::timeout()
{
}

/*---------------------------------------------------------------------------*/
/*                           MOVEMENT COMMANDS                               */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
// :MS# 
// Slew to Target Object 
// Returns: 
// 0 Slew is Possible 
// 1<string># Object Below Horizon w/string message 
// 2<string># Object Below Higher w/string message 
/*---------------------------------------------------------------------------*/

class SlewToTarget : public LX200Command {

 public:
  
  SlewToTarget(LX200Simple* teles);
  virtual ~SlewToTarget() {};

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void request(); 
  virtual void handle(const void* data, int len);
  virtual void timeout();
  virtual void response();

 private:

  char* msg;			/* message from telescope */
  NumberPropertyVector* eqCoords;

};

/*---------------------------------------------------------------------------*/

inline void
SlewToTarget::request()
{ 
  LX200Command::request();
  msg = 0;			// no message received
}

/*---------------------------------------------------------------------------*/
// :D#
// Requests a string of bars indicating the distance to the current
// library object. 
// Returns: 
// LX200's - a string of bar characters indicating the distance. 
// Autostars and LX200GPS - a string containing one bar until a 
// slew is complete, then a null string is returned ( # char only)
/*---------------------------------------------------------------------------*/

class SlewComplete : public LX200Command {

 public:
  
  SlewComplete(LX200Simple* teles);
  virtual ~SlewComplete() {}

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void handle(const void* data, int len);
  virtual void timeout();
  virtual void response();

 private:
  NumberPropertyVector* eqCoords;

};

/*---------------------------------------------------------------------------*/

inline void
SlewComplete::timeout()
{
  LX200Command::timeout(eqCoords);
}

/*---------------------------------------------------------------------------*/
// :Q#
// Halt all current slewing 
// Returns:Nothing 
/*---------------------------------------------------------------------------*/

class Abort : public LX200Command {

 public:
  
  Abort(LX200Simple* teles);
  virtual ~Abort() {}

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void request();
  virtual void timeout();
  virtual void response();
  virtual bool isBusy() const;

 private:

  SwitchPropertyVector* abortMotion;

};

/*---------------------------------------------------------------------------*/

inline void
Abort::response()
{
}

/*---------------------------------------------------------------------------*/

inline void
Abort::timeout()
{
}

/*---------------------------------------------------------------------------*/

inline bool
Abort::isBusy() const
{
  return(false);
}

/*---------------------------------------------------------------------------*/
/*                         INFO COMMANDS                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// :GVD# 
// Get Telescope Firmware Date 
// Returns: mmm dd yyyy# 
/*---------------------------------------------------------------------------*/

class FirmwareDate : public LX200Command {

 public:
  
  FirmwareDate(LX200Simple* teles);
  virtual ~FirmwareDate() {}

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void handle(const void* data, int len);
  virtual void timeout();
  virtual void response();

 private:

  char* msg;
  TextPropertyVector* mount;
};

/*---------------------------------------------------------------------------*/

inline void
FirmwareDate::handle(const void* data, int len)
{
  LX200Command::handle(data, len);
  busy = !parseMessage(&msg);
}

/*---------------------------------------------------------------------------*/

inline void
FirmwareDate::timeout()
{
  if (msg) 
    free(msg);
  msg = 0;
}


/*---------------------------------------------------------------------------*/
// :GVT# 
// Get Telescope Firmware Time 
// returns: HH:MM:SS# 
/*---------------------------------------------------------------------------*/

class FirmwareTime : public LX200Command {

 public:
  
  FirmwareTime(LX200Simple* teles);
  virtual ~FirmwareTime() {}

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void handle(const void* data, int len);
  virtual void timeout();
  virtual void response();


 private:

  char* msg;
  TextPropertyVector* mount;
};

/*---------------------------------------------------------------------------*/

inline void
FirmwareTime::handle(const void* data, int len)
{
  LX200Command::handle(data, len);
  busy = !parseMessage(&msg);
}

/*---------------------------------------------------------------------------*/

inline void
FirmwareTime::timeout()
{  
  if (msg) 
    free(msg);
  msg = 0;
}


/*---------------------------------------------------------------------------*/
// :GVN# 
// Get Telescope Firmware Number 
// Returns: dd.d# 
/*---------------------------------------------------------------------------*/

class FirmwareNumber : public LX200Command {

 public:
  
  FirmwareNumber(LX200Simple* teles);
  virtual ~FirmwareNumber() {}

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void handle(const void* data, int len);
  virtual void timeout();
  virtual void response();

 private:

  char* msg;
  TextPropertyVector* mount;
};

/*---------------------------------------------------------------------------*/

inline void
FirmwareNumber::handle(const void* data, int len)
{
  LX200Command::handle(data, len);
  busy = !parseMessage(&msg);
}

/*---------------------------------------------------------------------------*/

inline void
FirmwareNumber::timeout()
{  
  if (msg) 
    free(msg);
  msg = 0;
}

/*---------------------------------------------------------------------------*/
// :GVP# 
// Get Telescope Product Name 
// Returns: <string># 
/*---------------------------------------------------------------------------*/

class ProductName : public LX200Command {

 public:
  
  ProductName(LX200Simple* teles);
  virtual ~ProductName() {}

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void handle(const void* data, int len);
  virtual void timeout();
  virtual void response();

 private:

  char* msg;
  TextPropertyVector* mount;
};

/*---------------------------------------------------------------------------*/

inline void
ProductName::handle(const void* data, int len)
{
  LX200Command::handle(data, len);
  busy = !parseMessage(&msg);
}

/*---------------------------------------------------------------------------*/

inline void
ProductName::timeout()
{
  if (msg) 
    free(msg);
  msg = 0;
}


/*---------------------------------------------------------------------------*/
/*                         HELPER MACRO COMMANDS                             */
/*---------------------------------------------------------------------------*/

/* helper clas with common behaviour for MacroSyncToTarget and 
   MacroSlewToTarget */

class MacroSetRaDec : public MacroCommand
{
 public:

  MacroSetRaDec(LX200Simple* lx200, const char* logtag);
  ~MacroSetRaDec();

  /* set target coordinates */
   void setTarget(double ra, double dec);

   /*********************/
   /* redefined methods */
   /*********************/
   
   virtual void request();
   virtual void response();

 protected:

  Log* log;
  NumberPropertyVector* eqCoords;

};

inline 
MacroSetRaDec::~MacroSetRaDec()
{
  delete log;
}

/*---------------------------------------------------------------------------*/
/*                         POSITION MACRO COMMANDS                           */
/*---------------------------------------------------------------------------*/

class MacroGetRaDec : public MacroCommand
{

 public:

  MacroGetRaDec(LX200Simple* teles);
  virtual ~MacroGetRaDec() {};

   /*********************/
   /* redefined methods */
   /*********************/

  virtual void response();

 protected:
  
  Log* log;
  NumberPropertyVector* eqCoords;

};

/*---------------------------------------------------------------------------*/

class MacroGetRaDecSlew : public MacroCommand
{

 public:

  MacroGetRaDecSlew(LX200Simple* teles);
  virtual ~MacroGetRaDecSlew() {};
};

/*---------------------------------------------------------------------------*/

class MacroSyncRaDec : public MacroSetRaDec
{
 public:
  
  MacroSyncRaDec(LX200Simple* teles);
  virtual ~MacroSyncRaDec() {};
};

/*---------------------------------------------------------------------------*/
/*                         MOVEMENT MACRO COMMANDS                           */
/*---------------------------------------------------------------------------*/

class MacroSlewToTarget : public MacroSetRaDec
{
 public:

  MacroSlewToTarget(LX200Simple* teles);
  virtual ~MacroSlewToTarget() {};
};


/*---------------------------------------------------------------------------*/
/*                             INFO MACRO COMMANDS                           */
/*---------------------------------------------------------------------------*/



class MacroGetMount : public MacroCommand
{

 public:

  MacroGetMount(LX200Simple* teles);
  virtual ~MacroGetMount() {};

   /*********************/
   /* redefined methods */
   /*********************/

  virtual void request();
  virtual void response();
  virtual void timeout();

 protected:

  LX200Simple* teles;
  TextPropertyVector* mount;

};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

class TestRA : public LX200Command {

 public:

  TestRA(LX200Simple* teles);
  virtual ~TestRA() {};

  /*********************/
  /* redefined methods */
  /*********************/

  virtual void handle(const void* data, int len);
  virtual void response();
  virtual void timeout();
 
 private:

  bool parseRA(float* ra, int* n);
  
  SwitchPropertyVector* coordFormat;
  float ra;			/* parsed RA */
  int n;			/* n=2 if short, n=3 if long */

};

/*---------------------------------------------------------------------------*/

inline void
TestRA::handle(const void* data, int len)
{ 
  LX200Command::handle(data, len);
  busy = !parseRA(&ra,&n);
}

/*---------------------------------------------------------------------------*/

inline void
TestRA::timeout()
{
  LX200Command::timeout(coordFormat);
}


/*---------------------------------------------------------------------------*/

#if 0				/* NOT YET IMPLEMENTED */

/*---------------------------------------------------------------------------*/
// :Me#
//  Move Telescope East at current slew rate 
// Returns: Nothing 
/*---------------------------------------------------------------------------*/

Command(":Me",NULL);

/*---------------------------------------------------------------------------*/
// :Mn# 
// Move Telescope North at current slew rate 
// Returns: Nothing 
/*---------------------------------------------------------------------------*/

Command(":Me",NULL);

/*---------------------------------------------------------------------------*/
// :Ms#  
// Move Telescope South at current slew rate 
// Returns: Nothing 
/*---------------------------------------------------------------------------*/

Command(":Ms",NULL);

/*---------------------------------------------------------------------------*/
// :Mw# 
// Move Telescope West at current slew rate 
// Returns: Nothing 
/*---------------------------------------------------------------------------*/

Command(":Mw",NULL);

/*---------------------------------------------------------------------------*/
// :GT# 
// Get tracking rate 
// Returns: TT.T# 
// Current Track Frequency expressed in hertz assuming a synchonous motor
// design where a 60.0 Hz motor clock would produce 1 revolution of the 
// telescope in 24 hours. 
/*---------------------------------------------------------------------------*/

Command(":GT","[[:digit]]{2}\.[[:digit:]]#");

/*---------------------------------------------------------------------------*/
// :RC# 
// Set Slew rate to Centering rate (2nd slowest) 
// Returns: Nothing 
/*---------------------------------------------------------------------------*/

Command(":RC",NULL);

/*---------------------------------------------------------------------------*/
// :RG# 
// Set Slew rate to Guiding Rate (slowest) 
// Returns: Nothing 
/*---------------------------------------------------------------------------*/

Command(":RG",NULL);

/*---------------------------------------------------------------------------*/
// :RM# 
// Set Slew rate to Find Rate (2nd Fastest) 
// Returns: Nothing 
/*---------------------------------------------------------------------------*/

Command(":RM",NULL);

/*---------------------------------------------------------------------------*/
// :RS# 
// Set Slew rate to max (fastest) 
// Returns: Nothing 
/*---------------------------------------------------------------------------*/

Command(":RS",NULL);

#endif



#endif
