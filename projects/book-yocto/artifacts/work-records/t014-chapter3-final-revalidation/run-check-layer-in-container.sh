#!/usr/bin/env bash

set +e
set -o pipefail

POKY=/home/oops/workspace/poky
BUILD=/home/oops/workspace/build
LAYER=/home/oops/workspace/meta-tiger
BBLAYERS="$BUILD/conf/bblayers.conf"
LOCALCONF="$BUILD/conf/local.conf"
RUN_DIR=/tmp/chapter3-revalidation
RAW_LOG="$RUN_DIR/raw.log"

exec > >(tee -a "$RAW_LOG") 2>&1

run_cmd() {
    local label="$1"
    shift
    printf '\n===== BEGIN %s =====\n' "$label"
    printf '$'
    printf ' %q' "$@"
    printf '\n'
    "$@" 2>&1
    local rc=$?
    printf 'RC=%d\n' "$rc"
    printf '===== END %s =====\n' "$label"
    return 0
}

run_sh() {
    local label="$1"
    local command="$2"
    printf '\n===== BEGIN %s =====\n' "$label"
    printf '$ %s\n' "$command"
    bash -o pipefail -c "$command" 2>&1
    local rc=$?
    printf 'RC=%d\n' "$rc"
    printf '===== END %s =====\n' "$label"
    return 0
}

printf '\n################################################################\n'
printf '## 5B. FULL YOCTO-CHECK-LAYER WITH TEST LAYER UNREGISTERED\n'
printf '################################################################\n'

source "$POKY/oe-init-build-env" "$BUILD" >/dev/null 2>&1
run_cmd "T5B-01 remove-layer-before-check" bitbake-layers remove-layer "$LAYER"
run_sh "T5B-02 verify-unregistered" "sha256sum '$BBLAYERS'; grep -F '$LAYER' '$BBLAYERS'"
run_cmd "T5B-03 yocto-check-layer-full" yocto-check-layer "$LAYER"
run_cmd "T5B-04 restore-registration" bitbake-layers add-layer "$LAYER"
run_sh "T5B-05 final-checks" "grep -n -F '$LAYER' '$BBLAYERS'; bitbake-layers show-layers; sha256sum '$BBLAYERS' '$LOCALCONF'; git -C '$POKY' status --porcelain=v1; git -C '$LAYER' status --porcelain=v1 --branch"

exit 0
