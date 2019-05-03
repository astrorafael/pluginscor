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

#ifndef LX200_SIMPLE_H
#define LX200_SIMPLE_H


/*******************************/
/* THE PLUGIN FACTORY FUNCTION */
/*******************************/

#include <indicor/api.h>

BEGIN_C_DECLS

Plugin* CreatePlugin(Device* dev,const char* args);

END_C_DECLS

/* Simple LX200 driver */

class LX200Simple : public PluginBase {

 public:
  
  /* tolerances for slew complete */
  static const double EPSILON_RA;
  static const double EPSILON_DEC;
  
  LX200Simple(Device* dev, unsigned int perif);
  ~LX200Simple() {}

  
  /* ****************** */
  /* THE INDI INTERFACE */
  /* ****************** */

  virtual void 
    init();

  virtual void 
    update(TextPropertyVector* pv, char* name[], char* text[], int n);
  
  virtual void 
    update(NumberPropertyVector* pv, char* name[], double num[], int n);
    
  virtual void 
    update(SwitchPropertyVector* pv, char* name, ISState swit);
    
   
  /*****************************************/
  /* events comming from other observables */
  /*****************************************/

  virtual void
    update(PropertyVector* pv, ITopic t);
 
  /*************************/
  /* The UDP event handler */
  /*************************/

  virtual void 
    handle(unsigned int event, const void* data, int len);

 virtual void 
   sendMessage(Outgoing_Message* msg, int len);
  
  /**************************************/
  /* Timer events from the global timer */
  /**************************************/

  virtual void 
    tick();

  /***************************************/
  /* interface for LX200 command objects */
  /***************************************/

  double getTargetRA() const;	/* called when formatting a command */
  double getTargetDEC() const;	/* called when formatting a command */
  void   updateRA();		/* called when a RA  value is received */
  void   updateDEC();		/* called when a DEC value is received */
  void   startSlewing();
  void   slewComplete(bool flag);
  void   startPeriodicTask();	/* after initialization commands */
  void   testRA(double ra, int nconv); /* testing log/short format */
  void   getMountInfo();	/* starts the process of obtainin mount info */
  bool   isLongFormat();	/* format of coordinates */

 private:

  /* commands used to talk to this telescope model */

  Command* syncRaDec;
  Command* slewToTarget;
  Command* getRaDec;		/* polls (RA, DEC) */
  Command* getRaDecSlew;	/* polls (RA,DEC) while Slewing */
  Command* curMacroRaDec;	/* either one of the two above */
  Command* rawCmd;
  Command* abortCmd;
  Command* getMount;
  Command* testRACmd;		/* test for short/long format */
  Command* togglePrec;		/* may be used in test for short/long format */

  /*********************************/
  /* THE USER INTERFACE PROPERTIES */
  /*********************************/
  
  NumberPropertyVector* eqCoords;
  NumberPropertyVector* optics;
  TextPropertyVector*   target;
  TextPropertyVector*   edbLine;
  TextPropertyVector*   fitsTextData;
  TextPropertyVector*   mount;
  TextPropertyVector*   rawCommand;
  SwitchPropertyVector* configuration;
  SwitchPropertyVector* abortMotion;
  SwitchPropertyVector* movement;
  SwitchPropertyVector* onCoordSet;
  SwitchPropertyVector* slewRate;
  SwitchPropertyVector* coordFormat;

  /**************************/
  /* Other internal objects */
  /**************************/

  /* auxiliar state variables */

  double targetRA;		/* while reaching position */
  double targetDEC;

  double testedRA;		/* state variables for the long/short format */
  int nconv;			/* change process ... */

  unsigned int timeoutCount;
  bool corDisconnected;		/* tracks COR disconnection */
  unsigned int perifNum;	/* serial port number in COR */

  /* ************** */
  /* HELPER METHODS */
  /* ************** */

  void unknown(PropertyVector* pv);

  void updateTarget(char* name[], char* text[], int n);

  void updateRawCommand(char* name[], char* text[], int n);

  void updateEdbLine(char* name[], char* text[], int n);

  void updateFITSData(char* name[], char* text[], int n);

  void save(char* name, ISState swit);

  void toggleFormat(char* name, ISState swit);

  bool parseEDBLine(const char* line, char* objname, double* ra, double* dec);

  /* starts the chain of command queries/responses */
  void startFormatProcess();

};

/*---------------------------------------------------------------------------*/

inline double
LX200Simple::getTargetRA() const
{
  return(targetRA);
}

/*---------------------------------------------------------------------------*/

inline double
LX200Simple::getTargetDEC() const
{
  return(targetDEC);
}

/*---------------------------------------------------------------------------*/

inline void 
LX200Simple::updateRA()
{
  eqCoords->setValue("RA",targetRA);
}

/*---------------------------------------------------------------------------*/

inline void 
LX200Simple::updateDEC()
{
  eqCoords->setValue("DEC",targetDEC);
}

/*---------------------------------------------------------------------------*/

inline void
LX200Simple::startPeriodicTask()
{
  curMacroRaDec = getRaDec;
  hubTimer->add(this);
}

/*---------------------------------------------------------------------------*/

inline void 
LX200Simple::getMountInfo()
{
  queue->add(getMount);
}

/*---------------------------------------------------------------------------*/

inline void 
LX200Simple::startFormatProcess()
{
  nconv = 0;
  queue->add(testRACmd);
}

#endif
