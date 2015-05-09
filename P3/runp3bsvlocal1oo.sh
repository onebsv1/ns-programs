#!/bin/bash

	for rta in AODV OLSR
	do	
		for nod in 100
		do
	       		for tInt in 0.1 0.5 0.9
			do
				for p in 10.0 100.0
				do
			  
			    	./waf --run "scratch/p3v12 --nodes=$nod --trafficInt=$tInt --Ptx=$p --rtalgo=$rta" 1>> "outputn100.txt"

			done
		done
	done
done
