#!/bin/bash

	for min in 5 30 60 
	do
	       for qw in 0.25 0.002 0.0001 
		do
			for rtt in 10ms 30ms 50ms
			do
			  for maxp in 10 50 90
			    do
			    	./waf --run "scratch/p2v3 --red_dt=RED --minTh=$min --maxTh=$((min*3)) --qw=$qw --RTT=$rtt --maxP=$maxp"

				done
			done
		done
	done
