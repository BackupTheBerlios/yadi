# $Id: y_patch.sh,v 1.3 2004/10/10 16:00:21 essu Exp $
# is part of yadi-script-pack
#
#
# Copyright (c) 2004 essu Germany. All rights reserved.
# Mail: essu@berlios.de
#
#
# aktuelle Versionen gibt es hier:
# $Source: /home/xubuntu/berlios_backup/github/tmp-cvs/yadi/Repository/scripts/y_patch.sh,v $
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
# Das ist noch eine Baustelle, nur damit ihr seht was euch erwartet

show_help()
{
echo "Usage: $CALLED_FROM [-p ORIGINAL DIFF]|[-c ORIGINAL NEW]|[-d ORIGINAL]|[-v ORIGINAL]|[-a]|[-h]|[-i]";
echo "-p: patch ORIGINAL with DIFF and save ORIGINAL";
echo "-c: copy NEW to ORIGINAL and save ORIGINAL";
echo "-d: delete ORIGINAL";
echo "-m: makedir DIR";
echo "-r: revert changes copying first saved ORIGINAL to original place";
echo "-a: revert all changes copying first saved ORIGINALs to original places";
echo "-i: initalize - remove patches, patch.list, patch.log";
echo "-h: this help";
exit;
}

check_option()
{
case $OPTION in
 -i)initialize;;
 -p)patch_it;;
 -c)copy_it;;
 -d)delete;;
 -m)makedir;;
 -r)restore_original;;
 -a)revert_all;;
 -h)show_help;;
 *)echo "wrong usage: $ARG"; show_help;;
esac
}

initialize()
{
DELETE=`ls $ROOT/patches/*patch`
for FILE in $DELETE
do
{
 echo $FILE will be deleted
 if [ -e $FILE ]; then
  rm $FILE
 fi
 }
done

rm $ROOT/repatch
mv $ROOT/repatch.list $ROOT/$STAMP.repatch.list
mv $ROOT/patch.log $ROOT/$STAMP.patch.log
echo "# Repatch-List $STAMP" >$ROOT/repatch.list
echo "# Patch-Log $STAMP" >$ROOT/patch.log
}

former_patch()
{
echo $PATCH applied to $ORIGINAL >>$ROOT/patch.log
NR=`grep -c "$ORIGINAL >>" $ROOT/repatch.list`
if [ $? = 0 -a $NR -gt 0 ]; then #THX to Marsellus
 echo "$ORIGINAL wurde schon vorher gepatched" >>$ROOT/patch.log
else
 if [ -e $ROOT/patches/$STAMP.patch ]; then
  ( echo "cp $ROOT/patches/$STAMP.patch $ORIGINAL >>$ROOT/patch.log" >>$ROOT/repatch.list ) >>$ROOT/patch.log
 else
  ( echo "rm $ORIGINAL >>$ROOT/patch.log" >>$ROOT/repatch.list ) >>$ROOT/patch.log
 fi
fi
if [ -e $ORIGINAL.orig ]; then
 diff -NEbBur $ORIGINAL.orig $ORIGINAL >$ORIGINAL.yadi.diff
 mv $ORIGINAL.yadi.diff $ROOT/diffs
fi
}

patch_it()
{
  ( patch -bNp0 $ORIGINAL $PATCH  && cp $ORIGINAL.orig $ROOT/patches/$STAMP.patch && former_patch ) >>$ROOT/patch.log
  if [ $? != 0 ]; then
   cat $ORIGINAL.rej >>$ROOT/patch.log;
   cat $ORIGINAL.rej
   echo -e "\nWarning: Der Patch wurde zurueckgewiesen !\n"
   echo "Patch....: $PATCH"
   echo -e "Original.: $ORIGINAL\n"
   if [ -f $EDITOR ]; then
    $EDITOR $ORIGINAL.rej &
    $EDITOR $ORIGINAL
   fi
   read -p "Druecken Sie [RETURN] wenn die Aenderungen abgeschlossen sind -> " -t 15
  fi
}

copy_it()
{
 if [ -e $ORIGINAL ]; then
  ( cp $ORIGINAL $ROOT/patches/$STAMP.patch
    [ -e $ORIGINAL.orig ] && cp $ROOT/patches/$STAMP.patch $ORIGINAL.orig.$STAMP || cp $ROOT/patches/$STAMP.patch $ORIGINAL.orig ) >>$ROOT/patch.log
 fi
 cp $PATCH $ORIGINAL && former_patch >>$ROOT/patch.log

# some debug output
echo "Source......: $PATCH"
echo "Destination.: $ORIGINAL"
#echo ""
SLEEP="no"
}

delete()
{
if [ -e $ORIGINAL ]; then
  ( mv $ORIGINAL $ROOT/patches/$STAMP.patch && former_patch  ) >>$ROOT/patch.log
fi
# some debug output
echo "File: $ORIGINAL deleted"
}

makedir()
{
 if [ ! -d $ORIGINAL ]; then
  ( mkdir -p $ORIGINAL && former_patch  ) >>$ROOT/patch.log
  echo "Creating DIR: $ORIGINAL"
 fi
 SLEEP="no"
}

restore_original()
{
 ( grep $ORIGINAL $ROOT/repatch.list >$ROOT/repatch && . $ROOT/repatch && grep -v $ORIGINAL $ROOT/repatch.list >$ROOT/repatch.list ) >>$ROOT/patch.log
 if [ $? = 0 ]; then
  echo restore of $ORIGINAL failed >>$ROOT/patch.log;
 else
  echo $ORIGINAL restored >>$ROOT/patch.log;
 fi
}

revert_all()
{
 if [ -e $ROOT/repatch.list ]; then
  . $ROOT/repatch.list
  cat $ROOT/repatch.list >>$ROOT/patch.log
  rm $ROOT/repatch.list >>$ROOT/patch.log
 else
  echo "$ROOT/repatch.list not found" >>$ROOT/patch.log
 fi

 initialize
}

showall()
{
for FILE in $ORIGINAL $PATCH $ROOT/patches/*.patch $ROOT/repatch.list $ROOT/repatch $ROOT/patch.log
do
{
echo "---------------------------"
 if [ -e $FILE ]; then
  echo $FILE
  echo
  cat $FILE
 else
  echo $FILE does not exist
 fi
echo "---------------------------"
echo
 }
done
}

#sleep 1


STAMP=`date +%s`
CALLED_FROM="./y_patch.sh"

. ~/yadi/yadi-cvs/scripts/yadi.conf

echo "STAMP="$STAMP >>$ROOT/patch.log

if [ ! -d $ROOT/patches ]; then
 mkdir $ROOT/patches
fi
if [ ! -d $ROOT/diffs ]; then
 mkdir $ROOT/diffs
fi
OPTION=$1; ORIGINAL=$2; PATCH=$3

check_option
#echo showall

if [ "$SLEEP " = "no" ]; then
  SLEEP="no"
else
  sleep 1
fi;

#showall
exit;
