Dies ist eine fuer das "TS files abspielen" optimierte Version des neutrino 
movieplayers. Die Funktionalitaet zum Streamen ueber VLC wurde 1:1
uebernommen. 


1. Versionen
  
 movieplayer.cpp, v. 1.94
 movieplayer.cpp, v. 1.19
 
 sollte mit CDKs ab 06/06/2004 kompilierbar sein und funktionieren.
 -> einfach anstelle der Original-Dateien kopieren und build durchfuehren.


2. Erweiterungen 

2.1 Live Streaming

 mit Hilfe spezieller "description files" ist es nun u.a. moeglich, 
 das aktuelle Programm eines Kanals von einer anderen DBox (ueber Netzwerk) 
 anzuschauen.

 Ein "descrition file" hat folgende Syntax:

   #DBOXSTREAM
   <name>=<ip-addresse>;<port>;<vpid>;<apid>;<kanal-Id>
 
 wobei die erste Zeile immer #DBOXSTREAM lauten muss !
 in den einzelnen Zeilen duerfen keine whitspaces verwendet werden.

 Bedeutung der einzelnen tags:
   
   <name>        = Name des Kanals 
   <ip-addresse> = server, der den Livestream lifert
   <port>        = service port auf dem der server "lauscht"
   <vpid>        = video Pid in hexadezimaler Form mit vorangestelltem "0x"
   <apid>        = audio Pid in hexadezimaler Form mit vorangestelltem "0x"
   <kanal-id>    = tsid/onid/sid aus services.xml (hexadezimal) zum
                   Umschalten des Kanals auf dem server oder 
                   0, wenn kein Umschaltkommando an server gesendet werden soll
 		  -1, wie 0 jedoch ist die Pausefunktion moeglich.

 Beispiel P1.ts:
    
   #DBOXSTREAM
   P1=192.168.xxx.xxx;31339;0x100;0x101;0x10023001a    

 Fuer jeden gewuenschten Kanal kann ein solches file angelegt werden. Das
 Zappen geht dann mit Hilfe des Filebrowser und Ausfahl des entsprechenden
 Files mit der OK Taste. "Pause" und "Springen" sind natuerlich nicht 
 moeglich. Alles weitere geht wie gewohnt.

 Der Kanalwechsel kann bis zu mehreren Sekunden dauern (solange 
 bleibt der Bildschirm schwarz). Wird "0" als <kanal-Id> verwendet, d.h.
 kein Kanal-Umschalten auf der Serverbox initiiert, koennen nur Kanaele
 des gleichen Transponders fehlerfrei gestreamt werden. Jedoch gibt's 
 zuweilen damit Probeme, so dass - wenn moeglich - die Variante mit 
 "Umschalten" bevorzugt werden sollte.

 Auch ein Umschalten auf der Serverbox innerhalb des gleichen Transponders
 funktiniert zwar grundsaetzlich waehrend des Streamings, kann aber bisweilen 
 auch Probleme bereiten. 

 Sollten Bild und Ton einmal asynchron laufen, so kann mit der "0"-Taste der
 Fernbedienung ein "resync" durchgefuehrt werden.

  
2.2 Playlisten 

 Eine andere Form von description files sind "playlist files", mit deren Hilfe
 mehrere einzelne Video files als eine Einheit abgespielt werden koennen.
 Ausserdem kann mit auf/ab Taste zwischen den einzelnen Filmen hin und her
 gesprungen werden.
 
 Ein "playlist file" har folgende Syntax:

   #DBOXPLAYLST
   <movie-file-1>
   <movie-file-2>
   ...

 wobei die erste Zeile immer #DBOXPLAYLST lauten muss !
 Die einzelnen Zeilen muessen die Namen der gewuenschten movie-files mit
 voller Pfadangabe enthalten.
  
 Beispiel BestOfList.ts:
  
   #DBOXPLAYLST
   /mnt/filme/film01.ts
   /mnt/filme/archiv/film02.ts

 Beim Wechseln von einem Film zum anderen kann der Bildschirm bis zu 
 mehreren Sekunden schwarz bleiben.

 
2.3 Parental control

 hierbei handelt es sich um eine recht simple Implementierung, die das
 Abspielen von FSK Filmen etc. nur berechtigten Personen erlaubt.

 Dafuer wird der Menuepunkt "PES abspielen (experimentell)" von neutrino
 zweckendfremdet (da die PES abspielen Funktion kmpl. rausgeflogen ist s.u.)

 Bei Auswahl dieses Menuepunktes wird dann vor dem Starten des  movieplayers 
 (im Unterschied zum normalen "TS file abspielen") noch eine 4-stellige
 PIN abgefragt (unsichtbar bei schwarzem Bildschirm).
 Bei richtiger Eingabe wird der movieplayer im "erweiterten" Modus
 ausgefuehrt, ansonsten erfolgt Ruecksprung ins Hauptmenue ...

 Beim Starten direkt ueber "TS File abspielen" wird der player im 
 "eingeschraenkten" Modus betrieben.

 Da bei jedem Start des Movieplayers das Script "/var/bin/parental.sh"
 mit dem Modus (0=eingeschraenkt/1=erweiteret) als Parameter aufgerufen wird, 
 kann der Anwender sich selbst eine Strategie implementieren, indem er ein
 entsprechendes Script entwirft, z.B. das unmounten/mounten eines "FSK"-Shares
 in Abhaengigkeit des Modus.
 
 Die Referenz PIN kann in dem file "/var/tuxbox/config/mpcode" hinterlegt
 werden, ansonsten wird defaultmaessig "2222" benutzt.
 Das file mpcode muss also eine Zeile mit 4 Ziffern enthalten, wobei  
 jedoch zur Repraesentation einer "0" ein ":" verwendet werden muss.

 
3. Verbesserungen 

 Hauptproblem des original movieplayers war das sporadische Auftreten eines
 dauerhaft schwarzen Bildchirmes nach dem minutenweise Springen, nach Pause 
 oder Auswahl eins neuen Filmes, was nur durch erneutes Springen (evtl.
 mehrmals) behoben werden konnte.

 Das sollte in diesem movieplayer code nicht mehr vorkommen -> erfolgreich 
 getestet auf "Sagem 1xI Avia600", keine Ahnung inwieweit andere DBox Typen 
 ueberhaupt von dieser Problematik betroffen sind ... 

 Das optimierte Verhalten kommt natuerlich auch voll bei den Erweiterungen
 (Live Streaming, Playlisten) zum Tragen, wenn zu einem anderen Kanal/Film
 gewechselt wird.

 Weiterhin wurde die Pause-Funktion verbessert, so dass nunmehr _sofort_ nach
 Betaetigen der (gelben) Pause-Taste, Bild und Ton _gleichzeitig_ angehalten
 werden.


4. Aenderungen

- coding fuer das "experimentelle PES abspielen" kmpl. entsorgt ... 
- kein schneller vor/ruecklauf mehr, weil funktioniert nicht optimal

