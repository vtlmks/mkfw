#!/bin/bash
# Build all mkfw examples.
#
# Output binaries land alongside their .c sources in this directory.

set -e
cd "$(dirname "$0")"

CFLAGS="-std=gnu99 -O2 -Wall -Wextra"
LDFLAGS="-lm -lpthread -ldl"

for src in joystick.c threaded.c monitor.c transparency.c audio_beep.c; do
	name="${src%.c}"
	echo "Building $name..."
	gcc $CFLAGS "$src" $LDFLAGS -o "$name"
done

echo "Done."
