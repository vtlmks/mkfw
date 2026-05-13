#!/bin/bash
# Build all mkfw tests.
#
# Output binaries land alongside their .c sources in this directory.
# Run them individually after building, or call run_tests.sh (if added).

set -e
cd "$(dirname "$0")"

CFLAGS="-std=gnu99 -O2 -Wall -Wextra"
LDFLAGS="-lm -lpthread -ldl"

for src in smoke.c multi_window.c; do
	name="${src%.c}"
	echo "Building $name..."
	gcc $CFLAGS "$src" $LDFLAGS -o "$name"
done

echo "Done."
