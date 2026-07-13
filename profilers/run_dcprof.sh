#!/usr/bin/env bash

# pwd=$(dirname $(realpath -s $BASH_SOURCE))
pwd=$(pwd)

SLEEPTIME=1
DCIP=192.168.2.8
OUTDIR=${pwd}/profilers/profiles
BINARY=bin/
CDROMDIR=${pwd}/cdrom

mkdir -p ${OUTDIR}

run_dcprof() {
	OPTLEVEL=$1
	source /opt/toolchains/dc/kos/environ.sh

	cd ${pwd}
	TRACENAME=new
	ENJ_DCPROF=1 DCTRACE=1 make clean
	SINGLEDEMO=11 ENJ_DCPROF=1 OPTLEVEL=${OPTLEVEL} ENJ_CBASEPATH=/pc/endjinn_vr_colored_quads make -j 44 ${BINARY}
	dc-tool-ip -t ${DCIP} -x ${BINARY} -m ${CDROMDIR}
	sh-elf-gprof ${pwd}/${BINARY}  ${CDROMDIR}/gmon.out > ${OUTDIR}/${TRACENAME}_0${OPTLEVEL}.txt
}
# run_dcprof g
# sleep $SLEEPTIME

# run_dcprof 0
# sleep $SLEEPTIME

# run_dcprof 1
# sleep $SLEEPTIME

# run_dcprof 2
# sleep $SLEEPTIME

run_dcprof 3

exit
