#!/usr/bin/perl

use v5;
use strict;
use warnings;



open(FH,"<","cwnd.dat") or die "error happened: can't open file: $!";
open(WH,">","cwndextract.dat") or die "error happened: can't open file: $!";

while(<FH>){
	my @cwnd = split;
	$cwnd[1] = $cwnd[1] / 512;
	$cwnd[1] = $cwnd[1] % 60;
	print WH $cwnd[0],"\t\t", $cwnd[1],"\n";
}

close FH;
close WH;