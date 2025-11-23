#!/bin/bash

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <binary> <symbol> [output]"
    exit 1
elif [ "$#" -lt 3 ]; then
    FILE=$(mktemp --suffix .asm -p /opt/toolchains/dc/disasm)
else
    FILE=$3
fi

sh-elf-gdb $1 -batch -ex 'disassemble/s '"$2" > $FILE

if [ $? -ne 0 ]; then
  exit 1
fi

# rewrite output to pipeline simulator format
# regex 0x[0-9a-f]+ <\+\d+>:\s(.*)

# then comments: ^([^\s])

#  dctrace -t cdrom/trace.bin dRxLaX.elf -v

# sh-elf-gprof dRxLaX.elf  cdrom/gmon.out   


code $FILE
