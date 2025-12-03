#!/usr/bin/env bash


#set the KOS environtment variables
source /opt/toolchains/dc/kos/environ.sh

TARGET=enj_writing

cd examples/${TARGET}/
DCTRACE=1 ENJ_DEBUG=1 make clean
# SINGLEDEMO=0 DCPROF=1 SINGLEDEMO=7 
ENJ_SHOWFRAMETIMES=1 ENJ_DEBUG=1 OPTLEVEL=g make -j 44
exit