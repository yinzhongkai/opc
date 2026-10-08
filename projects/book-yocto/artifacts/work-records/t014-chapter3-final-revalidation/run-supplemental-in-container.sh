#!/usr/bin/env bash

set +e
set -o pipefail

POKY=/home/oops/workspace/poky
BUILD=/home/oops/workspace/build
LAYER=/home/oops/workspace/meta-tiger
BBLAYERS="$BUILD/conf/bblayers.conf"
LOCALCONF="$BUILD/conf/local.conf"
RUN_DIR=/tmp/chapter3-revalidation-supplemental
FRESH_REPO="$RUN_DIR/meta-tiger-fresh"
SUPPLEMENTAL_LOG="$RUN_DIR/supplemental.log"

rm -rf "$RUN_DIR"
mkdir -p "$RUN_DIR"
exec > >(tee "$SUPPLEMENTAL_LOG") 2>&1

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

restore_final_state() {
    set +e
    rm -f "$LAYER/recipes-bsp/tiger-probe_0.1.bb"
    rm -rf "$LAYER/recipes-bsp/tiger-probe"
    rm -rf "$FRESH_REPO"
    if [ -f "$RUN_DIR/bblayers.conf.initial" ]; then
        cp "$RUN_DIR/bblayers.conf.initial" "$BBLAYERS"
    fi
}

trap restore_final_state EXIT

printf '\n################################################################\n'
printf '## SUPPLEMENTAL VALIDATION METADATA\n'
printf '################################################################\n'
run_cmd "S0-01 date" date -Is
run_cmd "S0-02 user" id

printf '\n===== BEGIN S0-03 source oe-init-build-env =====\n'
printf '$ source %s/oe-init-build-env %s\n' "$POKY" "$BUILD"
source "$POKY/oe-init-build-env" "$BUILD" 2>&1
source_rc=$?
printf 'RC=%d\n' "$source_rc"
printf '===== END S0-03 source oe-init-build-env =====\n'
if [ "$source_rc" -ne 0 ]; then
    exit 90
fi

cp "$BBLAYERS" "$RUN_DIR/bblayers.conf.initial"
sha256sum "$BBLAYERS" > "$RUN_DIR/bblayers.initial.sha256"
sha256sum "$LOCALCONF" > "$RUN_DIR/localconf.initial.sha256"

printf '\n################################################################\n'
printf '## S1. CHAPTER 3.1 OFFICIAL LAYER DIRECTORY COMMANDS\n'
printf '################################################################\n'
run_cmd "S1-01 ls-meta-poky" ls "$POKY/meta-poky/"
run_cmd "S1-02 ls-meta-yocto-bsp" ls "$POKY/meta-yocto-bsp/"
run_cmd "S1-03 ls-meta" ls "$POKY/meta/"
run_cmd "S1-04 meta-poky-layer-conf" cat "$POKY/meta-poky/conf/layer.conf"
run_sh "S1-05 meta-poky-relevant-lines" "grep -nE '^(BBPATH|BBFILES|BBFILE_COLLECTIONS|BBFILE_PATTERN|BBFILE_PRIORITY|LAYERDEPENDS|LAYERSERIES_COMPAT)' '$POKY/meta-poky/conf/layer.conf'"

printf '\n################################################################\n'
printf '## S2. RECIPES-BSP PROBE: WRONG AND CORRECT DEPTH\n'
printf '################################################################\n'
run_sh "S2-01 baseline-registration" "grep -n -F '$LAYER' '$BBLAYERS'; bitbake-layers show-layers"
printf '%s\n' \
    'SUMMARY = "Temporary probe recipe for meta-tiger discovery"' \
    'LICENSE = "MIT"' \
    'LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"' \
    > "$LAYER/recipes-bsp/tiger-probe_0.1.bb"
run_sh "S2-02 wrong-depth-path" "find '$LAYER/recipes-bsp' -maxdepth 2 -type f -printf '%P\\n' | sort"
run_cmd "S2-03 wrong-depth-query" bitbake-layers show-recipes tiger-probe
mkdir -p "$LAYER/recipes-bsp/tiger-probe"
mv "$LAYER/recipes-bsp/tiger-probe_0.1.bb" "$LAYER/recipes-bsp/tiger-probe/"
run_sh "S2-04 correct-depth-path" "find '$LAYER/recipes-bsp' -maxdepth 3 -type f -printf '%P\\n' | sort"
run_cmd "S2-05 correct-depth-query" bitbake-layers show-recipes tiger-probe
rm -rf "$LAYER/recipes-bsp/tiger-probe"
run_sh "S2-06 probe-cleanup-and-meta-status" "find '$LAYER/recipes-bsp' -name '*tiger-probe*' -print; git -C '$LAYER' status --porcelain=v1 --branch"

printf '\n################################################################\n'
printf '## S3. FRESH GIT INITIALIZATION IN ISOLATED COPY\n'
printf '################################################################\n'
mkdir -p "$FRESH_REPO"
run_sh "S3-01 copy-skeleton-without-git" "cd '$LAYER' && tar --exclude=.git -cf - . | tar -C '$FRESH_REPO' -xf -"
run_sh "S3-02 verify-no-git-before-init" "test ! -e '$FRESH_REPO/.git'; rc=\$?; echo NO_GIT_RC=\$rc; find '$FRESH_REPO' -maxdepth 2 -mindepth 1 -printf '%P\\n' | sort; exit \$rc"
run_cmd "S3-03 fresh-git-init" git -C "$FRESH_REPO" init -b main
run_cmd "S3-04 set-local-author-name" git -C "$FRESH_REPO" config --local user.name "Chapter 3 Validation"
run_cmd "S3-05 set-local-author-email" git -C "$FRESH_REPO" config --local user.email "chapter3-validation@example.invalid"
run_sh "S3-06 show-local-author" "printf 'user.name='; git -C '$FRESH_REPO' config --local --get user.name; printf 'user.email='; git -C '$FRESH_REPO' config --local --get user.email"
run_cmd "S3-07 git-add" git -C "$FRESH_REPO" add .
run_cmd "S3-08 status-before-commit" git -C "$FRESH_REPO" status --short --branch
run_cmd "S3-09 initial-commit" git -C "$FRESH_REPO" commit -m "Initial meta-tiger layer skeleton"
run_cmd "S3-10 log-author-committer" git -C "$FRESH_REPO" log -1 --format=fuller
run_cmd "S3-11 status-after-commit" git -C "$FRESH_REPO" status --short --branch

printf '\n################################################################\n'
printf '## S4. ACTUAL CURRENT META-TIGER AUTHOR IDENTITY\n'
printf '################################################################\n'
run_cmd "S4-01 current-meta-head" git -C "$LAYER" rev-parse HEAD
run_cmd "S4-02 current-meta-author-committer" git -C "$LAYER" show -s --format=fuller HEAD
run_sh "S4-03 current-meta-local-identity" "printf 'user.name='; git -C '$LAYER' config --local --get user.name; printf 'user.email='; git -C '$LAYER' config --local --get user.email"

printf '\n################################################################\n'
printf '## S5. CLEANUP AND FINAL STATE\n'
printf '################################################################\n'
rm -rf "$FRESH_REPO"
rm -f "$LAYER/recipes-bsp/tiger-probe_0.1.bb"
rm -rf "$LAYER/recipes-bsp/tiger-probe"
cp "$RUN_DIR/bblayers.conf.initial" "$BBLAYERS"
run_sh "S5-01 temp-copy-deleted" "test ! -e '$FRESH_REPO'; rc=\$?; echo TEMP_COPY_ABSENT_RC=\$rc; exit \$rc"
run_sh "S5-02 final-registration" "grep -n -F '$LAYER' '$BBLAYERS'; bitbake-layers show-layers"
run_sh "S5-03 final-probe-check" "find '$LAYER/recipes-bsp' -name '*tiger-probe*' -print"
run_cmd "S5-04 final-meta-status" git -C "$LAYER" status --porcelain=v1 --branch
run_cmd "S5-05 final-poky-status" git -C "$POKY" status --porcelain=v1
run_sh "S5-06 final-config-hashes" "echo INITIAL_BBLAYERS; cat '$RUN_DIR/bblayers.initial.sha256'; echo FINAL_BBLAYERS; sha256sum '$BBLAYERS'; echo INITIAL_LOCALCONF; cat '$RUN_DIR/localconf.initial.sha256'; echo FINAL_LOCALCONF; sha256sum '$LOCALCONF'"

trap - EXIT
printf '\nSUPPLEMENTAL_LOG=%s\n' "$SUPPLEMENTAL_LOG"
exit 0
