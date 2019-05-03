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

#include <math.h>
#include <indicor/api.h>

#include "basiccmd.h"
#include "lx200.h"

static int
lx200_dec_sexa (char *out, double a, int w, int fracbase);

static int
lx200_ra_sexa (char *out, double a, int fracbase);

/*---------------------------------------------------------------------------*/

// Raw Command sent to telescope

RawCommand::RawCommand(LX200Simple* lx200) :
  LX200Command(lx200, "","RawCommand")
{
  rawCmd = STATIC_CAST(TextPropertyVector*, lx200->getDevice()->find("RAW_COMMAND"));
  assert(rawCmd != NULL);
}

/*---------------------------------------------------------------------------*/
void
RawCommand::request()
{
  setParameter(rawCmd->getValue("REQUEST"));
  LX200Command::request();
  rawCmd->busyStatus();
  rawCmd->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
RawCommand::handle(const void* data, int len)
{
  LX200Command::handle(data, len);
  busy = (buffer[buflen-1] != '#');
}

/*---------------------------------------------------------------------------*/

void
RawCommand::response()
{
  rawCmd->setValue("RESPONSE",buffer); 
  rawCmd->okStatus();
  rawCmd->indiSetProperty();
}

/*---------------------------------------------------------------------------*/

void
RawCommand::timeout()
{
  rawCmd->setValue("RESPONSE",buffer); // what we have so far ...
  LX200Command::timeout(rawCmd);
}

/*---------------------------------------------------------------------------*/
// :GR# 
// Get Telescope RA 
// Returns: HH:MM.T# or HH:MM:SS# 
// Depending which precision is set for the telescope 
/*---------------------------------------------------------------------------*/

GetRA::GetRA(LX200Simple* lx200) :
  LX200Command(lx200, ":GR","GetRA")
{

  eqCoords = STATIC_CAST(NumberPropertyVector*, lx200->getDevice()->find("EQUATORIAL_COORD"));
  assert(eqCoords != NULL);
}

/*---------------------------------------------------------------------------*/

bool 
GetRA::parseRA(float* num)
{
  /* is response complete ? */
  if(buffer[buflen-1] != '#')
    return(false);

  float hh, mm, ss = 0;
  int n = sscanf(buffer, "%f:%f:%f#", &hh, &mm, &ss);
  if(n != 3) {
    n = sscanf(buffer, "%f:%f#", &hh, &mm);
    if(n != 2) {
      log->error(IFUN,"Conversion de AR a numero imposible\n");
      return(false);
    }
  }
  *num = hh + mm/60 + ss/3600; // in case of 2, ss = 0
  return(true);
}

/*---------------------------------------------------------------------------*/
// :GD#
// Get Telescope Declination. 
// Returns: sDD*MM# or sDD*MM'SS# 
// Depending upon the current precision setting for the telescope. 
/*---------------------------------------------------------------------------*/

GetDEC::GetDEC(LX200Simple* lx200) :
  LX200Command(lx200,":GD","GetDEC")
{
  eqCoords = STATIC_CAST(NumberPropertyVector*, lx200->getDevice()->find("EQUATORIAL_COORD"));
  assert(eqCoords != NULL);
}

/*---------------------------------------------------------------------------*/

bool 
GetDEC::parseDEC(float* num)
{
  /* is response complete ? */
  if(buffer[buflen-1] != '#')
    return(false);

  float dd, mm, ss = 0;
  char dummy;
  int signum;

  int n = sscanf(buffer, "%f%c%f:%f#", &dd, &dummy, &mm, &ss);
  if(n != 4) {
    int n = sscanf(buffer, "%f%c%f#", &dd, &dummy, &mm);
    if(n != 3) {
      log->error(IFUN,"n = %d, Conversion de AR a numero imposible\n",n);
      return(false);
    }
  }

  signum = (dd <0) ? -1 : 1;
  dd = fabs(dd);
  *num = dd + mm/60 + ss/3600; // in case of 2, ss = 0
  *num = *num * signum;

  return(true);
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

SetTargetRA::SetTargetRA(LX200Simple* lx200) :
  LX200Command(lx200,":Sr","SetTargetRA")
{
  eqCoords = STATIC_CAST(NumberPropertyVector*, lx200->getDevice()->find("EQUATORIAL_COORD"));
  assert(eqCoords != NULL);
  coordFormat = STATIC_CAST(SwitchPropertyVector*, lx200->getDevice()->find("COORD_FORMAT"));
  assert(coordFormat != NULL);
}

/*---------------------------------------------------------------------------*/

void
SetTargetRA::request()
{
  int fracbase;

  if(coordFormat->getValue("LONG"))
    //    numberFormat(raStr, "%10.6m", teles->getTargetRA());
    fracbase = 3600;
  else
    //numberFormat(raStr, "%8.6m", teles->getTargetRA());
    fracbase = 600;

  ::lx200_ra_sexa(raStr, teles->getTargetRA(), fracbase);
  LX200Command::setParameter(raStr);
  LX200Command::request();
}

/*---------------------------------------------------------------------------*/

void
SetTargetRA::response()
{
  if(!accepted) {
    eqCoords->formatMsg("ERROR: No acepta la coordenada AR\n");
    eqCoords->indiMessage();
  } else {
    teles->updateRA();
  } 
}

/*---------------------------------------------------------------------------*/
// :SdsDD*MM# 
// Set target object declination to sDD*MM or sDD*MM:SS 
// depending on the current precision setting 
// Returns: 
// 1 - Dec Accepted 
// 0 - Dec invalid 
/*---------------------------------------------------------------------------*/

SetTargetDEC::SetTargetDEC(LX200Simple* lx200) :
  LX200Command(lx200,":Sd","SetTargetDEC")
{
  eqCoords = STATIC_CAST(NumberPropertyVector*, lx200->getDevice()->find("EQUATORIAL_COORD"));
  assert(eqCoords != NULL);
  coordFormat = STATIC_CAST(SwitchPropertyVector*, lx200->getDevice()->find("COORD_FORMAT"));
  assert(coordFormat != NULL);
}

/*---------------------------------------------------------------------------*/

void
SetTargetDEC::request()
{
  int fracbase;

  if(coordFormat->getValue("LONG"))
    //numberFormat(decStr, "%10.6m", teles->getTargetDEC());
    fracbase = 3600;
  else
    //numberFormat(decStr, "%8.6m", teles->getTargetDEC());
    fracbase = 60;

  // FALTA ESTO :
  // sustituir el primer : por un asterisco y el segundo por una coma !!!


  lx200_dec_sexa(decStr, teles->getTargetDEC(), 2, fracbase);
  LX200Command::setParameter(decStr);
  LX200Command::request();
}


/*---------------------------------------------------------------------------*/

void
SetTargetDEC::response()
{
  if(!accepted) {
    eqCoords->formatMsg("ERROR: No acepta la coordenada AR\n");
    eqCoords->indiMessage();
  } else {
    teles->updateRA();
  } 
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


SyncTelescope::SyncTelescope(LX200Simple* lx200) :
  LX200Command(lx200,":CM","SyncTelescope"), msg(0)
{
  eqCoords = STATIC_CAST(NumberPropertyVector*, lx200->getDevice()->find("EQUATORIAL_COORD"));
  assert(eqCoords != NULL);
}

/*---------------------------------------------------------------------------*/

void
SyncTelescope::timeout()
{ 
  LX200Command::timeout(eqCoords);
  if(msg) free(msg);
  msg = 0;
}

/*---------------------------------------------------------------------------*/

void
SyncTelescope::response()
{ 
  if(msg) {
    eqCoords->formatMsg("%s",msg);
    eqCoords->indiMessage();
    free(msg);
    msg = 0;
  }
}

/*---------------------------------------------------------------------------*/
// :U# 
// Toggle between low/hi precision positions 
// Low - RA displays and messages HH:MM.T sDD*MM 
// High - Dec/Az/El displays and messages HH:MM:SS sDD*MM:SS 
// Returns: Nothing 
/*---------------------------------------------------------------------------*/

TogglePrecision::TogglePrecision(LX200Simple* lx200) :
  LX200Command(lx200, ":U","TogglePrecision")
{
}

/*---------------------------------------------------------------------------*/
// :MS# 
// Slew to Target Object 
// Returns: 
// 0 Slew is Possible 
// 1<string># Object Below Horizon w/string message 
// 2<string># Object Below Higher w/string message 
/*---------------------------------------------------------------------------*/

SlewToTarget::SlewToTarget(LX200Simple* lx200) :
  LX200Command(lx200, ":MS","SlewToTarget")
{
  eqCoords = STATIC_CAST(NumberPropertyVector*, lx200->getDevice()->find("EQUATORIAL_COORD"));
  assert(eqCoords != NULL);
}


/*---------------------------------------------------------------------------*/

void
SlewToTarget::handle(const void* data, int len)
{
  LX200Command::handle(data, len);

  if(buflen == 0) {
    busy = true;
  } else if(buffer[0] == '0') {	// slew is possible
    msg = strndup(buffer, 1);
    busy = false;
  } else {			// slew not possible
    busy = ! parseMessage(&msg);
  }
}

/*---------------------------------------------------------------------------*/

void
SlewToTarget::timeout()
{ 
  LX200Command::timeout(eqCoords);
  if(msg) free(msg);
  msg = 0;
}

/*---------------------------------------------------------------------------*/

void
SlewToTarget::response()
{
  assert(msg != 0);
  if(msg[0] == '0') {
    teles->startSlewing();
  } else if(msg[0] == '1') { 
    eqCoords->formatMsg("ERROR: Objeto debajo de horizonte");
    eqCoords->indiMessage();
  } else if(msg[0] == '2') {
    eqCoords->formatMsg("ERROR: Objeto debajo de umbral preestablecido");
    eqCoords->indiMessage();
  } else {
    log->error(IFUN,"Algo gordo ha pasado: %s\n", buffer);
  }
  free(msg);
  msg = 0;
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

SlewComplete::SlewComplete(LX200Simple* lx200) :
  LX200Command(lx200, ":D", "SlewComplete")
{
  eqCoords = STATIC_CAST(NumberPropertyVector*, lx200->getDevice()->find("EQUATORIAL_COORD"));
  assert(eqCoords != NULL);
}

/*---------------------------------------------------------------------------*/
void
SlewComplete::handle(const void* data, int len)
{
  LX200Command::handle(data, len);
  busy = buffer[buflen-1] != '#';
}

/*---------------------------------------------------------------------------*/

void
SlewComplete::response()
{
  teles->slewComplete(buffer[0] == '#');
}


/*---------------------------------------------------------------------------*/
// :Q#
// Halt all current slewing 
// Returns:Nothing 
/*---------------------------------------------------------------------------*/

Abort::Abort(LX200Simple* lx200) :
  LX200Command(lx200, ":Q","Abort")
{ 
  abortMotion  = DYNAMIC_CAST(SwitchPropertyVector*, lx200->getDevice()->find("ABORT_MOTION"));
  assert(abortMotion != NULL);
}

/*---------------------------------------------------------------------------*/

void
Abort::request()
{
  LX200Command::request();
  abortMotion->okStatus();
  abortMotion->off("ABORT");
  abortMotion->indiSetProperty();
}

/*---------------------------------------------------------------------------*/
// :GVD# 
// Get Telescope Firmware Date 
// Returns: mmm dd yyyy# 
/*---------------------------------------------------------------------------*/

FirmwareDate::FirmwareDate(LX200Simple* lx200) :
  LX200Command(lx200,":GVD","FirmwareDate"), msg(0)
{ 
  mount = STATIC_CAST(TextPropertyVector*, lx200->getDevice()->find("MOUNT"));
  assert(mount != NULL);
}

/*---------------------------------------------------------------------------*/

void
FirmwareDate::response()
{
  mount->setValue("DATE",msg);
  free(msg);
  msg = 0;
}


/*---------------------------------------------------------------------------*/
// :GVT# 
// Get Telescope Firmware Time 
// returns: HH:MM:SS# 
/*---------------------------------------------------------------------------*/

FirmwareTime::FirmwareTime(LX200Simple* lx200) :
  LX200Command(lx200,":GVT","FirmwareTime"), msg(0)
{ 
  mount = STATIC_CAST(TextPropertyVector*, lx200->getDevice()->find("MOUNT"));
  assert(mount != NULL);
}

/*---------------------------------------------------------------------------*/

void
FirmwareTime::response()
{
  mount->setValue("TIME",msg);
  free(msg);
  msg = 0;
}


/*---------------------------------------------------------------------------*/
// :GVN# 
// Get Telescope Firmware Number 
// Returns: dd.d# 
/*---------------------------------------------------------------------------*/

FirmwareNumber::FirmwareNumber(LX200Simple* lx200) :
  LX200Command(lx200,":GVN","FirmwareNumber"), msg(0)
{ 
  mount = STATIC_CAST(TextPropertyVector*, lx200->getDevice()->find("MOUNT"));
  assert(mount != NULL);
}

/*---------------------------------------------------------------------------*/

void
FirmwareNumber::response()
{
  mount->setValue("PROGRAM",msg);
  free(msg);
  msg = 0;
}


/*---------------------------------------------------------------------------*/
// :GVP# 
// Get Telescope Product Name 
// Returns: <string># 
/*---------------------------------------------------------------------------*/

ProductName::ProductName(LX200Simple* lx200) :
  LX200Command(lx200,":GVP","ProductName"), msg(0)
{ 
  mount = STATIC_CAST(TextPropertyVector*, lx200->getDevice()->find("MOUNT"));
  assert(mount != NULL);
}

/*---------------------------------------------------------------------------*/

void
ProductName::response()
{
  mount->setValue("MODEL",msg);
  free(msg);
  msg = 0;
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MacroGetRaDec::MacroGetRaDec(LX200Simple* lx200) :
  MacroCommand(), log(0)
{
  add(new GetRA(lx200));
  add(new GetDEC(lx200));
  eqCoords = STATIC_CAST(NumberPropertyVector*, lx200->getDevice()->find("EQUATORIAL_COORD"));
  assert(eqCoords != NULL);
  log = LogFactory::instance()->forClass("MacroGetRaDec");
}

/*---------------------------------------------------------------------------*/

void
MacroGetRaDec::response()
{ 
  eqCoords->okStatus();
  eqCoords->indiSetProperty();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MacroGetRaDecSlew::MacroGetRaDecSlew(LX200Simple* lx200) :
  MacroCommand()
{
  add(new GetRA(lx200));
  add(new GetDEC(lx200));
  add(new SlewComplete(lx200));
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MacroSetRaDec::MacroSetRaDec(LX200Simple* lx200, const char* tag) :
  log(0)
{
  eqCoords = STATIC_CAST(NumberPropertyVector*, lx200->getDevice()->find("EQUATORIAL_COORD"));
  assert(eqCoords != NULL);
  log = LogFactory::instance()->forClass(tag);
}

/*---------------------------------------------------------------------------*/

void
MacroSetRaDec::request()
{
  eqCoords->busyStatus();
  eqCoords->indiSetProperty();
  MacroCommand::request();
}

/*---------------------------------------------------------------------------*/

void
MacroSetRaDec::response()
{
  eqCoords->okStatus();
  eqCoords->indiSetProperty();
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MacroSyncRaDec::MacroSyncRaDec(LX200Simple* lx200) :
  MacroSetRaDec(lx200, "MacroSyncRaDec")
{
  add(new SetTargetRA(lx200));
  add(new SetTargetDEC(lx200));
  add(new SyncTelescope(lx200));
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


MacroSlewToTarget::MacroSlewToTarget(LX200Simple* lx200) :
  MacroSetRaDec(lx200, "MacroSlewToTarget")
{
  add(new SetTargetRA(lx200));
  add(new SetTargetDEC(lx200));
  add(new SlewToTarget(lx200));
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MacroGetMount::MacroGetMount(LX200Simple* lx200) :
  MacroCommand(), teles(lx200)
{
  add(new ProductName(lx200));
  add(new FirmwareNumber(lx200));
  add(new FirmwareDate(lx200));
  add(new FirmwareTime(lx200));
  mount = STATIC_CAST(TextPropertyVector*, lx200->getDevice()->find("MOUNT"));
  assert(mount != NULL);
}

/*---------------------------------------------------------------------------*/

void
MacroGetMount::request()
{
  mount->busyStatus();
  mount->indiSetProperty();
  MacroCommand::request();
}

/*---------------------------------------------------------------------------*/

void
MacroGetMount::response()
{ 
  mount->okStatus();
  mount->indiSetProperty();
  teles->startPeriodicTask();
}

/*---------------------------------------------------------------------------*/

void
MacroGetMount::timeout()
{
  MacroCommand::timeout();
  mount->idleStatus();
  mount->setValue("MODEL","desconocido");
  mount->setValue("PROGRAM","desconocido");
  mount->setValue("DATE","desconocido");
  mount->setValue("TIME","desconocido");
  mount->indiSetProperty();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

TestRA::TestRA(LX200Simple* lx200) :
  LX200Command(lx200, ":GR","GetRA")
{
 coordFormat = STATIC_CAST(SwitchPropertyVector*, lx200->getDevice()->find("COORD_FORMAT"));
  assert(coordFormat != NULL);
}

/*---------------------------------------------------------------------------*/

bool 
TestRA::parseRA(float* num, int* n)
{
  /* is response complete ? */
  if(buffer[buflen-1] != '#')
    return(false);

  float hh = 0;
  float mm = 0;
  float ss = 0;

  *n = sscanf(buffer, "%f:%f:%f#", &hh, &mm, &ss);
  
  if(*n != 3) {
    *n = sscanf(buffer, "%f:%f#", &hh, &mm);
    if(*n != 2) {
      log->error(IFUN,"Conversion de AR a numero imposible\n");
      return(false);
    }
  }
  *num = hh + mm/60 + ss/3600; // in case of 2, ss = 0
  return(true);
}

/*---------------------------------------------------------------------------*/

void
TestRA::response()
{ 
  teles->testRA(ra, n);
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/* sprint the variable a in sexagesimal format into out[].
 * w is the number of spaces for the whole part.
 * fracbase is the number of pieces a whole is to broken into; valid options:
 *
 *	3600:	<w>:mm:ss
 *	600:	<w>:mm.m
 *
 * return number of characters written to out, not counting final '\0'.
 *
 * Valid for a >= 0 !
 */

static int
lx200_ra_sexa (char *out, double a, int fracbase)
{
  char *out0 = out;
  unsigned long n;
  int d;
  int f;
  int m;
  int s;
  
  assert(a >= 0);
  
  /* convert to an integral number of whole portions */
  n = (unsigned long)(a * fracbase + 0.5);
  d = n/fracbase;
  f = n%fracbase;
  
  out += sprintf (out, "%02d", d);

  assert(fracbase == 600 || fracbase == 3600);
  
  /* do the rest */
  switch (fracbase) {
  case 600:	/* HH:MM.T */
    out += sprintf (out, ":%02d.%1d", f/10, f%10);
    break;
  case 3600:	/* HH:MM:SS */
    m = f/(fracbase/60);
    s = f%(fracbase/60);
    out += sprintf (out, ":%02d:%02d", m, s);
    break;
  default:
    break;
  }
  
  return (out - out0);
}

/* sprint the variable a in sexagesimal format into out[].
 * w is the number of spaces for the whole part.
 * fracbase is the number of pieces a whole is to broken into; valid options:
 *	3600:	<w>:mm:ss
 *	60:	<w>:mm
 * return number of characters written to out, not counting final '\0'.
 */

/*---------------------------------------------------------------------------*/

static int
lx200_dec_sexa (char *out, double a, int w, int fracbase)
{
  char *out0 = out;
  unsigned long n;
  int d;
  int f;
  int m;
  int s;
  int isneg;
  
  /* save whether it's negative but do all the rest with a positive */
  isneg = (a < 0);
  if (isneg)
    a = -a;
  
  /* convert to an integral number of whole portions */
  n = (unsigned long)(a * fracbase + 0.5);
  d = n/fracbase;
  f = n%fracbase;
  
  /* form the whole part; "negative 0" is a special case */
  if (isneg && d == 0)
    out += sprintf (out, "-%0*d", w, 0);
  else
    out += sprintf (out, "%+0*d", w, isneg ? -d : d);

  assert(fracbase == 60 || fracbase == 3600);
  
  /* do the rest */
  switch (fracbase) { 
  case 60:	/* sDD*MM */
    m = f/(fracbase/60);
    out += sprintf (out, "*%02d", m);
    break;
  case 3600:	/* sDD*MM:SS */
    m = f/(fracbase/60);
    s = f%(fracbase/60);
    out += sprintf (out, "*%02d:%02d", m, s);
    break;
  default:
    break;
  }
  
  return (out - out0);
}
