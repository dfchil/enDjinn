#!/usr/bin/env zsh

source /opt/toolchains/dc/kos/environ.sh

PATH=$PATH:/opt/toolchains/dc/bin
# kos-tool goes through tools/dctool/kos-tool, which queues clients for a console.
_dctool_dir=${${(%):-%x}:A:h}/tools/dctool
[[ ":$PATH:" == *":$_dctool_dir:"* ]] || PATH=$_dctool_dir:$PATH
unset _dctool_dir

mkdccdi () {
   mkdir -p cdrom
   mkdcdisc -n $1 -e $1.elf -d cdrom/ -N -o $1.cdi -v 3 -m
}

kosstyle () {
	astyle --style=attach -s4 -c -S -w -y -f -U -p -m0 -xn -xc -xl -xk -C -N -Y \
    -k3 -xL -xM -xQ -xP0 $1
}

export DCTOOL_HOST=10.0.0.248
export PFX_HOST=10.0.0.244

# This machine's settings and the PixelFX login live in environ.local.sh next
# to this file (not in git; see environ.local.sh.example). It may override the
# defaults above.
_enj_local=${${(%):-%x}:A:h}/environ.local.sh
[[ -r $_enj_local ]] && source $_enj_local
unset _enj_local

# Description:
# The ensure_dctool_ready function safely manages Dreamcast system readiness for
# development and testing, especially with PixelFX Retro Gem HDMI mods or similar
# setups. It performs the following steps:
#
# 1. Prompts for and exports required environment variables ($DCTOOL_HOST,
#    $PFX_USER, $PFX_PASSWD, $PFX_HOST) if not already set.
# 2. Uses kos-tool's STAT query, which proves the dc-load-ip loader is idle.
# 3. If $DCTOOL_HOST is not loader-ready, calls reset_dctool_wait to POST to the
#    PixelFX system-restart endpoint and wait for STAT.
# 4. Waits up to 30 seconds for $DCTOOL_HOST to return STAT after restart,
#    ensuring the loader is ready for an ELF upload.
#    Dreamcast is ready for further operations.
# 5. Returns success if the Dreamcast is online, or failure if it does not
#    respond within the timeout.
dctool_loader_ready() {
  local timeout_cmd
  for timeout_cmd in gtimeout timeout; do
    if command -v "$timeout_cmd" >/dev/null; then
      "$timeout_cmd" 2 kos-tool --ready -t "$1" >/dev/null 2>&1
      return
    fi
  done
  kos-tool --ready -t "$1" >/dev/null 2>&1
}

# Holds the console lock in this shell until dctool_unlock or the shell ends,
# so several kos-tool runs in a row keep the console (tools/dctool/kos-tool
# takes the lock for one run). Waits up to $DCTOOL_LOCK_WAIT seconds
# (default 900); returns 75 if the console stays in use.
dctool_lock() {
  [[ -n "$DCTOOL_LOCK_HELD" ]] && return 0
  zmodload zsh/system || return 70
  # A POSIX lock, lost if this shell closes any descriptor of the lock file;
  # who holds it goes in the .who file (see tools/dctool/kos-tool).
  local lock=/tmp/dctool-${DCTOOL_HOST:-default}.lock who=/tmp/dctool-${DCTOOL_HOST:-default}.who fd
  : >> "$lock" || return 73
  if ! zsystem flock -t 0 -f fd "$lock" 2>/dev/null; then
    echo "DCTOOL_HOST ($DCTOOL_HOST) is in use, waiting up to ${DCTOOL_LOCK_WAIT:-900} s for: $(<$who)"
    if ! zsystem flock -t ${DCTOOL_LOCK_WAIT:-900} -f fd "$lock" 2>/dev/null; then
      echo "DCTOOL_HOST ($DCTOOL_HOST) is still in use by: $(<$who)"
      return 75
    fi
  fi
  print -r -- "pid $$ in ${PWD/#$HOME/~} since $(date '+%H:%M:%S'): dctool_lock" > "$who"
  typeset -g DCTOOL_LOCK_FD=$fd
  export DCTOOL_LOCK_HELD=$$
}

dctool_unlock() {
  [[ -n "$DCTOOL_LOCK_FD" ]] && zsystem flock -u $DCTOOL_LOCK_FD
  unset DCTOOL_LOCK_FD DCTOOL_LOCK_HELD
}

# Lists running kos-tool clients ("pid args"), leaving out --ready probes.
dctool_clients() {
  ps -Ao pid=,args= | awk '$2 ~ /(^|\/)kos-tool$/ && !/ --ready/ { sub(/^ +/, ""); print }'
}

function reset_dctool_wait(){
  if [[ -z "$DCTOOL_HOST" ]]; then
    echo -n "Enter DCTOOL_HOST IP OR HOSTNAME: "
    read DCTOOL_HOST
    export DCTOOL_HOST
  fi
  if [[ -z "$PFX_USER" ]]; then
    echo -n "Enter pixel fx username: "
    read PFX_USER
    export PFX_USER
  fi
  if [[ -z "$PFX_PASSWD" ]]; then
    echo -n "Enter pixel fx password: "
    read -s PFX_PASSWD
    echo
    export PFX_PASSWD
  fi
  if [[ -z "$PFX_HOST" ]]; then
    echo -n "Enter host (IP OR HOSTNAME, e.g. 10.0.0.244): "
    read PFX_HOST
    export PFX_HOST
  fi
  if ! curl --fail --silent --show-error --digest -u "$PFX_USER:$PFX_PASSWD" \
      -X POST "http://$PFX_HOST/system-restart" >/dev/null; then
    echo "PixelFX system restart request failed."
    return 1
  fi
  echo "Waiting for DCTOOL_HOST ($DCTOOL_HOST) to return dc-load-ip STAT after system restart..."
  local timeout=30 started=$SECONDS deadline=$((SECONDS + timeout)) elapsed
  while dctool_loader_ready "$DCTOOL_HOST" >/dev/null; do
    elapsed=$((SECONDS - started))
    echo "Waiting for DCTOOL_HOST ($DCTOOL_HOST) to leave the old dc-load-ip session... ($elapsed seconds elapsed)"
    if (( SECONDS >= deadline )); then
      echo "Timeout: DCTOOL_HOST did not leave its old dc-load-ip session after restart."
      return 1
    fi
    sleep 1
  done
  started=$SECONDS
  deadline=$((SECONDS + timeout))
  while ! dctool_loader_ready "$DCTOOL_HOST" >/dev/null; do
    elapsed=$((SECONDS - started))
    echo "Waiting for DCTOOL_HOST ($DCTOOL_HOST) to return dc-load-ip STAT for $timeout seconds... ($elapsed seconds elapsed)"
    if (( SECONDS >= deadline )); then
      echo "Timeout: DCTOOL_HOST did not return dc-load-ip STAT within $timeout seconds after restart."
      return 1
    fi
    sleep 1
  done
  echo "DCTOOL_HOST ($DCTOOL_HOST) is dc-load-ip ready after restart."
  return 0
}

function ensure_dctool_ready(){
  if [[ -z "$DCTOOL_HOST" ]]; then
    echo -n "Enter DCTOOL_HOST IP OR HOSTNAME: "
    read DCTOOL_HOST
    export DCTOOL_HOST
  fi
  # Hold the console lock while checking and maybe restarting, unless the
  # caller already holds it; a queued session's run is never restarted.
  if [[ -z "$DCTOOL_LOCK_HELD" ]]; then
    dctool_lock || return $?
    _ensure_dctool_ready_locked
    local rc=$?
    dctool_unlock
    return $rc
  fi
  _ensure_dctool_ready_locked
}

function _ensure_dctool_ready_locked(){
  if ! command -v kos-tool >/dev/null; then
    echo "kos-tool with --ready is required."
    return 127
  fi
  # Only one local client may own dc-load-ip's TCP session. A client started
  # without the lock (another machine's PATH, an old shell) is never killed:
  # wait up to $DCTOOL_WAIT seconds (default 0) for it to finish, and report the
  # console busy if it does not. DCTOOL_FORCE=1 takes the console anyway.
  local clients wait_deadline=$((SECONDS + ${DCTOOL_WAIT:-0}))
  while clients=$(dctool_clients) && [[ -n "$clients" ]]; do
    if [[ "${DCTOOL_FORCE:-0}" == 1 ]]; then
      echo "DCTOOL_FORCE=1: stopping kos-tool clients:"
      echo "$clients"
      kill ${(f)"$(echo "$clients" | awk '{print $1}')"} 2>/dev/null || true
      sleep 1
      break
    fi
    if (( SECONDS >= wait_deadline )); then
      echo "DCTOOL_HOST ($DCTOOL_HOST) is busy; another kos-tool client is running:"
      echo "$clients"
      echo "Set DCTOOL_WAIT=<seconds> to wait for it, or DCTOOL_FORCE=1 to stop it."
      return 75
    fi
    sleep 2
  done
  echo "Checking dc-load-ip readiness on DCTOOL_HOST ($DCTOOL_HOST)..."
  # After a program exits, dc-load-ip can take more than 6 s to answer again;
  # restarting the console then (through PixelFX) is needless and also drops
  # the HDMI capture's audio. Wait up to $DCTOOL_READY_WAIT seconds first.
  local timeout=${DCTOOL_READY_WAIT:-20} started=$SECONDS deadline=$((SECONDS + timeout)) elapsed
  while ! dctool_loader_ready "$DCTOOL_HOST" >/dev/null; do
    elapsed=$((SECONDS - started))
    echo "Waiting for DCTOOL_HOST ($DCTOOL_HOST) to return dc-load-ip STAT for $timeout seconds... ($elapsed seconds elapsed)"
    if (( SECONDS >= deadline )); then
      echo "DCTOOL_HOST ($DCTOOL_HOST) did not return dc-load-ip STAT within $timeout seconds. Restarting."
      break
    fi
    sleep 1
  done
  if dctool_loader_ready "$DCTOOL_HOST" >/dev/null; then
    echo "DCTOOL_HOST ($DCTOOL_HOST) is verified idle in dc-load-ip. Skipping system restart."
    return 0
  fi
  reset_dctool_wait
}
