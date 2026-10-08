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

mkdir -p "$RUN_DIR"
exec > >(tee "$RAW_LOG") 2>&1

section() {
    printf '\n################################################################\n'
    printf '## %s\n' "$1"
    printf '################################################################\n'
}

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
    rm -f "$LAYER/recipes-core/tiger-probe.bb"
    rm -rf "$LAYER/recipes-core/tiger-probe"
    if [ -f "$RUN_DIR/layer.conf.initial" ]; then
        cp "$RUN_DIR/layer.conf.initial" "$LAYER/conf/layer.conf"
    fi
    if [ -f "$RUN_DIR/bblayers.conf.initial" ]; then
        cp "$RUN_DIR/bblayers.conf.initial" "$BBLAYERS"
    fi
}

trap restore_final_state EXIT

section "RUN METADATA"
run_cmd "RUN-01 date" date -Is
run_cmd "RUN-02 uname" uname -a
run_cmd "RUN-03 container-user" id

section "ENVIRONMENT SETUP"
printf '\n===== BEGIN ENV-01 source oe-init-build-env =====\n'
printf '$ source %s/oe-init-build-env %s\n' "$POKY" "$BUILD"
source "$POKY/oe-init-build-env" "$BUILD" 2>&1
source_rc=$?
printf 'RC=%d\n' "$source_rc"
printf '===== END ENV-01 source oe-init-build-env =====\n'
if [ "$source_rc" -ne 0 ]; then
    printf 'FATAL: unable to initialize BitBake environment\n'
    exit 90
fi

cp "$BBLAYERS" "$RUN_DIR/bblayers.conf.initial"
cp "$LAYER/conf/layer.conf" "$RUN_DIR/layer.conf.initial"
sha256sum "$BBLAYERS" > "$RUN_DIR/bblayers.initial.sha256"
sha256sum "$LOCALCONF" > "$RUN_DIR/localconf.initial.sha256"

section "BASELINE INSPECTION AND RESTORATION DECISION"
run_cmd "BASE-01 poky-head" git -C "$POKY" rev-parse HEAD
run_cmd "BASE-02 poky-status" git -C "$POKY" status --porcelain=v1
run_cmd "BASE-03 meta-head" git -C "$LAYER" rev-parse HEAD
run_cmd "BASE-04 meta-branch" git -C "$LAYER" branch --show-current
run_cmd "BASE-05 meta-status" git -C "$LAYER" status --porcelain=v1
run_cmd "BASE-06 bblayers-hash" sha256sum "$BBLAYERS"
run_cmd "BASE-07 localconf-hash" sha256sum "$LOCALCONF"
run_cmd "BASE-08 layer-conf" nl -ba "$LAYER/conf/layer.conf"
run_sh "BASE-09 residue-scan" "find '$LAYER' -path '$LAYER/.git' -prune -o -name '*tiger-probe*' -print"
run_sh "BASE-10 registration-count" "grep -Fxc '  $LAYER \\' '$BBLAYERS'"

section "1. DIRECTORY SKELETON"
run_sh "T1-01 skeleton" "find '$LAYER' -path '$LAYER/.git' -prune -o -mindepth 1 -maxdepth 3 -printf '%y %P\\n' | sort"

section "2. COLLECTION TIGER: REMOVE, ADD, BBLAYERS, SHOW-LAYERS"
run_cmd "T2-01 remove-layer" bitbake-layers remove-layer "$LAYER"
run_sh "T2-02 bblayers-after-remove" "nl -ba '$BBLAYERS'; sha256sum '$BBLAYERS'"
run_cmd "T2-03 add-layer" bitbake-layers add-layer "$LAYER"
run_sh "T2-04 bblayers-after-add" "nl -ba '$BBLAYERS'; sha256sum '$BBLAYERS'"
run_cmd "T2-05 show-layers" bitbake-layers show-layers
run_sh "T2-06 expanded-collections" "bitbake -e core-image-minimal | sed -n 's/^BBFILE_COLLECTIONS=//p'"

section "3. EMPTY LAYER SHOW-RECIPES"
run_sh "T3-01 show-recipes-filtered" "bitbake-layers show-recipes 2>&1 | grep -i tiger"
run_sh "T3-02 show-recipes-tiger-probe-empty" "bitbake-layers show-recipes tiger-probe"

section "4. UNREGISTERED/REGISTERED BUILD COMPARISON"
run_cmd "T4-01 remove-layer" bitbake-layers remove-layer "$LAYER"
run_sh "T4-02 unregistered-bblayers" "sha256sum '$BBLAYERS'; grep -F '$LAYER' '$BBLAYERS'"
run_cmd "T4-03 unregistered-build" bitbake core-image-minimal
run_cmd "T4-04 add-layer" bitbake-layers add-layer "$LAYER"
run_sh "T4-05 registered-bblayers" "sha256sum '$BBLAYERS'; grep -F '$LAYER' '$BBLAYERS'"
run_cmd "T4-06 registered-build" bitbake core-image-minimal

section "5. FULL YOCTO-CHECK-LAYER"
run_cmd "T5-01 yocto-check-layer" yocto-check-layer "$LAYER"

section "6. INDEPENDENT GIT INIT, COMMIT, STATUS"
run_cmd "T6-01 git-init" git -C "$LAYER" init -b main
run_cmd "T6-02 git-add" git -C "$LAYER" add .
run_cmd "T6-03 git-commit" git -C "$LAYER" commit --allow-empty -m "Revalidate meta-tiger layer skeleton"
run_cmd "T6-04 git-log" git -C "$LAYER" log --oneline --decorate -3
run_cmd "T6-05 git-status" git -C "$LAYER" status --porcelain=v1 --branch

section "7. LAYERSERIES_COMPAT CASE ERROR AND BBLAYERS ROLLBACK"
run_cmd "T7-01 remove-layer-valid" bitbake-layers remove-layer "$LAYER"
cp "$BBLAYERS" "$RUN_DIR/bblayers.before-bad-add"
run_sh "T7-02 bblayers-before-bad-add" "sha256sum '$BBLAYERS'; nl -ba '$BBLAYERS'"
run_cmd "T7-03 set-bad-compat" sed -i 's/^LAYERSERIES_COMPAT_tiger = "scarthgap"$/LAYERSERIES_COMPAT_tiger = "Scarthgap"/' "$LAYER/conf/layer.conf"
run_sh "T7-04 bad-layer-conf" "grep -n '^LAYERSERIES_COMPAT_tiger' '$LAYER/conf/layer.conf'"
run_cmd "T7-05 add-layer-bad-compat" bitbake-layers add-layer "$LAYER"
cp "$BBLAYERS" "$RUN_DIR/bblayers.after-bad-add"
run_sh "T7-06 bblayers-after-bad-add" "sha256sum '$BBLAYERS'; nl -ba '$BBLAYERS'"
run_sh "T7-07 rollback-hash-and-content" "echo PRE; sha256sum '$RUN_DIR/bblayers.before-bad-add'; echo POST; sha256sum '$RUN_DIR/bblayers.after-bad-add'; cmp -s '$RUN_DIR/bblayers.before-bad-add' '$RUN_DIR/bblayers.after-bad-add'; rc=\$?; echo CMP_RC=\$rc; diff -u '$RUN_DIR/bblayers.before-bad-add' '$RUN_DIR/bblayers.after-bad-add'; diff_rc=\$?; echo DIFF_RC=\$diff_rc; exit \$rc"
run_cmd "T7-08 restore-good-layer-conf" cp "$RUN_DIR/layer.conf.initial" "$LAYER/conf/layer.conf"
run_cmd "T7-09 add-layer-restored" bitbake-layers add-layer "$LAYER"
run_cmd "T7-10 show-layers-restored" bitbake-layers show-layers

section "8. PROBE RECIPE WRONG DEPTH AND CORRECT DEPTH"
printf '%s\n' \
    'SUMMARY = "Chapter 3 probe"' \
    'LICENSE = "CLOSED"' \
    'do_install() {' \
    '    install -d ${D}${bindir}' \
    '}' > "$LAYER/recipes-core/tiger-probe_0.1.bb"
run_sh "T8-01 wrong-depth-file" "find '$LAYER/recipes-core' -maxdepth 2 -type f -printf '%P\\n' | sort"
run_sh "T8-02 wrong-depth-show-recipes-filtered" "bitbake-layers show-recipes 2>&1 | grep -i tiger"
run_sh "T8-03 wrong-depth-show-recipes-name" "bitbake-layers show-recipes tiger-probe"
rm -f "$LAYER/recipes-core/tiger-probe_0.1.bb"
mkdir -p "$LAYER/recipes-core/tiger-probe"
printf '%s\n' \
    'SUMMARY = "Chapter 3 probe"' \
    'LICENSE = "CLOSED"' \
    'do_install() {' \
    '    install -d ${D}${bindir}' \
    '}' > "$LAYER/recipes-core/tiger-probe/tiger-probe_0.1.bb"
run_sh "T8-04 correct-depth-file" "find '$LAYER/recipes-core' -maxdepth 3 -type f -printf '%P\\n' | sort"
run_cmd "T8-05 correct-depth-show-recipes" bitbake-layers show-recipes tiger-probe

section "9. CLEANUP AND FINAL STATE"
rm -rf "$LAYER/recipes-core/tiger-probe"
cp "$RUN_DIR/layer.conf.initial" "$LAYER/conf/layer.conf"
cp "$RUN_DIR/bblayers.conf.initial" "$BBLAYERS"
run_sh "T9-01 probe-cleanup-check" "find '$LAYER' -path '$LAYER/.git' -prune -o -name '*tiger-probe*' -print"
run_sh "T9-02 final-registration" "grep -n -F '$LAYER' '$BBLAYERS'; bitbake-layers show-layers"
run_sh "T9-03 final-layer-conf" "nl -ba '$LAYER/conf/layer.conf'"
run_cmd "T9-04 final-meta-head" git -C "$LAYER" rev-parse HEAD
run_cmd "T9-05 final-meta-status" git -C "$LAYER" status --porcelain=v1 --branch
run_cmd "T9-06 final-poky-status" git -C "$POKY" status --porcelain=v1
run_sh "T9-07 final-config-hashes" "echo INITIAL_BBLAYERS; cat '$RUN_DIR/bblayers.initial.sha256'; echo FINAL_BBLAYERS; sha256sum '$BBLAYERS'; echo INITIAL_LOCALCONF; cat '$RUN_DIR/localconf.initial.sha256'; echo FINAL_LOCALCONF; sha256sum '$LOCALCONF'"
run_cmd "T9-08 final-show-recipes-empty" bitbake-layers show-recipes tiger-probe

trap - EXIT
printf '\nRAW_LOG=%s\n' "$RAW_LOG"
exit 0
