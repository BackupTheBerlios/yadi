# $Id: Makefile,v 1.1 2004/10/07 12:11:27 essu Exp $
#
# Copyright (C) 2004 mogway <mogway@yadi.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
# Or, point your browser to http://www.gnu.org/copyleft/gpl.html
#
#

DIRS = lcdip

compile:
	for i in $(DIRS); do make -C $$i; done

install:
	for i in $(DIRS); do make -C $$i install; done

clean:
	for i in $(DIRS); do make -C $$i clean; done

