# $Id: Makefile,v 1.1 2004/10/07 13:12:38 essu Exp $
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

PCCBINNAME		= dbox2genpsi
X86BINNAME		= genpsi

SRCS			= genpsi.c 
INSTALLDIR		= ~/yadi/dbox/cdkflash/root/bin
FTPCONNECT		= ftp://root:dbox2@dbox

COMPILERDIR		= ~/yadi/dbox/cdk/bin

CPPFLAGS		= -I../../../tuxbox-cvs/driver/include

CC			= $(COMPILERDIR)/powerpc-tuxbox-linux-gnu-gcc
CXX                     = $(COMPILERDIR)/powerpc-tuxbox-linux-gnu-g++
X86CXX			= g++
STRIP                   = $(COMPILERDIR)/powerpc-tuxbox-linux-gnu-strip

CCDEPMODE		= depmode=gcc3
CXXDEPMODE              = depmode=gcc3
CFLAGS			= -Wall -mcpu=823 -mmultiple -mstring -meabi -pipe -Os
DEFS			=
LDFLAGS			= -g
OBJS 			= $(SRCS:%.cpp=%.o)
DISTFILES 		= $(SRCS)

all: $(PCCBINNAME)

.c.o:
	$(CXX) -c -I. $(CPPFLAGS) $(DEFS) $(CFLAGS) $<

$(PCCBINNAME): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) $(LIBCRYPT) -o $@
	$(STRIP) $(PCCBINNAME) 

linux: $(X86BINNAME)

$(X86BINNAME): $(OBJS)
	$(X86CXX) $(LDFLAGS) $(OBJS) $(LIBCRYPT) -o $@

install: all
	cp $(PCCBINNAME) $(INSTALLDIR)/$(PCCBINNAME)

clean:
	rm -f $(PCCBINNAME) $(X86BINNAME) *.o

dbox: all
	( echo cd /var; echo bin; echo put $(PCCBINNAME); echo chmod 755 $(PCCBINNAME);echo bye;) | ftp $(FTPCONNECT)

