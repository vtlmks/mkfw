#!/bin/bash
# Build all mkfw tests.
#
# Output binaries land alongside their .c sources in this directory.
# Run them individually after building, or call run_tests.sh (if added).

set -e
cd "$(dirname "$0")"

CFLAGS="-std=gnu99 -O2 -Wall -Wextra"
LDFLAGS="-lm -lpthread -ldl"

for src in smoke.c multi_window.c joystick_hotplug.c gamedb.c standalone.c; do
	name="${src%.c}"
	echo "Building $name..."
	gcc $CFLAGS "$src" $LDFLAGS -o "$name"
done

echo "Building audio_alsa..."
gcc $CFLAGS audio_alsa.c $LDFLAGS -lasound -o audio_alsa

echo "Building standalone_library..."
gcc $CFLAGS -DMKFW_BUILD_LIBRARY standalone.c ../mkfw_error.c ../mkfw_linux_audio.c ../mkfw_linux_timer.c $LDFLAGS -o standalone_library

echo "Done."
