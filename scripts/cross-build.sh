#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${WINDHAWK_SOURCE:?Set WINDHAWK_SOURCE to the official ramensoftware/windhawk checkout}"
: "${MINGW_CXX:=x86_64-w64-mingw32-g++}"
: "${MINGW_DLLTOOL:=x86_64-w64-mingw32-dlltool}"
sdk_source="$WINDHAWK_SOURCE/src/windhawk/engine"
mkdir -p build/sdk
cp "$sdk_source/mods_api.h" build/sdk/windhawk_api.h
cp "$sdk_source/mods_api_internal.h" build/sdk/mods_api_internal.h
"$MINGW_DLLTOOL" -d "$sdk_source/_exports.def" -l build/libwindhawk.a
"$MINGW_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
    -Wno-cast-function-type -DUNICODE -D_UNICODE -DWH_MOD -Ibuild/sdk \
    -c native-smooth-taskbar.wh.cpp -o build/native-smooth-taskbar.o
"$MINGW_CXX" -shared -static \
    -Wl,--export-all-symbols build/native-smooth-taskbar.o build/libwindhawk.a \
    -ldwmapi -o build/native-smooth-taskbar.validation.dll
"$MINGW_CXX" -std=c++20 -O2 -Wall -Wextra -Werror \
    -Wno-cast-function-type -DUNICODE -D_UNICODE -DWH_MOD -Ibuild/sdk \
    -c native-app-motion.wh.cpp -o build/native-app-motion.o
"$MINGW_CXX" -shared -static -Wl,--export-all-symbols \
    build/native-app-motion.o build/libwindhawk.a \
    -ldwmapi -lgdi32 -lole32 -ladvapi32 -luuid \
    -o build/native-app-motion.validation.dll
printf '%s\n' 'Windows x64 cross-compile and link passed (not a Windows runtime test).'
