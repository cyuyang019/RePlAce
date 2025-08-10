#!/usr/bin/env bash
# run_single.sh
# Usage:
#   ./run_single.sh -tc TESTCASE [-config CONFIGNAME] [-log LOGDIR]
#
# Example:
#   ./run_single.sh -tc ethmac -config config1 -log Log1

set -u
set -o pipefail
shopt -s nullglob

# Default values
FLAG_COMBOS=(
  ""
  "-cellswap"
  "-timing"
  "-cellswap -timing"
)
JOBS=4
TESTCASE=""
CONFIG_NAME=""
LOGDIR="Log"

while [[ $# -gt 0 ]]; do
  case "$1" in
    -tc|--testcase) TESTCASE="$2"; shift 2 ;;
    -config)        CONFIG_NAME="$2"; shift 2 ;;
    -log)           LOGDIR="$2"; shift 2 ;;
    -j|--jobs)      JOBS="$2"; shift 2 ;;
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done

if [[ -z "$TESTCASE" ]]; then
  echo "ERROR: You must specify -tc <testcase>" >&2
  exit 1
fi
if [[ -z "$CONFIG_NAME" ]]; then
  echo "ERROR: You must specify -config <name> (without .json)" >&2
  exit 1
fi

export TZ="Asia/Taipei"
mkdir -p "$LOGDIR"
timestamp() { date +"%m%d%H%M%S"; }

wait_for_slot() {
  while (( $(jobs -rp | wc -l) >= JOBS )); do
    wait -n || true
  done
}

flag_tag() {
  local s="$1"
  if [[ -z "$s" ]]; then
    echo "noflags"
  else
    echo "$s" | tr ' ' '_' | tr -d '-'
  fi
}

run_one() {
  local tc="$1"
  local cfg="$2"
  local combo="$3"

  local cfgbase
  cfgbase="$(basename "$cfg" .json)"
  local tag
  tag="$(flag_tag "$combo")"
  local ts
  ts="$(timestamp)"

  local logfile="${LOGDIR}/${tc}_${cfgbase}_${tag}_${ts}.log"

  local cadbin="./test/${tc}/${tc}_pl.in"
  local cadbglobal="./test/${tc}/${tc}_pl.global"
  local lib="./test/${tc}/NanGate_3D_slow.lib"
  local verilog="./test/${tc}/${tc}_3D.v"
  local sdc="./test/${tc}/${tc}.sdc"
  local outdir="./TTest"

  IFS=' ' read -r -a combo_arr <<< "$combo"

  {
    echo "==> START $(date '+%F %T %Z')"
    echo "TC=$tc CFG=$cfg FLAGS='$combo'"
    ./build/replace \
      -3D \
      -cadbin "$cadbin" \
      -cadbglobal "$cadbglobal" \
      -lib "$lib" \
      -verilog "$verilog" \
      -sdc "$sdc" \
      -config "$cfg" \
      -legalize \
      "${combo_arr[@]}" \
      -output "$outdir"
    rc=$?
    echo "EXIT_CODE=$rc"
    echo "==> END $(date '+%F %T %Z')"
    exit $rc
  } 2>&1 | tee "$logfile"

  rc=${PIPESTATUS[0]}
  if [[ $rc -eq 0 ]]; then
    mv -f "$logfile" "${logfile%.log}.OK.log"
    echo "[OK]   $tc $(basename "$cfg") $combo -> ${logfile%.log}.OK.log"
  else
    mv -f "$logfile" "${logfile%.log}.FAIL.log"
    echo "[FAIL] $tc $(basename "$cfg") $combo -> ${logfile%.log}.FAIL.log (rc=$rc)"
  fi
}

echo "JOBS=$JOBS  TZ=$TZ"
echo "Testcase:   $TESTCASE"
echo "Config:     $CONFIG_NAME"
echo "Log dir:    $LOGDIR"
echo "Flag combos:"
for c in "${FLAG_COMBOS[@]}"; do printf '  "%s"\n' "$c"; done
echo

cfg="./test/${TESTCASE}/${CONFIG_NAME}.json"
if [[ ! -f "$cfg" ]]; then
  echo "ERROR: config not found: $cfg" >&2
  exit 1
fi

for combo in "${FLAG_COMBOS[@]}"; do
  wait_for_slot
  echo "Launching: $TESTCASE | $(basename "$cfg") | '$combo'"
  run_one "$TESTCASE" "$cfg" "$combo" &
done

wait || true
echo
echo "=== Summary ==="
ok=$(ls "$LOGDIR"/*.OK.log 2>/dev/null | wc -l || echo 0)
fail=$(ls "$LOGDIR"/*.FAIL.log 2>/dev/null | wc -l || echo 0)
echo "OK:   $ok"
echo "FAIL: $fail"
echo "Logs -> $LOGDIR"
