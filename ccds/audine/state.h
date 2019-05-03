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

#ifndef AUDINE_STATE_H
#define AUDINE_STATE_H

#include "audine.h"

/*******************************************/
/* THE STATE PATTERN APPLIED TO AUDINE CCD */
/*******************************************/

/* As the Audine camera has several states and events, 
 * it is recomended to adopt a structred approach that
 * dos not mix several actions depending on the state
 * in a given event handler code.
 *
 * The mapping of device states to supervisory states
 * shown by INDI is as follows:
 *  
 *  AudineIdle     => IPS_IDLE  (grey)
 *  AudineOk       => IPS_OK    (green)
 *  AudineWait     => IPS_BUSY  (yellow)
 *  AudineExp      => IPS_BUSY  (yelow)
 *  AudineRead     => IPS_BUSY  (yellow)
 *  AudineAlert    => IPS_ALERT (red) 
 *
 */ 

class AudineState {

 public:
  
  AudineState(const char* logtag);
  virtual ~AudineState() {delete log;}
  
  virtual const char* getName() = 0;

  /* state change method */

  void nextState(Audine* ccd, AudineState* st) {
    st->setVisualState(ccd);	/* lights to proper color for next state */
    ccd->nextState(st);
  }

  /*****************************************/
  /* events coming from the user interface */
  /*****************************************/

#if 0				/* not yet implemented */
  virtual void 
    update(Audine* ccd, BLOBPropertyVector* pv, char* name[], char* blob[], int n) {
    forbidden(pv);
  }
#endif

  virtual void 
    update(Audine* ccd, TextPropertyVector* pv, char* name[], char* text[], int n) {
    forbidden(pv);
  }
  
  virtual void 
    update(Audine* ccd, SwitchPropertyVector* pv, char* name, ISState swit) {
    forbidden(pv);
  }
  
  virtual void 
    update(Audine* ccd, NumberPropertyVector* pv, char* name[], double num[], int n) {
    forbidden(pv);
  }
  
  /**************************/
  /* events coming from COR */
  /**************************/

  virtual void 
    handle(Audine* ccd, unsigned int event, const void* data, int len);

  /******************************************/
  /* events comming from other observables) */
  /******************************************/

  virtual void
    update(Audine* ccd, PropertyVector* pv, ITopic topic);

  /******************/
  /* timeout events */
  /******************/

  virtual void tick(Audine* ccd);	/* exposure down counter */
  virtual void timeout(Audine* ccd);	/* safety watchdog */

 protected:

  Log* log;

  /* senda a forbidden message to GUI */
  void forbidden(PropertyVector* pv);

  /* sets visual lights in CCD according to state */
  virtual void setVisualState(Audine* ccd) = 0;

  /* common observer update to states Ok, Wait and Exp  */
  void defaultUpd(Audine* ccd, PropertyVector* pvorig, ITopic topic);

};


/*************************/
/* THE AUDINE IDLE STATE */
/*************************/

class AudineIdle : public AudineState {

 public:
  
  virtual ~AudineIdle() {}

  static AudineState* instance();
  

  virtual const char* getName() { return ("Desconectado"); }

   
  /**************************/
  /* events coming from COR */
  /**************************/

#if 0
  virtual void 
    handle(Audine* ccd, unsigned int event, const void* data, int len);
#endif

  /*****************************************************/
  /* events comming from other Plugin (observables) */
  /*****************************************************/

  virtual void
    update(Audine* ccd, PropertyVector* pv, ITopic t);

  /*****************************************/
  /* events coming from the user interface */
  /*****************************************/

  virtual void 
    update(Audine* ccd, SwitchPropertyVector* pv,
	   char* name, ISState swit);

  virtual void 
    update(Audine* ccd, NumberPropertyVector* pv, char* name[], double num[], int n);
 
  virtual void 
    update(Audine* ccd, TextPropertyVector* pv, char* name[], char* text[], int n);

 private:

  AudineIdle() : AudineState("AudineIdle") {}
  static AudineState* _instance;

  virtual void setVisualState(Audine* ccd);

};


/***********************/
/* THE AUDINE OK STATE */
/***********************/

class AudineOk : public AudineState {

 public:
  
  virtual ~AudineOk() {}

  static AudineState* instance();

  virtual const char* getName() { return ("En reposo"); }
  
  /*****************************************/
  /* events coming from the user interface */
  /*****************************************/

#if 0				/* not yet implemented */
  virtual void 
    update(Audine* ccd, BLOBPropertyVector* pv, char* name[], char* blob[], int n);
#endif


  virtual void 
    update(Audine* ccd, TextPropertyVector* pv, char* name[], char* text[], int n);
  
  virtual void 
    update(Audine* ccd, SwitchPropertyVector* pv, char* name, ISState swit);
  
  virtual void 
    update(Audine* ccd, NumberPropertyVector* pv, char* name[], double num[], int n);


  /**************************/
  /* events coming from COR */
  /**************************/

  virtual void 
    handle(Audine* ccd, unsigned int event, const void* data, int len)
    { 
      AudineState::handle(ccd, event, data, len);
    }

  /*****************************************************/
  /* events comming from other Plugin (observables) */
  /*****************************************************/
     
  virtual void
    update(Audine* ccd, PropertyVector* pv, ITopic t);

  /******************/
  /* timeout events */
  /******************/

#if 0  
  virtual void tick(Audine* ccd);		
  virtual void timeout(Audine* ccd);	
#endif

 private:

  AudineOk() : AudineState("AudineOk") {}
  static AudineState* _instance;

  virtual void setVisualState(Audine* ccd);

  /*********************************/
  /* helper methods for this state */
  /*********************************/

  void exposure(Audine* ccd, SwitchPropertyVector* pv,
		char* name, ISState swit);

};


/*************************/
/* THE AUDINE WAIT STATE */
/*************************/

class AudineWait : public AudineState {

 public:
  
  virtual ~AudineWait() {}

  static AudineState* instance();

  virtual const char* getName() { return ("En espera"); }
  
  /*****************************************/
  /* events coming from the user interface */
  /*****************************************/

  
  virtual void 
    update(Audine* ccd, SwitchPropertyVector* pv, char* name, ISState swit);


  /**************************/
  /* events coming from COR */
  /**************************/

#if 0
  virtual void 
    handle(Audine* ccd, unsigned int event, const void* data, int len);
#endif

  /*****************************************************/
  /* events comming from other Plugin (observables) */
  /*****************************************************/
     
  virtual void
    update(Audine* ccd, PropertyVector* pv, ITopic t);

  /******************/
  /* timeout events */
  /******************/

  virtual void tick(Audine* ccd);		
  virtual void timeout(Audine* ccd);	

 private:

  AudineWait() : AudineState("AudineWait") {}
  static AudineState* _instance;

  virtual void setVisualState(Audine* ccd);

  /*********************************/
  /* helper methods for this state */
  /*********************************/

  void exposure(Audine* ccd, SwitchPropertyVector* pv,
		char* name, ISState swit);

};



/*****************************/
/* THE AUDINE EXPOSURE STATE */
/*****************************/

class AudineExp : public AudineState {

 public:
  
  virtual ~AudineExp() {}

  static AudineState* instance();

  virtual const char* getName() { return ("En exposicion"); }
  
  /*****************************************/
  /* events coming from the user interface */
  /*****************************************/

  
  virtual void 
    update(Audine* ccd, SwitchPropertyVector* pv, char* name, ISState swit);

  /**************************/
  /* events coming from COR */
  /**************************/

  virtual void 
    handle(Audine* ccd, unsigned int event, const void* data, int len);

  /*****************************************************/
  /* events comming from other Plugin (observables) */
  /*****************************************************/
     
  virtual void
    update(Audine* ccd, PropertyVector* pv, ITopic t);

  /******************/
  /* timeout events */
  /******************/

  virtual void tick(Audine* ccd);		
  virtual void timeout(Audine* ccd);	

 private:

  AudineExp() : AudineState("AudineExp") {}
  static AudineState* _instance;

  virtual void setVisualState(Audine* ccd);

  /*********************************/
  /* helper methods for this state */
  /*********************************/

  void exposure(Audine* ccd, SwitchPropertyVector* pv,
		char* name, ISState swit);

};



/*************************/
/* THE AUDINE READ STATE */
/*************************/

class AudineRead : public AudineState {

 public:
  
  virtual ~AudineRead() {}

  static AudineState* instance();
  
  virtual const char* getName() { return ("Leyendo CCD"); }

  /*****************************************/
  /* events coming from the user interface */
  /*****************************************/

#if 0				/* not yet implemented */
  virtual void 
    update(Audine* ccd, BLOBPropertyVector* pv, char* name[], char* blob[], int n);


  virtual void 
    update(Audine* ccd, TextPropertyVector* pv, char* name[], char* text[], int n);
  
  virtual void 
    update(Audine* ccd, SwitchPropertyVector* pv, char* name, ISState swit);

  virtual void 
    update(Audine* ccd, NumberPropertyVector* pv, char* name[], double num[], int n);
#endif

  
  /**************************/
  /* events coming from COR */
  /**************************/

  virtual void 
    handle(Audine* ccd, unsigned int event, const void* data, int len);


  /*****************************************************/
  /* events comming from other Plugin (observables) */
  /*****************************************************/

  virtual void
    update(Audine* ccd, PropertyVector* pv, ITopic t);
     

  /******************/
  /* timeout events */
  /******************/

  virtual void tick(Audine* ccd);		
  virtual void timeout(Audine* ccd);	

 private:

  AudineRead() : AudineState("AudineRead") {}
  static AudineState* _instance;

  virtual void setVisualState(Audine* ccd);

};

/**************************/
/* THE AUDINE ALERT STATE */
/**************************/

class AudineAlert : public AudineState {

 public:
  
  virtual ~AudineAlert() {}

  static AudineState* instance();
  
  virtual const char* getName() { return ("Alarma"); }

  /*****************************************/
  /* events coming from the user interface */
  /*****************************************/

#if 0				/* not yet implemented */
  virtual void 
    update(Audine* ccd, BLOBPropertyVector* pv, char* name[], char* blob[], int n);


  virtual void 
    update(Audine* ccd, TextPropertyVector* pv, char* name[], char* text[], int n);
  
  virtual void 
    update(Audine* ccd, SwitchPropertyVector* pv, char* name, ISState swit);

  virtual void 
    update(Audine* ccd, NumberPropertyVector* pv, char* name[], double num[], int n);
  
  /**************************/
  /* events coming from COR */
  /**************************/

  virtual void 
    handle(Audine* ccd, unsigned int event, const void* data, int len);
#endif

  /*****************************************************/
  /* events comming from other Plugin (observables) */
  /*****************************************************/
     
  virtual void
    update(Audine* ccd, PropertyVector* pv, ITopic t);

#if 0
  /******************/
  /* timeout events */
  /******************/

  virtual void tick(Audine* ccd);		
  virtual void timeout(Audine* ccd);	

#endif

 private:

  AudineAlert() : AudineState("AudineAlert") {}
  static AudineState* _instance;

  virtual void setVisualState(Audine* ccd);

};



#endif

