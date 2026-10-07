#!/bin/bash
set -eu
root=$(cd "$(dirname "$0")" && pwd)
testdir=$(mktemp -d)
trap 'rm -rf "$testdir"' EXIT
mkdir -p "$testdir/config/fcitx5" "$testdir/data"
cat > "$testdir/config/fcitx5/profile" <<'PROFILE'
[Groups/0]
Name=Default
Default Layout=us
DefaultIM=meltype
[Groups/0/Items/0]
Name=keyboard-us
[Groups/0/Items/1]
Name=meltype
[GroupOrder]
0=Default
PROFILE
export XDG_CONFIG_HOME="$testdir/config"
export XDG_DATA_HOME="$testdir/data"
export XDG_DATA_DIRS="$HOME/.local/share:/usr/local/share:/usr/share"
export MELTYPE_TEST_ROOT="$root"
dbus-run-session -- bash -c '
fcitx5 --disable=wayland,xim,notificationitem,notifications,ibusfrontend > /tmp/meltype-fcitx-isolated.log 2>&1 &
engine_pid=$!
trap "kill $engine_pid 2>/dev/null || true" EXIT
for step in {1..100}; do
 if fcitx5-remote --check; then break; fi
 sleep .05
done
python3 "$MELTYPE_TEST_ROOT/test-dbus.py"
'
