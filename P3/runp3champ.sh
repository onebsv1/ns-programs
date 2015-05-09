#!/bin/bash

	for rta in AODV OLSR
	do	
		for nod in 500 1000 
		do
	       		for tInt in 0.1 0.5 0.9
			do
			  
			    	./waf --run "scratch/p3v8 --nodes=$nod --trafficInt=$tInt --Ptx=500.0 --rtalgo=$rta" 1>> "output500.txt"

			done
		done
	done
