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

#ifndef PERSISTENT_COUNTER_H
#define PERSISTENT_COUNTER_H

#include <errno.h>

template<class T>
class PersistentCounter {

 public:

  PersistentCounter() :
    cnt(0), path("/tmp/persiscnt") {init(path.c_str()); }

  /* inits counter in a new location */
  void init(const char* newpath) {
    FILE* fp;
    int n;
    
    path = newpath;
    fp = fopen(newpath, "r");	/* test for existing */
  
    /* if file does not exists, create it and write initial value */
    /* if file exists, read the counter from the file */
    
    if (fp == NULL) {
      fp = fopen(newpath, "w+");	/* create it */
      n  = fwrite(&cnt, 1, sizeof cnt, fp);
      assert(n == sizeof cnt);
    } else {
      n  = fread(&cnt, 1, sizeof cnt, fp);
      assert(n == sizeof cnt);    
    }
    fclose(fp);
  }

  /* sets a new value for the counter */
  void set(T value) {
    FILE* fp;
    int n;
    
    fp = fopen(path.c_str(), "r+");	/* open it */
    assert(fp != NULL);
    
    cnt = value;
    n = fwrite(&cnt, 1, sizeof cnt, fp);
    assert(n == sizeof cnt);
    fclose(fp);
  }

  /* gets the current value */
  T value() const { return (cnt); }

  /* increments counter */
  void up() {
    FILE* fp;
    int n;
    
    fp = fopen(path.c_str(), "r+");	/* open it */
    assert(fp != NULL);
    
    cnt++;
    n = fwrite(&cnt, 1, sizeof cnt, fp);
    assert(n == sizeof cnt);
    fclose(fp);
  }

  /* decrements counter */
  void down() {
    FILE* fp;
    int n;
    
    fp = fopen(path.c_str(), "r+");	/* open it */
    assert(fp != NULL);
    
    cnt--;
    n = fwrite(&cnt, 1, sizeof cnt, fp);
    assert(n == sizeof cnt);
    fclose(fp);
  }

 private:
  
  T cnt;
  std::string path;

};

#endif
