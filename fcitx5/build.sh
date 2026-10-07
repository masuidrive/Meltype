#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
root="$(dirname "$here")"
case "$(uname -m)" in
 aarch64) rid=linux-arm64 ;;
 x86_64) rid=linux-x64 ;;
 *) echo "Unsupported architecture" >&2; exit 1 ;;
esac
helper="$root/native/mozc/bin/linux/meltype_mozc_helper"
[[ -x "$helper" ]] || { echo "Build native/mozc/build-mozc-helper.sh first." >&2; exit 1; }
dotnet publish "$root/src/Meltype.Mac.Native/Meltype.Mac.Native.csproj" -c Release -r "$rid" -p:PublishAot=true -p:NativeLib=Shared -p:StripSymbols=true --source https://api.nuget.org/v3/index.json -o "$here/build/native"
bash "$here/build-adapter.sh"
mkdir -p "$here/build/runtime/mozc"
install -m644 "$here/build/native/MeltypeNative.so" "$here/build/runtime/libMeltypeNative.so"
install -m755 "$helper" "$here/build/runtime/mozc/meltype_mozc_helper"
cp "$root/native/mozc/bin/linux/"MOZC-{LICENSE.txt,CREDITS.html} "$here/build/runtime/mozc/"
cp "$root/LICENSE" "$root/THIRD-PARTY-NOTICES.md" "$here/build/runtime/"
