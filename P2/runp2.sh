#!/bin/bash
for qS in 2000 8000 32000 64000 
do
	for min in 5 30 60  
	do
		for max in 15 90 180
		do
			for qw in 0.25 0.002 0.00
 			do
				./waf --run "scratch/p2v2 --red_dt=RED --minTh=$min --maxTh=$max --qw=$qw --queueSize=$qS"
			done
		done
	done
done