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

#ifndef COR_CORSTAT_H
#define COR_CORSTAT_H

#include "cor.h"

/*******************************************/
/* THE STATE PATTERN APPLIED TO COR Device */
/*******************************************/

/*
 * The mapping of COR device states to supervision states
 * shown by INDI is as follows:
 *  
 *  CORIdle     => IPS_IDLE  (grey)
 *  COROk       => IPS_OK    (green)
 *  CORBusy     => IPS_BUSY  (yellow)
 *  CORAlert    => IPS_ALERT (red) 
 *
 */ 

class CORState {

 public:
  
  CORState(const char* tag);
  virtual ~CORState() {delete log;}
  
  virtual const char* getName() = 0;

  /* state change method */

  void nextState(COR* cor, CORState* st) {
    st->setVisualState(cor);	/* proper color for next state */
    cor->nextState(st);
  }

  /*****************************************/
  /* events coming from the user interface */
  /*****************************************/

  virtual void 
    update(COR* cor, SwitchPropertyVector* pv, char* name, ISState swit) {
    forbidden(pv);
  }
  
  virtual void 
    update(COR* cor, NumberPropertyVector* pv, char* name[], double num[], int n) {
    forbidden(pv);
  }
  
  /*****************************************/
  /* events coming from the COR box by UDP */
  /*****************************************/

  virtual void connectResp(COR* cor, Incoming_Message* msg);
  virtual void presenceInd(COR* cor, Incoming_Message* msg);

  /*****************************************************/
  /* events comming from other Plugin (observables) */
  /*****************************************************/

  virtual void
    update(COR* cor, PropertyVector* pv, ITopic t);

  /******************/
  /* timeout events */
  /******************/

  virtual void tick(COR* cor);	/* ping timeout period */
  virtual void timeout(COR* cor);	/* safety watchdog */

 protected:

  Log* log;

  /* senda a forbidden message to GUI */
  void forbidden(PropertyVector* pv);

  /* default update coming from other observables */
  void defUpdate(COR* cor, PropertyVector* pv, ITopic t);

  /* default update with minimal options */
  void minimalUpdate(COR* cor, SwitchPropertyVector* pv,  char* name, 
		 ISState swit);

  /* default update with minimal options */
  void minimalUpdate(COR* cor, NumberPropertyVector* pv, char* name[], 
		 double number[], int n) ;

  /* performs a connect request by user */
  void connectReq(COR* cor, SwitchPropertyVector* pv, 
	   char* name, ISState swit);

  /* performs a disconnect by user */
  void disconnectReq(COR* cor, SwitchPropertyVector* pv, char* name,
		     ISState swit);

  /* sets visual state in GUI */
  virtual void setVisualState(COR* cor) = 0;

};

/*---------------------------------------------------------------------------*/

/*************************/
/* THE COR IDLE STATE */
/*************************/

class CORIdle : public CORState {

 public:
  
  virtual ~CORIdle() {}

  static CORState* instance();
  

  virtual const char* getName() { return ("Desconectado"); }

   
  /**************************/
  /* events coming from COR */
  /**************************/

  void presenceInd(COR* cor, Incoming_Message* msg);

  /*****************************************************/
  /* events comming from other Plugin (observables) */
  /*****************************************************/


  /*****************************************/
  /* events coming from the user interface */
  /*****************************************/

  virtual void 
    update(COR* cor, SwitchPropertyVector* pv,
	   char* name, ISState swit);

  virtual void 
    update(COR* cor, NumberPropertyVector* pv, char* name[], double num[], int n);
 

 private:

  CORIdle() : CORState("CORIdle") {}
  static CORState* _instance;

  virtual void setVisualState(COR* cor);

};


/***********************/
/* THE COR OK STATE */
/***********************/

class COROk : public CORState {

 public:
  
  virtual ~COROk() {}

  static CORState* instance();

  virtual const char* getName() { return ("Conectado"); }
  
  /*****************************************/
  /* events coming from the user interface */
  /*****************************************/
  
  virtual void 
    update(COR* cor, SwitchPropertyVector* pv, char* name, ISState swit);
  

  /*****************************************************/
  /* events comming from other Plugin (observables) */
  /*****************************************************/
     
  virtual void
    update(COR* cor, PropertyVector* pv,  ITopic t);

  /******************/
  /* timeout events */
  /******************/

  virtual void tick(COR* cor);		

 private:

  COROk() : CORState("COROk") {}
  static CORState* _instance;

  virtual void setVisualState(COR* cor);

};


/*************************/
/* THE COR BUSY STATE */
/*************************/

class CORBusy : public CORState {

 public:
  
  virtual ~CORBusy() {}

  static CORState* instance();

  virtual const char* getName() { return ("Ocupado"); }
  
  /*****************************************/
  /* events coming from the user interface */
  /*****************************************/

  virtual void 
    update(COR* cor, SwitchPropertyVector* pv, char* name, ISState swit);

  /**************************/
  /* events coming from COR */
  /**************************/

  virtual void connectResp(COR* cor, Incoming_Message* msg);

  /*****************************************************/
  /* events comming from other Plugin (observables) */
  /*****************************************************/
     
  virtual void
    update(COR* cor, PropertyVector* pv, ITopic t);

  /******************/
  /* timeout events */
  /******************/

  virtual void tick(COR* cor);	/* plugin periodic timer */
  virtual void timeout(COR* cor);	

 private:

  CORBusy() : CORState("CORBusy") {}
  static CORState* _instance;

  virtual void setVisualState(COR* cor);

};


/**************************/
/* THE COR ALERT STATE */
/**************************/

class CORAlert : public CORState {

 public:
  
  virtual ~CORAlert() {}

  static CORState* instance();
  
  virtual const char* getName() { return ("Alarma"); }

  /*****************************************/
  /* events coming from the user interface */
  /*****************************************/

  
  virtual void 
    update(COR* cor, SwitchPropertyVector* pv, char* name, ISState swit);

  virtual void 
    update(COR* cor, NumberPropertyVector* pv, char* name[], double num[], int n);
  
  /**************************/
  /* events coming from COR */
  /**************************/


  /*****************************************************/
  /* events coming from other Plugin (observables) */
  /*****************************************************/
     

  /******************/
  /* timeout events */
  /******************/


 private:

  CORAlert() : CORState("CORAlert") {}
  static CORState* _instance;

  virtual void setVisualState(COR* cor);

};



#endif

