#!/usr/bin/env bash

# pwd=$(dirname $(realpath -s $BASH_SOURCE))
pwd=$(pwd)

DCIP=10.0.0.248
OUTDIR=${pwd}/profilers/profiles
BINARY=bin/dRxLaX.elf
CDROMDIR=${pwd}/cdrom

mkdir -p ${OUTDIR}

run_dtrace() {
	OPTLEVEL=$1
	source /opt/toolchains/dc/kos/environ.sh

	cd ${pwd}
	TRACENAME=new
	DCTRACE=1 make clean
	SINGLEDEMO=11 DCTRACE=1 OPTLEVEL=${OPTLEVEL} ENJ_CBASEPATH=/pc/dRxLaX make -j 44 ${BINARY}
	dc-tool-ip -t ${DCIP} -x ${BINARY} -m ${CDROMDIR}
	mv ${CDROMDIR}/trace.bin ${OUTDIR}/${TRACENAME}_0${OPTLEVEL}.bin
	dctrace -t ${OUTDIR}/${TRACENAME}_0${OPTLEVEL}.bin ${BINARY}
	sed 's,\.lto_priv\.,_lto_priv_,g' < graph.dot > ${TRACENAME}_O${OPTLEVEL}.dot

	# TRACENAME=old
	# DCTRACE=1 make clean
	# SINGLEDEMO=11 OLDCODE=1 DCTRACE=1 OPTLEVEL=${OPTLEVEL} ENJ_CBASEPATH=/pc make -j 44
	# dc-tool-ip -t 10.0.0.219 -x ${pwd}/dRxLaX.elf -m ${pwd}/cdrom/
	# mv ${pwd}/cdrom/trace.bin ${pwd}/profilers/traces/${TRACENAME}_0${OPTLEVEL}.bin
	# dctrace -t profilers/traces/${TRACENAME}_0${OPTLEVEL}.bin dRxLaX.elf
	# sed 's,\.lto_priv\.,_lto_priv_,g' < graph.dot > ${TRACENAME}_O${OPTLEVEL}.dot
}

run_dtrace 3
# run_dtrace 0


exit