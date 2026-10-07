#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
[[ -f "$here/build/runtime/libMeltypeNative.so" ]] || { echo "Run fcitx5/build.sh first." >&2; exit 1; }
[[ -f "$here/build/libfcitx5-meltype.so" ]] || exit 1
if pgrep -u "$(id -u)" -x fcitx5 >/dev/null; then
 echo "Stop Fcitx5 before installation (fcitx5-remote -e), then rerun." >&2; exit 1
fi
# Fcitx's addon library directory can differ between distributions.
addon_dir="${FCITX5_ADDON_DIR:-$(pkg-config --variable=libdir Fcitx5Core)/fcitx5}"
sudo install -Dm755 "$here/build/libfcitx5-meltype.so" "$addon_dir/libfcitx5-meltype.so"
mkdir -p "$HOME/.local/share/meltype"
cp -a "$here/build/runtime/." "$HOME/.local/share/meltype/"
install -Dm644 "$here/meltype-addon.conf" "$HOME/.local/share/fcitx5/addon/meltype.conf"
install -Dm644 "$here/meltype-inputmethod.conf" "$HOME/.local/share/fcitx5/inputmethod/meltype.conf"
install -Dm644 "$here/../linux/icon.png" "$HOME/.local/share/icons/hicolor/48x48/apps/meltype.png"
echo "Installed. Log in again, add Meltype in fcitx5-configtool and select it."
