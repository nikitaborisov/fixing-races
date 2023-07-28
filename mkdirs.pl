#!/usr/bin/perl -w

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
# Date:    Oct 7, 2004


sub base62
{
    return $_[0] < 10 ? "$_[0]" : 
	$_[0] < 36 ? chr($_[0] + 97 - 10) : 
	chr($_[0] + 65 - 10);
}

sub forcelink
{
    ($old, $new) = @_;
    unlink($new);
    symlink($old, $new);
}

sub make1tree
{
    my ($target, $nlinks, $depth) = @_;
    my $PWD;
    my $i;
    my $j;
    my $c;
    my $c2;
    my $path;
    my $path2;
    my $last = "exit";
    my $first = "sentry";
    my $linkname = "lnk";

    chomp($PWD = `pwd`);
    forcelink($target, $last);
    for ($i = 0; $i < $nlinks; $i++) {
	chdir($PWD);
	mkdir("chain$i");
	chdir("chain$i");
	for ($j = 0; $j < $depth; $j++) {
	    mkdir("d");
	    chdir("d");
	}
	if ($i > 0) {
	    $j = $i-1;
	    $path2 = "chain$j/" . ("d/" x $depth);
	    forcelink("$PWD/$path2", "$linkname");
	} else {
	    forcelink("$PWD/$last", "$linkname");
	}
    }
    chdir($PWD);
    $i--;
    forcelink("chain$i/" . ("d/" x $depth), $first);
}

if ($#ARGV != 5) {
    print "mkdirs.pl <basedir> <ndirs> <nlinks> <depth> <realgood> <realbad>\n";
    print "  basedir:    base directory containing mazes\n";
    print "  ndirs:      number of mazes (i.e. number of rounds of strengthening)\n";
    print "  nlinks:     number of chains per maze\n";
    print "  depth:      depth of directory chains\n";
    print "  realpublic: good target file\n";
    print "  realsecret: bad target file\n";
    print "\n";
    print "Note: realpublic and realsecret must be relative to basedir\n";

    exit(0);
}

my $basedir = shift;
my $nmazes = shift;
my $nlinks = shift;
my $depth = shift;
my $realgood = shift;
my $realbad = shift;

system("mkdir -p $basedir");
chdir($basedir);

forcelink($realgood, "public");
forcelink($realbad, "secret");
forcelink("public", "target");

my $i;
for ($i = 0; $i < $nmazes; $i++) {
    mkdir("maze$i");
    chdir("maze$i");
    make1tree("../target", $nlinks, $depth);
    chdir("..");
}

forcelink("maze0", "activemaze");
system("echo $nmazes $nlinks $depth > params");
