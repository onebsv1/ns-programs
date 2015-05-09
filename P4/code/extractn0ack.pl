#!/usr/bin/perl

use v5;
use strict;
use warnings;

my $matchstring;
my $matchr;
my $acktime;
my $ackcount;
my $ackplus;
my $ackminus;

open(FH,"<","node0long.tr") or die "error happened: can't open file: $!";
open(PH,">","ACKr.dat") or die "error happened: can't open file: $!";


while(<FH>){
	$matchstring = $_;


	if($matchstring =~ /^\+/){
		$matchr = $';
		$matchr =~ /\d+\.\d+|\d+/;
		$acktime = $&;
		print PH $acktime,"\t\t";
		if($matchr =~ /ns3::TcpHeader\s.*/){
			$ackcount = $&;
			$ackcount =~ s/.*Ack=(\d+)\sWin=.*/$1/;
			$ackcount = $ackcount / 512;
			$ackcount = $ackcount % 60;
			print PH $ackcount,"\n";

		}
	} 
}

close FH;
close PH;
