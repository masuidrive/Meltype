#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
g++ -shared -fPIC -O2 -std=c++20 $(pkg-config --cflags Fcitx5Core jsoncpp) meltype.cpp -o build/libfcitx5-meltype.so $(pkg-config --libs Fcitx5Core jsoncpp) -ldl
g++ -std=c++20 $(pkg-config --cflags Fcitx5Core) test-candidate-page.cpp -o build/test-candidate-page $(pkg-config --libs Fcitx5Core)
build/test-candidate-page
