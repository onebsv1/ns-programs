#!/usr/bin/perl

use v5;
use strict;
use warnings;

open my $GP, '|-', 'gnuplot' or die "$! : Can't open .dat file";

print {$GP} <<'__GNUPLOT__';
     set terminal png size 1280,640
     set output "tahoe-1mod.png"     
     plot "ACKr.dat" using 1:2 title 'ACKs rx' with points, \
      "ACK+.dat" using 1:2 title 'ACKs q' with points, \
	  "SEQ+.dat" using 1:2 title 'SEQs q' with points, \
      "SEQr.dat" using 1:2 title 'SEQs tx' with points, \
      "cwndextract.dat" using 1:2 title 'Congestion Window' with linespoints
     exit
__GNUPLOT__

close $GP;