#!/usr/bin/perl

use v5;
use strict;
use warnings;

my $matchstring;
my $matchr;
my $acktime;
my $ackcount;

open(FH,"<","node0long.tr") or die "error happened: can't open file: $!";
open(WH,">","SEQr.dat") or die "error happened: can't open file: $!";


while(<FH>){
	$matchstring = $_;
	if($matchstring =~ /^r/){
		$matchr = $';
		$matchr =~ /\d+\.\d+|\d+/;
		$acktime = $&;
		print WH $acktime,"\t\t";
		if($matchr =~ /ns3::TcpHeader\s.*/){
			$ackcount = $&;
			$ackcount =~ s/.*Seq=(\d+)\sAck=.*/$1/;
			$ackcount = $ackcount / 512;
			$ackcount = $ackcount % 60;
			print WH $ackcount,"\n";

		}
	} 
}

close FH;
close WH;
