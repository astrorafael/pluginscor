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

#ifndef AUDINE_SHUTTER_H
#define AUDINE_SHUTTER_H

class Audine;

class Shutter {

 public:
  
  Shutter(Audine* aud) :
     shutterLogic(0), shutterDelay(0), audine(aud) {}


  /* shutter initialization from current device tree */
  void init();
  
 /* action when SHUTTER_DELAY is set */
  void updateDelay(char* name[], double number[], int n);

  /* action when SHUTTER_LOGIC switch is selected */
  void updateLogic(char* name, ISState swit);

  /* updates shutter mode and delay when image type is changed elsewhere */
  void update();

  /* gets the shutter delay depending on image type */
  double getDelay();

 private:

  /*********************************/
  /* THE USER INTERFACE PROPERTIES */
  /*********************************/


  SwitchPropertyVector* shutterLogic;
  NumberPropertyVector* shutterDelay;


  /********************/
  /* other attributes */
  /********************/

  Audine* audine;

};

#endif
