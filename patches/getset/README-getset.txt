getset - Settings aus dem Internet auf die Box downloaden v $Revision: 1.1 $

Enigma / Neutrino Komplettpaket

Mit diesem Plugin lassen sich vorsortierte Satelliten-Settings von dewi-sat auf die Box laden und installieren.
Der Download dauert nur einen Bruchteil der Zeit eines Sat-Scans (Unter Neutrino für Astra und Hotbird ca 1 min) und
liefert (fast) alle aktuellen Services.

INSTALLATION:
Dateien entpacken und alle (getset, getset.1 - getset.10) per FTP nach /var/plugins auf die DBox/DreamBox übertragen. Ausführungsrechte für getset auf 500.
Wenn getset zum ersten Mal installiert wird, per telnet /var/plugins/getset ausführen, wenn getset schon installiert war, einfach das Plugin "Download Settings" ausführen.
Eine settings.cfg wird nicht mitgeliefert, kann aber per telnet-Aufruf  'getset -cs' erstellt werden und per telnet-Aufruf  'getset -cu' anstelle der Config-Bouquets benutzt werden.

BEDIENUNG:

Diese Version von getset kann als Plugin "Settings-Download" oder per telnet ausgeführt werden.

Neutrino:
Eine Einstellung der Satelliten, Sortorder und Programmoptionen kann in "CONFIG"-Bouquets vorgenommen werden.
Dazu muss in die Bouquetverwaltung gewechselt werden und in den Bouquets getsetSAT, getsetSORT und getsetCONFIG durch Verschieben eine Auswahl getroffen werden.
Anschliessend das Plugin "Settings-Download" ausgeführen.

Enigma:
Eine Einstellung der Satelliten, Sortorder und Programmoptionen kann in den File-Bouquets SAT, SORT , CONFIG und EXPERT vorgenommen werden.
Dazu muss im Verschiebe-Modus die Auswahl getroffen werden.
Anschliessend das Plugin "Settings-Download" ausgeführen.
Sollte die Box nach etwa ca 1 Min pro gewählter Satellit nicht reagieren, Box resetten.
Nach dem Neustart müssten die neuen Settings vorhanden sein.
Vor dem nächsten Settings-Download sollten Sie die Option RELOAD deaktivieren und REBOOT aktivieren.


per telnet:
getset kann nach wie vor auch als shell-script ausgeführt werden. Einfach /var/plugins/getset eingeben.
Die Option REBOOT unter Enigma ist dann nicht notwendig, stattdessen sollte RELOAD gewählt werden.

settings.cfg:
getset arbeitet zunächst mit programm-internen Voreinstellungen (z.B. SAT1=ASTRA_19_2), diese sollen eine Grundfunktionalität sichern.
Wenn es per telnet mit der Option -cs werden die settings in /var/tuxbox/config/settings.cfg gespeichert. Ein weiterer Aufruf mit der Option -cu lädt /var/tuxbox/config/settings.cfg und alle darin gespeicherten Einstellungen werden übernommen.
Beim Aufruf aus Enigma werden die Config-Bouquets ausgewertet und mit den dort gefundenen Einstellungen alle bisherigen Werte überschrieben, es werden also keine Einstellungen aus der settings.cfg genommen.


In beiden GUIs wird der freie Speicherplatz geprüft. Sollte kein Download der Settings erfolgt sein, kann es daran liegen, dass der Speicherplatz zu gering ist.
Allerdings ist dieses Feuture nur für Astra und Hotbird getestet, für jeden Satelliten werden ca. 50 k veranschlagt, insbesondere die Option ALLSATS sollte nicht gewählt werden, wenn nicht sichergestellt ist, dass genügend freier Speicher vorhanden ist.

ACHTUNG:
Das Programm befindet sich noch in einer sehr frühen Version. So wird der z.B. der verfügbare Speicherplatz auf der Box zwar geprüft, aber sollte der freie Speicherplatz für Services und Bouquets dennoch zu gering sein, kann ein Neuflashen des Images notwendig werden.
Für diesen und andere Fehler übernehmen wir keine Haftung.

Solltet ihr Fehler finden oder Verbesserungsvorschläge haben wendet euch an den Autor oder ans Forum: http://its.no-enigma.de/de

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
