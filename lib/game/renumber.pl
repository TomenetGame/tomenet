#!/usr/bin/perl
# $Id: renumber.pl,v 1.1.1.1 2002/06/15 16:32:53 jir Exp $
#from http://www.fragment.com/~jl8e/angband/

# This script renumber the ID numbers in *_info.txt of *angband..
#
# ** WARNING **
# Since many of ID numbers are hard-coded, renumbering them often means
# corrupted savefiles and strange program behaviors. You are warned.

#inform the user of changes in numbers, so you can fix defines.h
$logchange = 0;

#produce no holes. Even without pack, holes can be closed up 
#by renumbering, especially if you use first.
$pack = 0;

#start numbering at $first
$first = 0;

while ($foo = shift @ARGV) {
	if ($foo =~ /_info.txt/) {
#It's a file
		print "Processing $foo\n";
		rename $foo, "$foo.orig";
		open OLDFILE, "<$foo.orig";
		open NEWFILE, ">$foo";
		$count = $first;
		while (<OLDFILE>) {
			if (/^N:(.*):.*/) {
				$num = $1;
				if (not $pack and $num =~ /[0-9]/) {
					if ($num > $count) {
						$count = $num;
					}
				}
				if ($num != $count) {
					if ($logchange) {
						print "change: $_"; }
					s/N:.*:/N:$count:/;
					if ($logchange) {
						print "to    : $_\n"; }
				}
				$count++;
			}
			print NEWFILE;
		}
		print "New max_index: $count \n";
		close OLDFILE;
		close NEWFILE;
	}
	else {
#It's an option
		$opt = 0;
		if ( $foo =~ /-l/) { $logchange = 1; $opt = 1; }
		if ( $foo =~ /-nl/) { $logchange = 0; $opt = 1; }
		if ( $foo =~ /-p/) { $pack = 1; $opt = 1; }
		if ( $foo =~ /-np/) { $pack = 0; $opt = 1; }
		if ( $foo =~ /-f([0-9]+)/) { $first = $1; $opt = 1; }
		if ( not $opt ) { print "Unknown option: $foo\n"; }
	}
}

