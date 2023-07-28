#!/usr/bin/perl

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

my $trials = shift;
my $greptime = shift;
my $basedir = shift;
my $sync = shift;
my $dist = shift;
my $suser = shift;
my $k = shift;
my $rand = shift;
my $slp = shift;
my $attacker = shift;
my $victim = shift;

my ($nummazes, $numchains, $depth) = split(' ', `cat $basedir/params`);

my $wins = 0;
my $i;

system("date");

for ($i = 0; $i < $trials; $i++) {
    print("TRIAL: $i\n");
    system("uptime");
    # Flush buffer cache
    system("./runfor.pl $greptime \"find /usr -type f 2> /dev/null | xargs grep hlasdhfajdhf\" > /dev/null 2>&1");
    # Stupid solaris and linux
    system("ps -A 2>/dev/null | grep xargs 2>/dev/null | cut -c1-6 2>/dev/null | xargs kill > /dev/null 2>&1");
    system("uptime");
    my $status = system("$attacker 0 $sync $dist $basedir $nummazes $numchains $victim $suser $k $rand $slp > /dev/null 2>&1");
    if ($status == 0) {
	print ("TRIAL $i: WON\n");
	$wins = $wins + 1;
    } else {
	print ("TRIAL $i: LOST\n");
    }
}

system("date");
system("uname -a");
print "GREPTIME: $greptime\n";
print "ATTACKER: $attacker\n";
print "SYNCMETHOD: $sync\n";
print "DISTINGUISHERMETHOD: $dist\n";
print "MAZEDIR: $basedir\n";
print "NUMMAZES: $nummazes\n";
print "NUMCHAINS: $numchains\n";
print "DEPTH: $depth\n";
print "VICTIM: $victim\n";
print "SINGLEUSER: $suser\n";
print "K: $k\n";
print "RAND: $rand\n";
print "WAITMETHOD: $slp\n";
print "TRIALS: $trials\n";
print "WINS: $wins\n";
