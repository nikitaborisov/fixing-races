# "Copyright (c) 2003 The Regents of the University  of California.  
# All rights reserved.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose, without fee, and without written agreement is
# hereby granted, provided that the above copyright notice, the following
# two paragraphs and the author appear in all copies of this software.
# 
# IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
# OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
# CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
# ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
# PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
#
# Authors: Naveen Sastry (nks@cs), Rob Johnson (rtjohnso@cs), Nikita Borisov
#          (nikitab@cs)
# Date:    Sep 21, 2004

PLATFORM=$(shell uname)

ifeq "$(PLATFORM)" "SunOS"
LIBS=-lrt -lnsl -lsocket
endif

CFLAGS=-g -DPLATFORM_$(PLATFORM) $(LIBS)
CC=gcc

all: attacker victim speed 

attacker: attacker.c 

victim: victim.c kopen.c 

speed: kopen.c forkopen.c speed.c

clean: 
	rm -f victim speed attacker races.tar.gz

tar:
	cd ../../ && tar cfvz races/src/races.tar.gz \
	    races/src/Makefile \
	    races/src/README \
	    races/src/LICENSE \
	    races/src/attacker.c \
	    races/src/do_test.pl \
	    races/src/forkopen.c \
	    races/src/kopen.c \
	    races/src/mkdirs.pl \
	    races/src/open.h \
	    races/src/runfor.pl \
	    races/src/speed.c \
	    races/src/victim.c
