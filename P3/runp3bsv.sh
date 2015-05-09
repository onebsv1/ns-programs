#!/bin/bash

	for rta in AODV
	do	
		for nod in 1000 
		do
	       		for tInt in 0.1 0.5 0.9
			do
			  
			    	./waf --run "scratch/p3v13 --nodes=$nod --trafficInt=$tInt --Ptx=1.0 --rtalgo=$rta" 1>> "output1.txt"

			done
		done
	done
