/*
 * $Id: chkucodes.cpp,v 1.1 2004/10/27 19:04:20 essu Exp $
 *
 * Check Ucodes 
 *
 * Copyright (C) 2004 alexh  <alexh@yadi.org>,
 *                    essu   <essu@yadi.org>,
 *                    mogway <mogway@yadi.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 *
 */


#include <stdio.h>
#include <errno.h>
#include <fstream>
#include <linux/input.h>

#include "lcddisplay.h"
#include "rc.h"

int pt[3]={1, 10, 100};


int ucodesFound(){
  static char *camAlphaPath="/var/tuxbox/ucodes/cam-alpha.bin";
  static char *avia500Path="/var/tuxbox/ucodes/avia500.ux";
  static char *avia600Path="/var/tuxbox/ucodes/avia600.ux";

  struct stat sta;
  int result = 1;

  if(stat(camAlphaPath, &sta)){
    // cam-alpha.bin gibts nicht
    result=0;
  }

  if((stat(avia500Path, &sta)) && (stat(avia600Path, &sta))) {
    // avia500.ux oder avia600.ux muss existieren
    result = 0;
  }
  return result;
}

void waitForOK() {
  
  int update=0;
  int done=0;
  RCOpen();
  while (!done){
    if (!update){
      int code;
      switch (code=RCGet()){
      case KEY_LEFT:
      case KEY_RIGHT:
      case KEY_UP:
      case KEY_0:
      case KEY_1:
      case KEY_2:
      case KEY_3:
      case KEY_4:
      case KEY_5:
      case KEY_6:
      case KEY_7:
      case KEY_8:
      case KEY_9:
      case KEY_DOWN:
      case KEY_POWER:
	break;

      case KEY_OK:
	done=1;
	break;

      case KEY_HOME:
        CLCDDisplay display;
        exit(0);
        break;
      }
    }
  }
  RCClose();
}


int main(){
  fprintf(stderr, "$Id: chkucodes.cpp,v 1.1 2004/10/27 19:04:20 essu Exp $\n");

  int ucodes_found = ucodesFound();

  while(!ucodes_found){
    CLCDDisplay display;
    // Meldung ausgeben...
    display.draw_string(0, 10, "Per FTP ucodes ");
    display.draw_string(0, 22, "nach /var/tuxbo");
    display.draw_string(0, 34, "x/ucodes laden ");
    display.draw_string(0, 55, " [OK = Weiter] ");
    display.update();

    waitForOK();
   
    ucodes_found=ucodesFound();

    if(!ucodes_found){
      // Keine oder nicht alle ucodes gefunden
      display.draw_string(0, 10, "               ");
      display.draw_string(0, 22, "               ");
      display.draw_string(0, 34, "               ");
      display.draw_string(0, 55, "               ");
      display.draw_string(0, 10, "Fehler: Ucodes ");
      display.draw_string(0, 22, "konnten nicht  ");
      display.draw_string(0, 34, "gefunden werden");
      display.draw_string(0, 55, "[OK = Zurueck] ");
      display.update();

      waitForOK();

    }else{
      // Ucodes erfolgreich hochgeladen
      // Meldungen ausgeben...
      display.draw_string(0, 10, "               ");
      display.draw_string(0, 22, "               ");
      display.draw_string(0, 34, "               ");
      display.draw_string(0, 55, "               ");
      display.draw_string(0, 22, "    Ucodes     ");
      display.draw_string(0, 34, "  akzeptiert   ");
      display.draw_string(0, 55, " [OK = Reboot] ");
      display.update();

      waitForOK();
	
      display.draw_string(0, 22, "               ");
      display.draw_string(0, 34, "               ");
      display.draw_string(0, 55, "               ");
      display.draw_string(0, 30, " Starte neu... ");
      display.update();

      // System neu starten
      system("/sbin/reboot");
      return 0;

    }// else (!ucodes_found)
  }//while(!ucodes_found)

  
        
  return 0;
}
