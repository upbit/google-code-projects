#!/bin/bash

export PATH=/home/tmake-1.8/bin:$PATH
export TMAKEPATH=/home/tmake-1.8/lib/linux-g++

export APP=dual_faygate

progen -n $APP -o $APP.pro -d ./ 
tmake $APP.pro > Makefile
rm *.pro
