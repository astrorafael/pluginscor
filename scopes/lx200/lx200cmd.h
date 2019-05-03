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

#ifndef LX200_COMMANDS_H
#define LX200_COMMANDS_H

#include <string>		/* STL's string */

#include <indicor/api.h>

/*****************/
/* LX200 COMMAND */
/*****************/

class LX200Simple;


class LX200Command : public Command
{

 public:

  /* constructors and destructor */

  LX200Command( LX200Simple* teles,  const char* pfx, const char* tag);
  virtual ~LX200Command();

  /* executes the command request.  */  
  virtual void request();

  /* analyzes data comming from COR and processes it */
  /* can be called one or several times */
  /* appends new data to the existing buffer */
  virtual void handle(const void* data, int len);

  /* execute actions following a complete reception of data */
  virtual void response() = 0;

  /* command timeout while waiting for a complete response */
  virtual void timeout() = 0;	

  /* true if command is waiting for a complete response*/
  /* there may be commands that do not need it at all */
  /* a default implementation is provided */
  virtual bool isBusy() const;
  
  /* sets a new timeout in milliseconds */
  virtual void setTimeout(unsigned int value);

  /* to send an optional parameter to the command */
  void setParameter(const char* p);

 protected:

  static Outgoing_Message msg;		/* message to COR */
  static const unsigned int MAXBUF = 128;
  static char buffer[MAXBUF];   /* static buffer to accumulate response data */
  static unsigned int buflen;	/* current buffer size */

  Log* log;			/* log object */
  LX200Simple* teles;		/* where commands act upon */
  const char* prefix;		/* command's prefix */
  const char* parameter;	/* optional parameter string */
  bool busy;			/* busy flag for use in subclasses */

  /******************/
  /* HELPER METHODS */
  /******************/

  /* sends a timeout message and puts property in Alert state */
  void timeout(PropertyVector* pv);

  /**************************************/
  /* parsers common to various commands */
  /* returns true if parsing is OK      */
  /**************************************/

  /* parses a single 0|1 response */
  bool parseBool(bool* flag);		

  /* parses a message. string returned must be free'd by caller */
  bool parseMessage(char** msg);

 private:

  void newLog(const char* tag);

};

/*---------------------------------------------------------------------------*/

inline
LX200Command::~LX200Command()
{
  delete log;
}

/*---------------------------------------------------------------------------*/

inline bool
LX200Command::isBusy() const
{  
  return(busy);
  // return(buffer[buflen-1] != '#');
}

/*---------------------------------------------------------------------------*/

inline void
LX200Command::setTimeout(unsigned int value)
{
}

/*---------------------------------------------------------------------------*/

inline void
LX200Command::setParameter(const char* p)
{
  parameter = p;
}

/*---------------------------------------------------------------------------*/

#endif

#if 0

/*
 * Meade Telescope Serial Command Protocol 
 *
 * Revision L 9 October 2002 
 */

// ACK <0x06>
// Query of alignment mounting mode.
// Returns: 
// A If scope in AltAz Mode 
// L If scope in Land Mode 
// P If scope in Polar Mode 


/***************************/
/* A - Alignment Commands  */
/***************************/

// :Aa#
// Start Telescope Automatic Alignment Sequence 
// [LX200GPS only]
// Return: 
// 1: When complete (can take several minutes). 
// 0: If scope not AzEl Mounted or align fails 


// :AL#
// Sets telescope to Land Alignment Mode
// Return: Nothing 


// :AP#
// Sets telescope to Polar Alignment mode
// Return: Nothing 


// :AA#
// Sets telescope the AltAz alignment mode
// Return: Nothing 


/************************************/
/* $B Active Backlash Compensation  */
/************************************/

// :$BAdd#
// Set Altitude/Dec Antibacklash
// Return: Nothing 


// :$BZdd#
// Set Azimuth/RA Antibacklash
// Return: Nothing 


/***********************************/
/* B - Reticule/Accessory Control  */
/***********************************/

// :B+#
// Increase reticule Brightness 
// Return: Nothing 


// :B-#
// Decrease Reticule Brightness 
// Return: Nothing 


// :Bn#
//Set Reticle flash rate to <n> (an ASCII expressed number) 
// <n> Values of 0..3 for LX200 series 
// <n> Values of 0..9 for Autostar and LX200GPSa
// Return: Nothing 


// :BDn#
// Set Reticule Duty flash duty cycle to <n> (an ASCII expressed digit)
// [LX200 GPS Only] 
// <n> Values: 0 = On, 1..15 flash rate 
// Return: Nothing


/********************/
/* C - Sync Control */
/********************/

// :CL#
// Synchonize the telescope with the current Selenographic coordinates. 
// Return: Nothing ?????


// :CM#
// Synchronizes the telescope's position with the currently selected
// database object's coordinates. 
// Returns: 
// LX200's - a "#" terminated string with the name of 
//           the object that was synced. 
// Autostars & LX200GPS - At static string: " M31 EX GAL MAG 3.5 SZ178.0'#" 



/*********************/
/* D - Distance Bars */
/*********************/

// :D#
// Requests a string of bars indicating the distance to the current
// library object. 
// Returns: 
// LX200's - a string of bar characters indicating the distance. 
// Autostars and LX200GPS - a string containing one bar until a 
// slew is complete, then a null string is returned ( # char only). 


/*******************/
/* f - Fan Command */
/*******************/

// :f+#
// LX200 16" - Turn on the tube exhaust fan 
// LX200GPS - Turn on power to accessor panel 
// Autostar & LX200 < 16" Not Supported 
// Returns: nothing 


// :f-#
// LX200 16" - Turn off the tube exhaust fan 
// LX200GPS - Turn off power to accessor panel 
// Autostar & LX200 < 16" Not Supported 
// Returns: nothing 


// :fT#
// LX200GPS Return Optical Tube Assembly Temperature 
// Returns <sdd.ddd># - a '#' terminated signed ASCII real number 
// indicating the Celsius ambient temperature. 
// All others -  Not supported 

 
/***********************/
/* F - Focuser Control */
/***********************/

// :F+#
// Start Focuser moving inward (toward objective) 
// Returns: None 


// :F-#
// Start Focuser moving outward (away from objective) 
// Returns: None 


// :FQ#
// Halt Focuser Motion 
// Returns: Nothing 


// :FF#
// Set Focus speed to fastest setting 
// Returns: Nothing 


// :FS#
//Set Focus speed to slowest setting 
//Returns: Nothing 


// :Fn#
// Autostar & LX200GPS  set focuser speed to <n> 
// where <n> is an ASCII digit 1..4 
// Returns: Nothing 
// LX200 Not Supported 


/*********************************/
/* g - GPS/Magnetometer commands */
/*********************************/

// :g+#
// LX200GPS Only - Turn on GPS 
// Returns: Nothing 


// :g-#
// LX200GPS Only - Turn off GPS 
// Returns: Nothing 


// :gps#
// LX200GPS Only - Turns on NMEA GPS data stream. 
// Returns: The next string from the GPS in standard NEMA format 
// followed by a '#' key 


// :gT#
// Powers up the GPS and updates the system time from the GPS stream. 
// The process my take several minutes to complete. 
// During GPS update, normal handbox operations are interrupted. 
// [LX200gps only] 
// Returns: '0'  In the event that the user interrupts the process, 
// or the GPS times out. 
// Returns: '1' After successful updates 


/**********************************/
/* G - Get Telescope Information  */
/**********************************/

// :G0#
// Get Alignment Menu Entry 0 
// Returns: A '#' Terminated ASCII string. [LX200 legacy command] 


// :G1#
// Get Alignment Menu Entry 1 
// Returns: A '#' Terminated ASCII string. [LX200 legacy command] 


// :G2#
// Get Alignment Menu Entry 2 
// Returns: A '#' Terminated ASCII string. [LX200 legacy command] 


// :GA#
// Get Telescope Altitude 
// Returns: sDD*MM# or sDD*MM'SS# 
// The current scope altitude. The returned format depending on 
// the current precision setting. 


// :Ga#
// Get Local Telescope Time In 12 Hour Format 
// Returns: HH:MM:SS# 
// The time in 12 format 


// :Gb#
// Get Browse Brighter Magnitude Limit 
// Returns: sMM.M# 
// The magnitude of the faintest object to be returned from the telescope 
// FIND/BROWSE command. 
// Command when searching for objects in the Deep Sky database. 


// :GC#
// Get current date.  
// Returns: MM/DD/YY# 
// The current local calendar date for the telescope. 


// :Gc#
// Get Calendar Format 
// Returns: 12# or 24# 
// Depending on the current telescope format setting.


// :GD#
// Get Telescope Declination. 
// Returns: sDD*MM# or sDD*MM'SS# 
// Depending upon the current precision setting for the telescope. 


// :Gd#
// Get Currently Selected Object/Target Declination 
// Returns: sDD*MM# or sDD*MM'SS# 
// Depending upon the current precision setting for the telescope. 


// :GF#
// Get Find Field Diameter 
// Returns: NNN# 
// An ASCIi interger expressing the diameter of the field search 
// used in the IDENTIFY/FIND commands. 


// :Gf#
// Get Browse Faint Magnitude Limit 
// Returns: sMM.M# 
// The magnitude or the birghtest object to be returned from the 
// telescope FIND/BROWSE command. 


// :GG# 
// Get UTC offset time 
// Returns: sHH# or sHH.H# 
// The number of decimal hours to add to local time to convert it to UTC.
// If the number is a whole number the 
// sHH# form is returned, otherwise the longer form is return. 
// On Autostar and LX200GPS, the daylight savings 
// setting in effect is factored into returned value. 


// :Gg# 
// Get Current Site Longitude 
// Returns: sDDD*MM# 
// The current site Longitude. East Longitudes are expressed as negative 


// :Gh# 
// Get High Limit 
// Returns: sDD* 
// The minimum elevation of an object above the horizon to which 
// the telescope will slew with reporting a 'Below Horizon' error. 



// :GL# 
// Get Local Time in 24 hour format 
// Returns: HH:MM:SS# 
// The Local Time in 24-hour Format 


// :Gl# 
// Get Larger Size Limit 
// Returns: NNN'# 
// The size of the smallest object to be returned by a search of 
// the telescope using the BROWSE/FIND commands. 


// :GM# 
// Get Site 1 Name 
// Returns: <string># 
// A '#' terminated string with the name of the requested site. 


// :GN# 
// Get Site 2 Name 
// Returns: <string># 
// A '#' terminated string with the name of the requested site. 


// :GO# 
// Get Site 3 Name 
// Returns: <string># 
// A '#' terminated string with the name of the requested site. 


// :GP# 
// Get Site 4 Name 
// Returns: <string># 
// A '#' terminated string with the name of the requested site. 


// :Go# 
// Get Lower Limit 
// Returns: DD*# 
// The highest elevation above the horizon that the telescope will be
// allowed to slew to without a warning message. 


// :Gq# 
// Get Minimum Quality For Find Operation 
// Returns: 
// SU# Super 
// EX# Excellent 
// VG# Very Good 
// GD# Good 
// FR# Fair 
// PR# Poor 
// VP# Very Poor 
// The mimum quality of object returned by the FIND command. 


// :GR# 
// Get Telescope RA 
// Returns: HH:MM.T# or HH:MM:SS# 
// Depending which precision is set for the telescope 


// :Gr# 
// Get current/target object RA 
// Returns: HH:MM.T# or HH:MM:SS 
// Depending upon which precision is set for the telescope 


// :GS# 
// Get the Sidereal Time 
// Returns: HH:MM:SS# 
// The Sidereal Time as an ASCII Sexidecimal value in 24 hour format 


// :Gs# 
// Get Smaller Size Limit 
// Returns: NNN'# 
// The size of the largest object returned by the FIND command 
// expressed in arcminutes. 


// :GT# 
// Get tracking rate 
// Returns: TT.T# 
// Current Track Frequency expressed in hertz assuming a synchonous motor
// design where a 60.0 Hz motor clock would produce 1 revolution of the 
// telescope in 24 hours. 


// :Gt# 
// Get Current Site Latitdue 
// Returns: sDD*MM# 
// The latitude of the current site. Positive inplies North latitude. 


// :GVD# 
// Get Telescope Firmware Date 
// Returns: mmm dd yyyy# 


// :GVN# 
// Get Telescope Firmware Number 
// Returns: dd.d# 


// :GVP# 
// Get Telescope Product Name 
// Returns: <string># 


// :GVT# 
// Get Telescope Firmware Time 
// returns: HH:MM:SS# 


// :Gy# 
// Get deepsky object search string 
// Returns: GPDCO# 
// A string indicaing the class of objects that should be returned 
// by the FIND/BROWSE command. If the character is upper case,
// the object class is return. If the character is lowercase,
// objects of this class are ignored. The character meanings are as follows: 
// G - Galaxies 
// P - Planetary Nebulas 
// D - Diffuse Nebulas 
// C - Globular Clusters 
// O - Open Clusters 



// :GZ# 
// Get telescope azimuth 
// Returns: DDD*MM#T or DDD*MM'SS# 
// The current telescope Azimuth depending on the selected precision.
// (astrorafael) DOCUMENTATION IS WRONG !!!!!



/****************************/
/* - Home Position Commands */
/****************************/

// :hS# 
// LX200GPS and LX 16" Seeks Home Position and stores the encoder values 
// from the aligned telescope at the home position in the nonvolatile memory 
// of the scope. 
// Returns: Nothing 
// Autostar,LX200 - Ignored 


// :hF# 
// LX200GPS and LX 16" Seeks the Home Position of the scope and 
// sets/aligns the scope based on the encoder values stored in 
// non-volatile memory
// Returns: Nothing 
// Autostar,LX200 - Igrnored 


// :hN# 
// LX200GPS only: Sleep Telescope. Power off motors, encoders, 
// displays and lights. Scope remains in minimum power mode until a
// keystroke is received or a wake command is sent. 


// :hP# 
// Autostar, LX200GPS and LX 16" Slew to Park Position 
// Returns: Nothing 


// :hW# 
// LX200 GPS Only: Wake up sleeping telescope. 
// Returns: Nothing 


// :h? 
// Autostar, LX200GPS and LX 16" Query Home Status
// LX200 Not Supported 
// Returns: 
// 0 - Home Search Failed 
// 1 - Home Search Found 
// 2 - Home Search in Progress 



/***************************/
/* H - Time Format Command */
/***************************/

// :H# 
// Toggle Between 24 and 12 hour time format 
// Returns: Nothing 


/************************************/
/* I - Initialize Telescope Command */
/************************************/

// :I# 
// Causes the telescope to cease current operations 
// and restart at its power on initialization. 
// [LX200 GPS Only]
// Returns: Nothing 


/*******************************/
/* L - Object Library Commands */
/*******************************/

/* NOTE: This command series is not useful to me so I just skip it */

/***********************************/
/* M - Telescope Movement Commands */
/***********************************/

// :MA# 
// Slew to target Alt and Az 
// Autostar, LX 16"", LX200GPS 
// LX200 - Not supported 
// Returns: 
// 0 - No fault 
// 1 - Fault 


// :Me#
//  Move Telescope East at current slew rate 
// Returns: Nothing 


// :Mn# 
// Move Telescope North at current slew rate 
// Returns: Nothing 


// :Ms#  
// Move Telescope South at current slew rate 
// Returns: Nothing 


// :Mw# 
// Move Telescope West at current slew rate 
// Returns: Nothing 


// :MS# 
// Slew to Target Object 
// Returns: 
// 0 Slew is Possible 
// 1<string># Object Below Horizon w/string message 
// 2<string># Object Below Higher w/string message 


/*****************************/
/* P - High Precision Toggle */
/*****************************/

// :P#
// Toggles High Precsion Pointing. When High precision pointing 
// is enabled scope will first allow the operator to center a nearby 
// bright star before moving to the actual taget. 
// Returns: <string> 
//    HIGH PRECISION    - Current setting after this command. 
//    LOW PRECISION     - Current setting after this command. 


/****************************/
/* $Q - Smart Drive Control */
/****************************/

// $Q#
// Toggles Smart Drive PEC on and off for both axis 
// Returns: Nothing 
// Not supported on Autostar 
// BUG IN IN THIS INFO ?????



// :$QA+#
// Enable Dec/Alt PEC [LX200gps only] 
// Returns: Nothing 


// :$QA-#
// Disable Dec/Alt PEC [LX200gps only] 
// Returns: Nothing 


// :$QZ+#
// Enable RA/AZ PEC compensation [LX200gps only] 
// Returns: Nothing 


// :$QZ-#
// Disable RA/AZ PEC Compensation [LX200gpgs only] 
// Return: Nothing 


/*************************/
/* Q - Movement Commands */
/*************************/

// :Q#
// Halt all current slewing 
// Returns:Nothing 


// :Qe# 
// Halt eastward Slews 
// Returns: Nothing


// :Qn# 
// Halt northward Slews 
// Returns: Nothing


// :Qs#   
// Halt southward Slews 
// Returns: Nothing


// :Qw#  
// Halt westward Slews 
// Returns: Nothing


/********************************/
/* r - Field Derotator Commands */
/********************************/


// :r+#   
// Turn on Field Derotator 
// [LX 16" and LX200GPS] 
// Returns: Nothing 


// :r-#  
// Turn off Field Derotator, halt slew in progress.
// [Lx 16" and LX200GPS] 
// Returns Nothing 


/**************************/
/* R - Slew Rate Commands */
/**************************/

// :RC# 
// Set Slew rate to Centering rate (2nd slowest) 
// Returns: Nothing 


// :RG# 
// Set Slew rate to Guiding Rate (slowest) 
// Returns: Nothing 


// :RM# 
// Set Slew rate to Find Rate (2nd Fastest) 
// Returns: Nothing 


// :RS# 
// Set Slew rate to max (fastest) 
// Returns: Nothing 


// :RADD.D# 
// Set RA/Azimuth Slew rate to DD.D degrees per second 
// [LX200GPS Only] 
// Returns: Nothing 


// :REDD.D# 
// Set Dec/Elevation Slew rate to DD.D degrees per second 
// [ LX200GPS only] 
// Returns: Nothing 


// :RgSS.S# 
// Set guide rate to +/- SS.S to arc seconds per second. 
// This rate is added to or subtracted from the current tracking 
// Rates when the CCD guider or handbox guider buttons are pressed 
// when the guide rate is selected. Rate shall not exceed 
// sidereal speed (approx 15.0417"/sec)
// [ LX200GPS only] 
// Returns: Nothing 


/******************************/
/* S - Telescope Set Commands */
/******************************/

// :SasDD*MM# 
// Set target object altitude to sDD*MM# or sDD*MM'SS# 
// [LX 16", Autostar, LX200GPS] 
// Returns: 
// 0 - Object within slew range 
// 1 - Object out of slew range 


// :SbsMM.M# 
// Set Brighter limit to the ASCII decimal magnitude string. sMM.M 
// Returns: 
// 0 - Valid 
// 1 - invalid number 


// :SBn# 
// Set Baud Rate n, where n is an ASCII digit (1..9) with the following
// interpertation 
// 1 56.7K 
// 2 38.4K 
// 3 28.8K 
// 4 19.2K 
// 5 14.4K 
// 6 9600 
// 7 4800 
// 8 2400 
// 9 1200 
// Returns: 
// 1 At the current baud rate and then changes to the new rate 
// for further communication 


// :SCMM/DD/YY# 
// Change Handbox Date to MM/DD/YY 
// Returns: <D><string> 
// D = 0 if the date is invalid. The string is the null string. 
// D = 1 for valid dates and the string is "Updating Planetary Data#"
// Note: For LX200GPS this is the UTC data! 


// :SdsDD*MM# 
// Set target object declination to sDD*MM or sDD*MM:SS 
// depending on the current precision setting 
// Returns: 
// 1 - Dec Accepted 
// 0 - Dec invalid 


// :SEsDD*MM# 
// Sets target object to the specificed selenographic latitude on the Moon. 
// Returns:
// 1 - If moon is up and coordinates are accepted. 
// 0 - If the coordinates are invalid 


// :SesDDD*MM# 
// Sets the target object to the specified selenogrphic longitude on the Moon 
// Returns:
// 1 - If the Moon is up and coordinates are accepted. 
// 0 - If the coordinates are invalid for any reason. 


// :SfsMM.M# 
// Set faint magnitude limit to sMM.M 
// Returns: 
// 0 - Invalid 
// 1 - Valid 


// :SFNNN# 
// Set FIELD/IDENTIFY field diamter to NNNN arc minutes. 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SgDDD*MM# 
// Set current siteâs longitude to DDD*MM an ASCII position string
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SGsHH.H# 
// Set the number of hours added to local time to yield UTC 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :ShDD# 
// Set the minimum object elevation limit to DD# 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SlNNN# 
// Set the size of the smallest object returned by FIND/BROWSE 
// to NNNN arc minutes 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SLHH:MM:SS# 
// Set the local Time 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SM<string># 
// Set site 1's name to be <string>. LX200s only accept 3 character strings.
// Other scopes accept up to 15 characters. 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SN<string># 
// Set site 2's name to be <string>. LX200s only accept 3 character strings.
// Other scopes accept up to 15 characters. 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SO<string># 
// Set site 3's name to be <string>. LX200s only accept 3 character strings. 
// Other scopes accept up to 15 characters. 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SP<string># 
// Set site 4's name to be <string>. LX200s only accept 3 character strings. 
// Other scopes accept up to 15 characters. 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SoDD*# 
// Set highest elevation to which the telescope will slew 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :Sq# 
// Step the quality of limit used in FIND/BROWSE through its cycle of VP...SU. 
// Current setting can be queried with :Gq# 
// Returns: Nothing 


// :SrHH:MM.T# 
// :SrHH:MM:SS# 
// Set target object RA to HH:MM.T or HH:MM:SS depending on
// the current precision setting. 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SsNNN# 
// Set the size of the largest object the FIND/BROWSE command 
// will return to NNNN arc minutes 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SSHH:MM:SS# 
// Sets the local sideral time to HH:MM:SS 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :StsDD*MM# 
// Sets the current site latitdue to sDD*MM# 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :STTT.T# 
// Sets the current tracking rate to TTT.T hertz, 
// assuming a model where a 60.0 Hertz synchronous motor will cause the RA 
// axis to make exactly one revolution in 24 hours. 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SwN# 
// Set maximum slew rate to N degrees per second. N is the range (2..8) 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SyGPDCO# 
// Sets the object selection string used by the FIND/BROWSE command. 
// Returns: 
// 0 - Invalid
// 1 - Valid


// :SzDDD*MM# 
// Sets the target Object Azimuth 
// [LX 16" and LX200GPS only] 
// Returns: 
// 0 - Invalid
// 1 - Valid


/*************************/
/* T - Tracking Commands */
/*************************/

// :T+# 
// Increment Manual rate by 0.1 Hz 
// Returns: Nothing 


// :T-# 
// Decrement Manual rate by 0.1 Hz 
// Returns: Nothing 


// :TL# 
// Set Lunar Tracking Rate 
// Returns: Nothing 


// :TM# 
// Select custom tracking rate 
// Returns: Nothing 


// :TQ# 
// Select default tracking rate 
// Returns: Nothing 


// :TDDD.DDD# 
// Set Manual rate do the ASCII expressed decimal DDD.DD 
// Returns: 1 


/************************/
/* U - Precision Toggle */
/************************/

// :U# 
// Toggle between low/hi precision positions 
// Low - RA displays and messages HH:MM.T sDD*MM 
// High - Dec/Az/El displays and messages HH:MM:SS sDD*MM:SS 
// Returns: Nothing 


/*******************/
/* W - Site Select */
/*******************/

// :W<n># 
// Set current site to <n>, an ASCII digit in the range 0..3 
// Returns: Nothing 


#endif





