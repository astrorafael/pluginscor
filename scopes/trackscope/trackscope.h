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

#ifndef COR_TRACKSCOPE_H
#define COR_TRACKSCOPE_H

#ifndef INDIAPI_H
#include <indicor/api.h>
#endif


/*******************************/
/* THE PLUGIN FACTORY FUNCTION */
/*******************************/

BEGIN_C_DECLS

Plugin* CreatePlugin(Device*,  const char* args);

END_C_DECLS

/* Trcking-type telescope with no GOTO or remote commanded capabilites */
/* just to receive IND request from XEphem and sets data accordingly */
/* Data changes are notified from this device to all Audine devices to
update their FITS headers */

class TrackScope : public PluginBase
{
  public:
  
    TrackScope(Device* dev) : PluginBase(dev,"TrackScope") {}
    ~TrackScope() {}
    
    
  /* ****************** */
  /* THE INDI INTERFACE */
  /* ****************** */

  virtual void init();

  virtual void 
    update(TextPropertyVector* pv, char* name[], char* text[], int n);
  
  virtual void 
    update(NumberPropertyVector* pv, char* name[], double num[], int n);
    
  virtual void 
    update(SwitchPropertyVector* pv, char* name, ISState swit);
    
   
  /*****************************************************/
  /* events coming from other Plugin (observables) */
  /*****************************************************/

  virtual void
    update(PropertyVector* pv, ITopic t);
    
  private:
  
  /*********************************/
  /* THE USER INTERFACE PROPERTIES */
  /*********************************/
  
  NumberPropertyVector* eqCoords;
  NumberPropertyVector* optics;
  TextPropertyVector*   object;
  TextPropertyVector*   edbLine;
  TextPropertyVector*   fitsTextData;
  SwitchPropertyVector* configuration;

  /* ************** */
  /* HELPER METHODS */
  /* ************** */

  void unknown(PropertyVector* pv);

  void updateObject(char* name[], char* text[], int n);

  void updateEdbLine(char* name[], char* text[], int n);

  void updateFITSData(char* name[], char* text[], int n);

  void save(char* name, ISState swit);

  bool parseEDBLine(const char* line, char* objname, double* ra, double* dec);


};

#endif
