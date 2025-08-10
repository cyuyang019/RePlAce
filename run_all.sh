#!/usr/bin/env bash
# run_matrix.sh
# Usage:
#   ./run_matrix.sh [-j JOBS] [-config CONFIGNAME] [-log LOGDIR]

set -u
set -o pipefail
shopt -s nullglob

# --------- Your 6 testcases ----------
TESTCASES=(sha3 ethmac mor1kx tinyaes jpegencode or1200)
# -------------------------------------

# Four flag combinations per testcase+config
FLAG_COMBOS=(
  ""
  "-cellswap"
  "-timing"
  "-cellswap -timing"
)

JOBS=6
CONFIG_NAME=""
LOGDIR="Log"

while [[ $# -gt 0 ]]; do
  case "$1" in
    -j|--jobs) JOBS="$2"; shift 2 ;;
    -config)   CONFIG_NAME="$2"; shift 2 ;;
    -log)      LOGDIR="$2"; shift 2 ;;
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done

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
echo "Config name: $CONFIG_NAME"
echo "Log dir:     $LOGDIR"
echo "Flag combos:"
for c in "${FLAG_COMBOS[@]}"; do printf '  "%s"\n' "$c"; done
echo

# Launch jobs
for tc in "${TESTCASES[@]}"; do
  cfg="./test/${tc}/${CONFIG_NAME}.json"
  if [[ ! -f "$cfg" ]]; then
    echo "Warning: config not found: $cfg" >&2
    continue
  fi
  for combo in "${FLAG_COMBOS[@]}"; do
    wait_for_slot
    echo "Launching: $tc | $(basename "$cfg") | '$combo'"
    run_one "$tc" "$cfg" "$combo" &
  done
done

wait || true
echo
echo "=== Summary ==="
ok=$(ls "$LOGDIR"/*.OK.log 2>/dev/null | wc -l || echo 0)
fail=$(ls "$LOGDIR"/*.FAIL.log 2>/dev/null | wc -l || echo 0)
echo "OK:   $ok"
echo "FAIL: $fail"
echo "Logs -> $LOGDIR"
