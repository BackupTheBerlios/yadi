#include <stdio.h>
#include <errno.h>
#include <linux/input.h>

#include "lcddisplay.h"
#include "rc.h"

CLCDDisplay display;

int pt[3]={1, 10, 100};

int main()
{
	

	
	
        /*
	FILE* pFile=fopen("/var/ip.lcdip", "r"); // tum lesen "w" zum schreiben
	if ((pFile == NULL)&&(errno != ENOENT)){
		printf ("error: %d ",errno);
		perror ("/var/ip.lcdip");
		return 1;
	} 
	else if (pFile != NULL){
		int err = fscanf (pFile,"%03d.%03d.%03d.%03d\n",&ip[0],&ip[1],&ip[2],&ip[3]);
		if ((err == EOF) ||( err<4)){
			printf ("error: /var/ip.lcdip too short, using default\n");
			ip[0]=192;ip[1]=168;ip[2]=0;ip[3]=1;
		}
		fclose(pFile);
	}

        */

	
        // IP einlesen
        display.draw_string(10, 10, "     IP");
        display.draw_string(10, 22, "  eingeben:");
        display.draw_string(0, 55, " [OK = Weiter]");
        
        int ip[4]={192,168,0,1};

        int curpos=0;
	int update=1;
	int done=0;
	RCOpen();
	while (!done)
	{
		if (!update)
		{
			int code;
			switch (code=RCGet())
			{
			case KEY_LEFT:
				if (curpos)
				{
					curpos--;
					update=1;
				}
				break;
			case KEY_RIGHT:
				if (curpos<14)
				{
					curpos++;
					update=1;
				}
				break;
			case KEY_UP:
			{
				if ((curpos%4)==3)
					break;
				int i=curpos/4;
				int a=2-(curpos%4);
				int nv=ip[i];
				int s=(nv/pt[a])%10;
				nv-=s*pt[a];
				s++;
				if (s>9)
					s=0;
				nv+=s*pt[a];
				ip[i]=nv; 
				update=1;
				break;
			}

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
			{
				unsigned char keynumber[10]={KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9};
				int number;
				for (number=0;number<10;number++) if (code==keynumber[number]) break;
				if ((curpos%4)==3)
					break;
				int i=curpos/4;
				int a=2-(curpos%4);
				int nv=ip[i];
				int s=(nv/pt[a])%10;
				nv-=s*pt[a];
				s=number;
				nv+=s*pt[a];
				ip[i]=nv; 
				update=1;
				if (curpos<14)
				{
					curpos++;
					if ((curpos%4)==3)
						curpos++;
				} /* else
					done=1;*/ 
				break;
			}
			case KEY_DOWN:
			{
				if ((curpos%4)==3)
					break;
				int i=curpos/4;
				int a=2-(curpos%4);
				int nv=ip[i];
				int s=(nv/pt[a])%10;
				nv-=s*pt[a];
				s--;
				if (s<0)
					s=9;
				nv+=s*pt[a];
				ip[i]=nv; 
				update=1;
				break;
			}
			case KEY_POWER:
				if (curpos<14){
					curpos++;
					update=1;
					break;
				}
			case KEY_OK:
				done=1;
				update=1;
				break;
			}
		}
		if (update)
		{
			char ips[16];
			sprintf(ips, "%03d.%03d.%03d.%03d", ip[0], ip[1], ip[2], ip[3]);
			if (done)
			{
				if ((ip[0]>255) || (ip[1]>255) || (ip[2]>255) || (ip[3]>255))
				{
					done=0;
					display.draw_string(0, 38, "  INVALID IP!  ");
					curpos = 0;
					display.update();
					int code = RCGet();
					while ((code!=KEY_OK)&&(code!=KEY_POWER));
				}
			}			
			display.draw_string(0, 38, ips);
			if (!done)
				display.draw_rectangle(curpos*8, 38, curpos*8+7, 45, LCD_PIXEL_INV, LCD_PIXEL_INV);
			display.update();
			update=0;
		}
	}
	RCClose();


        // netmask einlesen
        display.draw_string(10, 10, "   Netmask");
        display.draw_string(10, 22, "  eingeben:");

        int netmask[4]={255,255,255,0};

        curpos=0;
	update=1;
	done=0;
	RCOpen();
	while (!done)
	{
		if (!update)
		{
			int code;
			switch (code=RCGet())
			{
			case KEY_LEFT:
				if (curpos)
				{
					curpos--;
					update=1;
				}
				break;
			case KEY_RIGHT:
				if (curpos<14)
				{
					curpos++;
					update=1;
				}
				break;
			case KEY_UP:
			{
				if ((curpos%4)==3)
					break;
				int i=curpos/4;
				int a=2-(curpos%4);
				int nv=netmask[i];
				int s=(nv/pt[a])%10;
				nv-=s*pt[a];
				s++;
				if (s>9)
					s=0;
				nv+=s*pt[a];
				netmask[i]=nv; 
				update=1;
				break;
			}

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
			{
				unsigned char keynumber[10]={KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9};
				int number;
				for (number=0;number<10;number++) if (code==keynumber[number]) break;
				if ((curpos%4)==3)
					break;
				int i=curpos/4;
				int a=2-(curpos%4);
				int nv=netmask[i];
				int s=(nv/pt[a])%10;
				nv-=s*pt[a];
				s=number;
				nv+=s*pt[a];
				netmask[i]=nv; 
				update=1;
				if (curpos<14)
				{
					curpos++;
					if ((curpos%4)==3)
						curpos++;
				} /* else
					done=1;*/ 
				break;
			}
			case KEY_DOWN:
			{
				if ((curpos%4)==3)
					break;
				int i=curpos/4;
				int a=2-(curpos%4);
				int nv=netmask[i];
				int s=(nv/pt[a])%10;
				nv-=s*pt[a];
				s--;
				if (s<0)
					s=9;
				nv+=s*pt[a];
				netmask[i]=nv; 
				update=1;
				break;
			}
			case KEY_POWER:
				if (curpos<14){
					curpos++;
					update=1;
					break;
				}
			case KEY_OK:
				done=1;
				update=1;
				break;
			}
		}
		if (update)
		{
			char netmasks[16];
			sprintf(netmasks, "%03d.%03d.%03d.%03d", netmask[0], netmask[1], netmask[2], netmask[3]);
			if (done)
			{
				if ((netmask[0]>255) || (netmask[1]>255) || (netmask[2]>255) || (netmask[3]>255))
				{
					done=0;
					display.draw_string(0, 38, "  INVALID Netmask!  ");
					curpos = 0;
					display.update();
					int code = RCGet();
					while ((code!=KEY_OK)&&(code!=KEY_POWER));
				}
			}			
			display.draw_string(0, 38, netmasks);
			if (!done)
				display.draw_rectangle(curpos*8, 38, curpos*8+7, 45, LCD_PIXEL_INV, LCD_PIXEL_INV);
			display.update();
			update=0;
		}
	}
	RCClose();

        // Gateway einlesen
        display.draw_string(10, 10, "   Gateway");
        display.draw_string(10, 22, "  eingeben:");

        int gateway[4]={127,0,0,1};

        curpos=0;
	update=1;
	done=0;
	RCOpen();
	while (!done)
	{
		if (!update)
		{
			int code;
			switch (code=RCGet())
			{
			case KEY_LEFT:
				if (curpos)
				{
					curpos--;
					update=1;
				}
				break;
			case KEY_RIGHT:
				if (curpos<14)
				{
					curpos++;
					update=1;
				}
				break;
			case KEY_UP:
			{
				if ((curpos%4)==3)
					break;
				int i=curpos/4;
				int a=2-(curpos%4);
				int nv=gateway[i];
				int s=(nv/pt[a])%10;
				nv-=s*pt[a];
				s++;
				if (s>9)
					s=0;
				nv+=s*pt[a];
				gateway[i]=nv; 
				update=1;
				break;
			}

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
			{
				unsigned char keynumber[10]={KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9};
				int number;
				for (number=0;number<10;number++) if (code==keynumber[number]) break;
				if ((curpos%4)==3)
					break;
				int i=curpos/4;
				int a=2-(curpos%4);
				int nv=gateway[i];
				int s=(nv/pt[a])%10;
				nv-=s*pt[a];
				s=number;
				nv+=s*pt[a];
				gateway[i]=nv; 
				update=1;
				if (curpos<14)
				{
					curpos++;
					if ((curpos%4)==3)
						curpos++;
				} /* else
					done=1;*/ 
				break;
			}
			case KEY_DOWN:
			{
				if ((curpos%4)==3)
					break;
				int i=curpos/4;
				int a=2-(curpos%4);
				int nv=gateway[i];
				int s=(nv/pt[a])%10;
				nv-=s*pt[a];
				s--;
				if (s<0)
					s=9;
				nv+=s*pt[a];
				gateway[i]=nv; 
				update=1;
				break;
			}
			case KEY_POWER:
				if (curpos<14){
					curpos++;
					update=1;
					break;
				}
			case KEY_OK:
				done=1;
				update=1;
				break;
			}
		}
		if (update)
		{
			char gateways[16];
			sprintf(gateways, "%03d.%03d.%03d.%03d", gateway[0], gateway[1], gateway[2], gateway[3]);
			if (done)
			{
				if ((gateway[0]>255) || (gateway[1]>255) || (gateway[2]>255) || (gateway[3]>255))
				{
					done=0;
					display.draw_string(0, 38, "  INVALID Gateway!  ");
					curpos = 0;
					display.update();
					int code = RCGet();
					while ((code!=KEY_OK)&&(code!=KEY_POWER));
				}
			}			
			display.draw_string(0, 38, gateways);
			if (!done)
				display.draw_rectangle(curpos*8, 38, curpos*8+7, 45, LCD_PIXEL_INV, LCD_PIXEL_INV);
			display.update();
			update=0;
		}
	}
	RCClose();

        

        // Meldung ausgeben...
        display.draw_string(0, 10, "               ");
        display.draw_string(0, 22, "               ");
        display.draw_string(0, 38, "               ");
        display.draw_string(0, 55, "               ");
        display.draw_string(0, 20, "Die Box startet");
        display.draw_string(0, 32, "   jetzt neu   ");
        display.draw_string(0, 55, " [OK = Reboot] ");
        display.update();

        curpos=0;
	update=0;
	done=0;
	RCOpen();
	while (!done)
	{
		if (!update)
		{
			int code;
			switch (code=RCGet())
			{
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

	// Datei schreiben
	FILE* paFile=fopen("/etc/network/interfaces", "w"); 
	if (paFile == NULL){
		perror ("writing /etc/network/interfaces");
		return 2;
	}

        fprintf (paFile,"auto lo\n");
        fprintf (paFile,"iface lo inet loopback\n");
        fprintf (paFile,"\n");
        fprintf (paFile,"auto eth0\n");
        fprintf (paFile,"iface eth0 inet static\n");
	fprintf (paFile,"  address %d.%d.%d.%d\n",ip[0],ip[1],ip[2],ip[3]);
	fprintf (paFile,"  gateway %d.%d.%d.%d\n",gateway[0],gateway[1],gateway[2],gateway[3]);
	fprintf (paFile,"  netmask %d.%d.%d.%d\n",netmask[0],netmask[1],netmask[2],netmask[3]);
        fprintf (paFile,"\n");
	fclose(paFile);





// if needed: replace with whatever you want to do with the ip...
/*	char exec[100];
	sprintf(exec, "/sbin/ifconfig eth0 %d.%d.%d.%d up", ip[0], ip[1], ip[2], ip[3]);
	if (!(system(exec)>>8))
	{
		display.draw_string(0, 30, "  IP GESETZT!  ");
		display.update();
	} else
	{
		display.draw_string(0, 30, "    FEHLER!    ");
		display.update();
	}
*/
        display.draw_string(0, 20, "               ");
        display.draw_string(0, 32, "               ");
        display.draw_string(0, 55, "               ");
	display.draw_string(0, 30, " Starte neu... ");
	display.update();
	return 0;
}
