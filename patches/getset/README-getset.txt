getset - Settings aus dem Internet auf die Box downloaden v $Revision: 1.1 $

Enigma / Neutrino Komplettpaket

Mit diesem Plugin lassen sich vorsortierte Satelliten-Settings von dewi-sat auf die Box laden und installieren.
Der Download dauert nur einen Bruchteil der Zeit eines Sat-Scans (Unter Neutrino f�r Astra und Hotbird ca 1 min) und
liefert (fast) alle aktuellen Services.

INSTALLATION:
Dateien entpacken und alle (getset, getset.1 - getset.10) per FTP nach /var/plugins auf die DBox/DreamBox �bertragen. Ausf�hrungsrechte f�r getset auf 500.
Wenn getset zum ersten Mal installiert wird, per telnet /var/plugins/getset ausf�hren, wenn getset schon installiert war, einfach das Plugin "Download Settings" ausf�hren.
Eine settings.cfg wird nicht mitgeliefert, kann aber per telnet-Aufruf  'getset -cs' erstellt werden und per telnet-Aufruf  'getset -cu' anstelle der Config-Bouquets benutzt werden.

BEDIENUNG:

Diese Version von getset kann als Plugin "Settings-Download" oder per telnet ausgef�hrt werden.

Neutrino:
Eine Einstellung der Satelliten, Sortorder und Programmoptionen kann in "CONFIG"-Bouquets vorgenommen werden.
Dazu muss in die Bouquetverwaltung gewechselt werden und in den Bouquets getsetSAT, getsetSORT und getsetCONFIG durch Verschieben eine Auswahl getroffen werden.
Anschliessend das Plugin "Settings-Download" ausgef�hren.

Enigma:
Eine Einstellung der Satelliten, Sortorder und Programmoptionen kann in den File-Bouquets SAT, SORT , CONFIG und EXPERT vorgenommen werden.
Dazu muss im Verschiebe-Modus die Auswahl getroffen werden.
Anschliessend das Plugin "Settings-Download" ausgef�hren.
Sollte die Box nach etwa ca 1 Min pro gew�hlter Satellit nicht reagieren, Box resetten.
Nach dem Neustart m�ssten die neuen Settings vorhanden sein.
Vor dem n�chsten Settings-Download sollten Sie die Option RELOAD deaktivieren und REBOOT aktivieren.


per telnet:
getset kann nach wie vor auch als shell-script ausgef�hrt werden. Einfach /var/plugins/getset eingeben.
Die Option REBOOT unter Enigma ist dann nicht notwendig, stattdessen sollte RELOAD gew�hlt werden.

settings.cfg:
getset arbeitet zun�chst mit programm-internen Voreinstellungen (z.B. SAT1=ASTRA_19_2), diese sollen eine Grundfunktionalit�t sichern.
Wenn es per telnet mit der Option -cs werden die settings in /var/tuxbox/config/settings.cfg gespeichert. Ein weiterer Aufruf mit der Option -cu l�dt /var/tuxbox/config/settings.cfg und alle darin gespeicherten Einstellungen werden �bernommen.
Beim Aufruf aus Enigma werden die Config-Bouquets ausgewertet und mit den dort gefundenen Einstellungen alle bisherigen Werte �berschrieben, es werden also keine Einstellungen aus der settings.cfg genommen.


In beiden GUIs wird der freie Speicherplatz gepr�ft. Sollte kein Download der Settings erfolgt sein, kann es daran liegen, dass der Speicherplatz zu gering ist.
Allerdings ist dieses Feuture nur f�r Astra und Hotbird getestet, f�r jeden Satelliten werden ca. 50 k veranschlagt, insbesondere die Option ALLSATS sollte nicht gew�hlt werden, wenn nicht sichergestellt ist, dass gen�gend freier Speicher vorhanden ist.

ACHTUNG:
Das Programm befindet sich noch in einer sehr fr�hen Version. So wird der z.B. der verf�gbare Speicherplatz auf der Box zwar gepr�ft, aber sollte der freie Speicherplatz f�r Services und Bouquets dennoch zu gering sein, kann ein Neuflashen des Images notwendig werden.
F�r diesen und andere Fehler �bernehmen wir keine Haftung.

Solltet ihr Fehler finden oder Verbesserungsvorschl�ge haben wendet euch an den Autor oder ans Forum: http://its.no-enigma.de/de

CHANGELOG:
v.0.06:
date 200403061203
telnet:
new Options -cs -cu to save and load config if called via telnet
Enigma:
new File-Bouquet Expert,
new Options:
KEEP_MARKED to keep '+'-marked Bouquets
NO_USERBOUQUETS to keep all Userbouquets
NEWSORT_ONLY to keep Services and Provider and only change Sorting
Neutrino:
no special changes against v.0.05 Installation not recommended

CHANGELOG:
v.0.05:
date 200402161449
no more Reboot necessary with ENIGMA lucgas-image > 15.02.2004
new Features LOG, SAVELOG, ANNOUNCE to show message on the TV, log them in /tmp or save them in /var
improved Freespace-check
Intelsat 707 and Thor are now: ThorIntel_  >  Thor/Intelsat 0.8W
spellcheck ;)

v.0.04:
date 200402161224
(I hope it`s now) working with both Enigma and Neutrino
Neutrino:
no more need to reboot dbox to install programm or settings
added 'lost' satellites to SAT-CONFIG
Enigma:
dbox/dream need to reboot(maybe depending on imageversion) to install programm or settings
new Programm-CONFIG: dial_wait

v.0.04pre:
date 200402141545
special neutrino version
new: neutrino-bouquet-config, freespace check, version-check, dialin-wait

v.0.03a.1:
version-ID: v.0.03a
date 200402082130
plugin-version
auto-install, ENIGMA-FileMode-config

v.0.03a:
date 200402072100 - not available
first special ENIGMA-version
new: auto-install, FileMode-config
changed: reload-Function for enigma
bugs: not working called from a plugin

v.0.03:
early ftp-version
