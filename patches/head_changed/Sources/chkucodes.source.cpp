#include <stdio.h>
#include <errno.h>
#include <fstream>
#include <linux/input.h>

#include "lcddisplay.h"
#include "rc.h"

CLCDDisplay display;

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
      }
    }
  }
  RCClose();
}


int main(){
  int ucodes_found = ucodesFound();
  while(!ucodes_found){

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
  //RCOpen();
  //int code;
  //code=RCGet();
  //if (code == KEY_0) {
  //  display.draw_string(0, 2,  "               ");
  //  display.draw_string(0, 16, " MAINTAINANCE- ");
  //  display.draw_string(0, 28, "     MODE      ");
  //  display.draw_string(0, 40, "               ");
  //  display.draw_string(0, 54, "               ");
  //  display.update();
  //  return 1;
  //} // if (code == KEY_0)
  //else {
    // "/.image_version" auf Display ausgeben
    FILE* fl=fopen("/.image_version","r");
    char buf[16];        
 
    if(!fl) {
      display.draw_string(0, 2,  " IMAGE-INFOS:  ");
      display.draw_string(0, 16, "Datum: unknown ");
      display.draw_string(0, 28, " Ver.: head-cvs");
      display.draw_string(0, 40, "  Von: unknown ");
      display.draw_string(0, 54, " Starte GUI... ");
      display.update();
    } else {
      if(fgets(buf,sizeof(buf),fl)) {
        display.draw_string(0, 2,  buf);
      }
      if(fgets(buf,sizeof(buf),fl)) {
        if(fgets(buf,sizeof(buf),fl)) {
          display.draw_string(0, 16,  buf);
        }
      }
      if(fgets(buf,sizeof(buf),fl)) {
        if(fgets(buf,sizeof(buf),fl)) {
          display.draw_string(0, 28,  buf);
        }
      }
      if(fgets(buf,sizeof(buf),fl)) {
        if(fgets(buf,sizeof(buf),fl)) {
          display.draw_string(0, 40,  buf);
        }
      }
      if(fgets(buf,sizeof(buf),fl)) {
        if(fgets(buf,sizeof(buf),fl)) {
          display.draw_string(0, 54,  buf);
        }
      }
      display.update();
    }
    //} // Else (maintainance-mode)

        
  return 0;
}
