$Id: README.txt,v 1.1 2004/10/07 13:12:38 essu Exp $

genpsi (Autor: gmo18t) patcht mit Neutrino-File-Recording aufgenommene TS-Streams, 
damit diese auch unter Enigma abspielbar sind.

genpsi - Linux-Version
genpsi.exe - Win-Version
dbox2genpsi - Dbox2-Version
genpsi.c - Source
README.txt - dieses Readme

Compile:
- For DBox just use 'make && make install'
- For X86 just use 'make linux'
- Use 'make clean' to cleanup 


Usage: 

Linux: ./genpsi <ts-file-to-patch> 
Windows: genpsi <ts-file-to-patch> 
Dbox2: /var/bin/dbox2genpsi /hdd/movie/<ts-file-to-patch> 

Es werden alle vorhanden A/V PIDs aufgelistet, 
dann werden die ersten 188er Packets (je nach Anzahl der gefundenen Audiospuren) 
des Files mit entsprechendem PAT/PMT gepatcht. 

Kompiliert werden kann das Programm mit:

gcc -o genpsi genpsi.c 

Linux:'#define WIN_COMPILE' muss auskommentiert sein: //#define WIN_COMPILE
Win: '#define WIN_COMPILE' muss unkommentiert sein: #define WIN_COMPILE


