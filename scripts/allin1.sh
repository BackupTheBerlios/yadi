#!/bin/bash
#
# allin1 $Id: allin1.sh,v 1.11 2004/03/07 18:29:03 essu Exp $
#
# Copyright (c) 2004 essu, dmitri, Acoo Germany. All rights reserved.
# Mail: acoo@berlios.de
# Mail: essu@berlios.de
# Mail: @berlios.de
#
# aktuelle Versionen gibt es hier:
# $Source: /home/xubuntu/berlios_backup/github/tmp-cvs/yadi/Repository/scripts/allin1.sh,v $
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 675 Mass Ave, Cambridge MA 02139, USA.
#
# Mit diesem Programm koennen Images fuer DBox2 erstellt werden
#
# -------------------------------------------------------
#
# folgende Optionen einstellen:
RM_CVS='yes' 			# yes | no  : altes CVS loeschen oder updaten
USE_CHANGE_ARC='no'		# yes | no  : Patches aus Archiv oder Verzeichnis
# Folgende Pfade und Dateinamen anpassen:
RT=$HOME/yadi
CVS=$RT/tuxbox-cvs		# Pfad zum CVS
DBOX=$RT/dbox 			# Pfad zu dbox2
IMAGES=$RT/images 		# Pfad wohin die fertigen Images (mit Datum) kopiert werden
VERSION=" Ver.: 0.1     " 	# Zeilenlaenge: genau 15 Zeichen sollte kuenftig $Revision: 1.11 $ enthalten
# Pfad zu den geaenderten und sonstigen Dateien
CHANGE_DIR=$RT/head_changed
CHANGE_ARC_DIR=$RT/change_arcs
CHANGE_ARC=head_changed_A # Datei mit den Aenderungen (ohne .tar.gz)
#
# Nun braucht nichts mehr angepasst zu werden
# ------------------------------------------------------
#
prepare()
{
# um kueftig verschiedene Aenderungsarchive verwenden zu koennen, die evtl. per download eingebunden werden
cd $CHANGEDIR || USE_CHANGE_ARC='yes' # falls das Aenderungsverzeichnis fehlt
if [ $USE_CHANGE_ARC = "yes" ]; then
 if [ ! -e $CHANGE_ARC_DIR/$CHANGE_ARC.tar.gz ]; then
  echo "Archiv $CHANGE_ARC_DIR/$CHANGE_ARC.tar.gz nicht gefunden"
  exit 1;  # ohne Patches kein Image
 else
  # Aenderungsarchiv entpacken
  cd $RT
  cp $CHANGE_ARC_DIR/$CHANGE_ARC.tar.gz .
  gzip -d $CHANGE_ARC.tar.gz
  tar -xpf $CHANGE_ARC.tar
  rm $CHANGE_ARC.tar
  CHANGE_DIR=$RT/$CHANGE_ARC 	# es ist vorausgesetzt, das Archiv enthaelt einen Ordner mit dem Archivnamen (ohne '.tar.gz')
 fi
fi
echo "Die Aenderungs-Daten wie Patches etc. befinden sich in "$CHANGE_DIR
# Namensprefix mit Datum
IMG_PRE=`date +%Y%m%d%H%M`
IMG_PRE=$IMG_PRE"yadi_"
if [ ! -e $IMAGES ]; then
 mkdir $IMAGES
fi

# File mit Image-Infos (wird erstellt)
# Aufbau:
# Zwei Dateien (.image_version.neutrino und .image_version.enigma)
# Zeilenlaenge: Jeweils genau 15 Zeichen + Return
# 1. Zeile Ueberschrift (ein paar Pixel mehr Abstand zur neuen Zeile)
# 2. Zeile Image-Datum
# 3. Zeile Image-Version
# 4. Zeile Image-Name
# 5. Zeile beschreibt, was gestartet werden soll
# (etwas groesserer Abstand zur Zeile davor)

VER=$CHANGE_DIR/.image_version
EN=$VER.enigma
NE=$VER.neutrino

# Image-Versionen schreiben
echo " IMAGE-INFOS:  "  > $EN
echo "Datum: $(date '+%d.%m.%y')" >> $EN
echo "$VERSION" >> $EN
echo " Yadi - Image  " >> $EN
echo " Starte Enigma " >> $EN

echo " IMAGE-INFOS:  "  > $NE
echo "Datum: $(date '+%d.%m.%y')" >> $NE
echo "$VERSION" >> $NE
echo " Yadi - Image  " >> $NE
echo "Starte Neutrino" >> $NE
echo "XXXXXXXXXXXXXXXXXXXXXXXXXXX"
echo

# Benutzer Moeglichkeit zum Aendern von Patches, Logos etc. geben
read -p "Druecken Sie [RETURN] um noch Aenderungen in $CHANGE_DIR vorzunehmen -> " -t 10
if [ $? = 0 ]; then
 read -p "Druecken Sie [RETURN], wenn Sie die Aenderungen in $CHANGE_DIR abgeschlossen haben -> "
 # hier koennte jetzt noch das Packen und Sichern des veraenderten Archivs kommen
fi
}

config()
{
# Pfad zu mklibs setzen
export MKLIBS=$CVS/hostapps/mklibs/mklibs.py
# Nur konfigurieren, falls kein Fehler aufgetreten ist
if [ -e $RT/.mkheadall_error ];then
 echo
 echo "Beim letzten Durchgang ist ein Fehler aufgetreten"
 echo "Make-Vorgang wird an der letzten Position fortgesetzt"
 echo
else
  #  DBOX loeschen, falls es existiert, DBOX-Verzeichnis erzeugen
  if [ -d $DBOX ]; then
    rm -rf $DBOX
  fi
  mkdir $DBOX
  cd $DBOX

   if [ $RM_CVS = "yes" ]; then
    # Archive verschieben
    if test -d $CVS/cdk/Archive
    then
     mv $CVS/cdk/Archive $RT
    fi
   #Inhalt von CVS loeschen
   rm -rf $CVS
   mkdir $CVS
   # CVS runterladen
   cd $CVS
   cvs -d:pserver:anonymous@cvs.tuxbox.org:/cvs/tuxbox -z3 co .
   if test -d $RT/Archive
   then
   if  ! test -d $RT/Archive
    then
     mkdir $CVS/cdk
    fi
    mv $RT/Archive mv $CVS/cdk
   fi
 else
  cd $CVS
  cvs -d:pserver:anonymous@cvs.tuxbox.org:/cvs/tuxbox -z3 up -dP
 fi
 cd $CVS/cdk/

  ./autogen.sh
  ./configure --prefix=$DBOX --with-cvsdir=$CVS --enable-maintainer-mode --with-targetruleset=flash

  # Kernel-Verzeichnisse erstellen und Patches kopieren
  #make .linuxdir
  make .deps/linuxdir

  patch -N -p0 $CVS/cdk/linux-2.4.25/drivers/mtd/maps/dbox2-flash.c $CHANGE_DIR/Patches/dbox2-flash.c.diff

  # Neu: Aenderungen fuer 1xI schon im CVS
  cp Patches/u-boot.1x-flash.dbox2.h ../boot/u-boot/include/configs/dbox2.h

  # Die Datei aus dem CVS fuer JFFS2-Only patchen
  patch -N -p0 $CVS/boot/u-boot/include/configs/dbox2.h $CHANGE_DIR/Patches/dbox2.h.neu.diff
fi
}

make_it()
{
# Alles erstellen
cd $CVS/cdk
touch $RT/.mkheadall_error
make all

if test -e $CVS/cdk/.deps/nano
then
  echo Erstellen erfolgreich!
  rm $RT/.mkheadall_error
else
  echo FEHLER: make all abgebrochen!!
  exit 1
fi
}

flfs()
{
###########################
#
#         FLFS bauen
#
###########################
if [ ! -d $DBOX/flfs ]; then
 mkdir $DBOX/flfs
fi

# FLFS 1XI mit u-boot erzeugen
cd $CVS/hostapps/mkflfs/
gcc -o mkflfs mkflfs.c minilzo.c
cp $CVS/boot/u-boot/u-boot.stripped test
./mkflfs 1x
mv flfs.img $DBOX/flfs/flfs1x.img

# U-Boot fuer 2xI erzeugen
cp $CVS/cdk/Patches/u-boot.2x-flash.dbox2.h $CVS/boot/u-boot/include/configs/dbox2.h
patch -N -p0 $CVS/boot/u-boot/include/configs/dbox2.h $CHANGE_DIR/Patches/dbox2.h.2x.neu.diff
rm $CVS/boot/u-boot/u-boot.stripped
cd $CVS/cdk
rm .deps/u-boot
make u-boot

# FLFS fuer 2xI erzeugen
cd $CVS/hostapps/mkflfs/
rm test
cp $CVS/boot/u-boot/u-boot.stripped test
./mkflfs 2x
mv flfs.img $DBOX/flfs/flfs2x.img
echo
echo
echo flfs erstellt
echo
echo
}

neutrino()
{
##########################
#                        #
# Neutrino - Image bauen #
#                        #
##########################
cd $CVS/cdk
# Enigma-Dateien loeschen
rm -rf $DBOX/cdkflash/

# Neutrino ins Flash aufnehmen
make flash-neutrino
if ! test -e $DBOX/cdkflash/.part_neutrino
then
  echo FEHLER: make flash-neutrino abgebrochen!!
  exit 1
fi

# Plugins ins Flash aufnehmen
make flash-plugins
if ! test -e $DBOX/cdkflash/.part_plugins
then
  echo FEHLER: make flash-plugins abgebrochen!!
  exit 1
fi

# ftpd ins Flash aufnehmen
make flash-ftpd
if ! test -e $DBOX/cdkflash/.part_ftpd
then
  echo FEHLER: make flash-ftpd abgebrochen!!
  exit 1
fi

# telnetd ins Flash aufnehmen
make flash-telnetd
if ! test -e $DBOX/cdkflash/.part_telnetd
then
  echo FEHLER: make flash-telnetd abgebrochen!!
  exit 1
fi

# Udpstreampes ins Flash aufnehmen
cp $DBOX/cdkroot/sbin/udpstreampes $DBOX/cdkflash/root/sbin

# Top kopieren
cp $DBOX/cdkroot/bin/top $DBOX/cdkflash/root/bin

# rcs kopieren
patch -N -p0 $DBOX/cdkflash/root/etc/init.d/rcS $CHANGE_DIR/Patches/rcS.diff
# cp $CHANGE_DIR/Configs/rcS.local $DBOX/cdkflash/root/etc/init.d/

# MKLibs ausfuehren
make flash-lib
if ! test -e $DBOX/cdkflash/.lib
then
  echo FEHLER: make flash-lib abgebrochen!!
  exit 1
fi

# dvb-tools ins Flash aufnehmen
make flash-dvb-tools
if ! test -e $DBOX/cdkflash/.part_dvb_tools
then
  echo FEHLER: make flash-dvb-tools abgebrochen!!
  exit 1
fi

# Logos kopieren
mkdir $DBOX/cdkflash/root/var/tuxbox/boot
cp $CHANGE_DIR/Logos/logo-fb $DBOX/cdkflash/root/var/tuxbox/boot
cp $CHANGE_DIR/neutrino/logo-lcd $DBOX/cdkflash/root/var/tuxbox/boot

# ppcboot kopiern
cp $DBOX/cdkflash/root/boot/ppcboot.conf $DBOX/cdkflash/root/var/tuxbox/boot/boot.conf

# Iso kopieren
#mkdir $DBOX/cdkflash/root/share/iso-codes
# cp $DBOX/cdkroot/share/iso-codes/iso-639.tab $DBOX/cdkflash/root/share/iso-codes/

# XMLs kopieren
cp $DBOX/cdkroot/share/tuxbox/*.xml $DBOX/cdkflash/root/share/tuxbox/
patch -N -p0 $DBOX/cdkflash/root/share/tuxbox/cables.xml $CHANGE_DIR/Patches/cables.xml.diff

# Interfaces loeschen
rm $DBOX/cdkflash/root/etc/network/interfaces

# Start.neutrino kopieren
patch -N -p0 $DBOX/cdkflash/root/etc/init.d/start $CHANGE_DIR/neutrino/start.diff
patch -N -p0 $DBOX/cdkflash/root/etc/init.d/start_neutrino $CHANGE_DIR/neutrino/start_neutrino.diff
chmod a+x $DBOX/cdkflash/root/etc/init.d/start_neutrino

# Root-Home erstellen
# mkdir $DBOX/cdkflash/root/root

# lcdip kopieren
cp $CHANGE_DIR/bin/lcdip $DBOX/cdkflash/root/bin/

# Ucode-Checker kopieren
cp $CHANGE_DIR/bin/chkucodes $DBOX/cdkflash/root/sbin/

# Image-Infos kopieren, falls existent
if test -e $VER.neutrino
then
  cp $VER.neutrino $DBOX/cdkflash/root/.image_version
fi

# Teletext auf blaue Taste legen
mv $DBOX/cdkflash/root/lib/tuxbox/plugins/tuxtxt.cfg $DBOX/cdkflash/root/lib/tuxbox/plugins/_tuxtxt.cfg
mv $DBOX/cdkflash/root/lib/tuxbox/plugins/tuxtxt.so $DBOX/cdkflash/root/lib/tuxbox/plugins/_tuxtxt.so

# Ein paar Dateien linken loeschen
cd $DBOX/cdkflash/root/sbin/

#rm telnetd
#ln -s ../bin/busybox telnetd

#rm inetd
#ln -s ../bin/busybox inetd

ln -s ../bin/busybox ../bin/ps

GUI="neutrino" # Der Zusammenbau der Images ist jetzt in cat_images
}

enigma()
{
########################
#                      #
# Enigma - Image bauen #
#                      #
########################

# Neutrino-Dateien loeschen
rm -rf $DBOX/cdkflash/

# Enigma ins Flash aufnehmen
cd $CVS/cdk
make flash-enigma
if ! test -e $DBOX/cdkflash/.part_enigma
then
  echo FEHLER: make flash-enigma abgebrochen!!
  exit 1
fi

# Plugins ins Flash aufnehmen
make flash-plugins
if ! test -e $DBOX/cdkflash/.part_plugins
then
  echo FEHLER: make flash-plugins abgebrochen!!
  exit 1
fi

# ftpd ins Flash aufnehmen
make flash-ftpd
if ! test -e $DBOX/cdkflash/.part_ftpd
then
  echo FEHLER: make flash-ftpd abgebrochen!!
  exit 1
fi

# telnetd ins Flash aufnehmen
make flash-telnetd
if ! test -e $DBOX/cdkflash/.part_telnetd
then
  echo FEHLER: make flash-telnetd abgebrochen!!
  exit 1
fi

# Strace kopieren
cp $DBOX/cdkroot/bin/strace $DBOX/cdkflash/root/bin

# Fonts kopieren
cp $DBOX/cdkroot/share/fonts/* $DBOX/cdkflash/root/share/fonts/

# Scart.conf kopieren
cp $DBOX/cdkroot/var/tuxbox/config/scart.conf $DBOX/cdkflash/root/var/tuxbox/config/

# Udpstreampes ins Flash aufnehmen
cp $DBOX/cdkroot/sbin/udpstreampes $DBOX/cdkflash/root/sbin

# Top kopieren
cp $DBOX/cdkroot/bin/top $DBOX/cdkflash/root/bin

# rcs kopieren
patch -N -p0 $DBOX/cdkflash/root/etc/init.d/rcS $CHANGE_DIR/Patches/rcS.diff
cp $CHANGE_DIR/Configs/rcS.local $DBOX/cdkflash/root/etc/init.d/

# Locales kopieren
#cp $DBOX/cdkroot/bin/locale $DBOX/cdkflash/root/bin/
#cp $DBOX/cdkroot/bin/localedef $DBOX/cdkflash/root/bin/

# MKLibs ausfuehren
make flash-lib
if ! test -e $DBOX/cdkflash/.lib
then
  echo FEHLER: make flash-lib abgebrochen!!
  exit 1
fi

# genlocales kopieren
#cp $CHANGE_DIR/enigma/genlocales $DBOX/cdkflash/root/bin

# Sprachen installieren
cp -r $CHANGE_DIR/enigma/lib/ $DBOX/cdkflash/root
cd $CVS/apps/tuxbox/enigma/po/
make install
mkdir $DBOX/cdkflash/root/share/locale
cp -r $DBOX/cdkroot/share/locale/de/ $DBOX/cdkflash/root/share/locale/
cp -r $DBOX/cdkroot/share/locale/fr/ $DBOX/cdkflash/root/share/locale/
cp $CHANGE_DIR/Configs/locales $DBOX/cdkflash/root/share/locale/

# dvb-tools ins Flash aufnehmen
cd $CVS/cdk
make flash-dvb-tools
if ! test -e $DBOX/cdkflash/.part_dvb_tools
then
  echo FEHLER: make flash-dvb-tools abgebrochen!!
  exit 1
fi

# Logos kopieren
mkdir $DBOX/cdkflash/root/var/tuxbox/boot
cp $CHANGE_DIR/Logos/logo-fb $DBOX/cdkflash/root/var/tuxbox/boot
cp $CHANGE_DIR/enigma/logo-lcd $DBOX/cdkflash/root/var/tuxbox/boot

# ppcboot kopiern
cp $DBOX/cdkflash/root/boot/ppcboot.conf $DBOX/cdkflash/root/var/tuxbox/boot/boot.conf

# XMLs kopieren
cp $DBOX/cdkroot/share/tuxbox/*.xml $DBOX/cdkflash/root/share/tuxbox/
patch -N -p0 $DBOX/cdkflash/root/share/tuxbox/cables.xml $CHANGE_DIR/Patches/cables.xml.diff

# Interfaces loeschen
rm $DBOX/cdkflash/root/etc/network/interfaces

# Start kopieren
patch -N -p0 $DBOX/cdkflash/root/etc/init.d/start $CHANGE_DIR/enigma/start.diff
chmod a+x $DBOX/cdkflash/root/etc/init.d/start_enigma

# Root-Home erstellen
#mkdir $DBOX/cdkflash/root/root

# lcdip kopieren
cp $CHANGE_DIR/bin/lcdip $DBOX/cdkflash/root/bin/

# Ucode-Checker kopieren
cp $CHANGE_DIR/bin/chkucodes $DBOX/cdkflash/root/sbin/

# Image-Infos kopieren, falls existent
if test -e $VER.enigma
then
  cp $VER.enigma $DBOX/cdkflash/root/.image_version
fi

# Teletext auf blaue Taste legen
mv $DBOX/cdkflash/root/lib/tuxbox/plugins/tuxtxt.cfg $DBOX/cdkflash/root/lib/tuxbox/plugins/_tuxtxt.cfg
mv $DBOX/cdkflash/root/lib/tuxbox/plugins/tuxtxt.so $DBOX/cdkflash/root/lib/tuxbox/plugins/_tuxtxt.so

# Ein paar Dateien loeschen und verlinken
cd $DBOX/cdkflash/root/sbin/

#rm telnetd
#ln -s ../bin/busybox telnetd

#rm inetd
#ln -s ../bin/busybox inetd

ln -s ../bin/busybox ../bin/ps

# /var/tuxbox/config/enigma erstellen
mkdir $DBOX/cdkflash/root/var/tuxbox/config/enigma

GUI="enigma" # Der Zusammenbau der Images ist jetzt in cat_images
}

cat_image()
{
# Image zusammenbauen aus flfs und root-jffs2
# ist jetzt fuer beide GUIs zusammengefasst um die (existierenden) Fehler besser eingrenzen zu koennen
echo
echo
fakeroot mkfs.jffs2 -b -e 0x20000 --pad=0x7c0000 -r $DBOX/cdkflash/root/ -o $IMAGES/$GUI.root-jffs2.tmp
read -p "Druecken Sie [RETURN] um fortzusetzen (automagisch nach 10 s) -> " -t 10 #um Zeit f?r fakeroot zu gewinnen
if [ ! -e $IMAGES/$GUI.root-jffs2.tmp ]; then
 echo "$IMAGES/$GUI.root-jffs2.tmp nicht gefunden"
else
 if [ ! -e $DBOX/flfs/flfs1x.img ]; then
  echo "$DBOX/flfs/flfs1x.img nicht gefunden"
 else
  cat $DBOX/flfs/flfs1x.img $IMAGES/$GUI.root-jffs2.tmp >$IMAGES/$IMG_PRE$GUI"_head_1x.img"
  echo $IMG_PRE$GUI"_head_1x.img erstellt"
 fi
 if [ ! -e $DBOX/flfs/flfs2x.img ]; then
  echo "$DBOX/flfs/flfs2x.img nicht gefunden"
 else
  cat $DBOX/flfs/flfs2x.img $IMAGES/$GUI.root-jffs2.tmp >$IMAGES/$IMG_PRE$GUI"_head_2x.img"
  echo $IMG_PRE$GUI"_head_2x.img erstellt"
 fi
fi
echo
echo
}
#MAIN
prepare
config
make_it
flfs

neutrino
cat_image

enigma
cat_image

echo
echo
echo Images erstellt!
echo Diese sollten sich jetzt in $IMAGES befinden.
echo
echo
exit;

