#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
  echo "usage: $0 OUTPUT_HEADER BUILD_DIR" >&2
  exit 2
fi

output=$1
build_dir=$2
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
assembly="$build_dir/kos_abi_probe.s"
values="$build_dir/kos_abi_values.txt"

mkdir -p "$build_dir" "$(dirname -- "$output")"
kos-cc -std=gnu11 -S "$script_dir/kos_abi_probe.c" -o "$assembly"
sed -n 's/.*PC_ENDJINN_ABI \([A-Z0-9_]*\) #\([^\\]*\)\\n".*/\1 \2/p' \
  "$assembly" > "$values"

if [ ! -s "$values" ]; then
  echo "failed to extract KOS ABI values from $assembly" >&2
  exit 1
fi

{
  echo '#ifndef PC_ENDJINN_KOS_ABI_CONTRACT_GENERATED_H'
  echo '#define PC_ENDJINN_KOS_ABI_CONTRACT_GENERATED_H'
  echo
  echo '/* Generated from the installed Dreamcast KOS headers. Do not edit. */'
  while read -r name value; do
    printf '#define PC_ENDJINN_KOS_ABI_%s (%s)\n' "$name" "$value"
  done < "$values"
  echo
  echo '#endif'
} > "$output.tmp"
mv "$output.tmp" "$output"
