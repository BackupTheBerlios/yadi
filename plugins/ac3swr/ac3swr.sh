#!/bin/sh
# $Id: ac3swr.sh,v 1.1 2004/10/07 11:58:04 essu Exp $
#
if [ -e /tmp/.ac3off ]; then
  /bin/aviaext --iec-on
  rm /tmp/.ac3off
  cd /tmp
  /bin/wget -q "http://127.0.0.1/control/message?popup=Digitale%20Audio%20Ausgabe%20wurde%20eingeschaltet !!!"
else
  cd /tmp
  /bin/wget -q "http://127.0.0.1/control/message?popup=Bitte%20innerhalb%20von%2010%20Sekunden%20den%20Audioplayer%20starten"
  /bin/sleep 10
  /bin/aviaext --iec-off
  touch /tmp/.ac3off
  cd /tmp
  /bin/wget -q "http://127.0.0.1/control/message?popup=Digitale%20Audio%20Ausgabe%20wurde%20ausgeschaltet !!!"
fi;

rm /tmp/message?*
exit;

