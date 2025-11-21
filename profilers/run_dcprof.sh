#!/usr/bin/env bash

# pwd=$(dirname $(realpath -s $BASH_SOURCE))
pwd=$(pwd)

SLEEPTIME=1
DCIP=10.0.0.248
OUTDIR=${pwd}/profilers/profiles
BINARY=bin/dRxLaX.elf
CDROMDIR=${pwd}/cdrom

mkdir -p ${OUTDIR}

run_dcprof() {
	OPTLEVEL=$1
	source /opt/toolchains/dc/kos/environ.sh

	cd ${pwd}
	TRACENAME=new
	DCPROF=1 DCTRACE=1 make clean
	SINGLEDEMO=11 DCPROF=1 OPTLEVEL=${OPTLEVEL} ENJ_CBASEPATH=/pc/dRxLaX make -j 44 ${BINARY}
	dc-tool-ip -t ${DCIP} -x ${BINARY} -m ${CDROMDIR}
	sh-elf-gprof ${pwd}/${BINARY}  ${CDROMDIR}/gmon.out > ${OUTDIR}/${TRACENAME}_0${OPTLEVEL}.txt


	# TRACENAME=old
	# DCPROF=1 DCTRACE=1 make clean
	# SINGLEDEMO=11 OLDCODE=1 DCPROF=1 OPTLEVEL=${OPTLEVEL} ENJ_CBASEPATH=/pc/dRxLaX make -j 44
	# sleep $SLEEPTIME
	# dc-tool-ip -t ${DCIP} -x ${pwd}/dRxLaX.elf -m ${CDROMDIR}
	# sh-elf-gprof dRxLaX.elf  ${CDROMDIR}/gmon.out > ${OUTDIR}/${TRACENAME}_0${OPTLEVEL}.txt
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
