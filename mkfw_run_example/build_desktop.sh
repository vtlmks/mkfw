#!/bin/bash

# Linux
gcc mkfw_run_example.c -I.. -lX11 -lXi -lXrandr -lGL -lm -lpthread -lasound -o mkfw_run_example -O2

# Windows (cross-compile with mingw)
x86_64-w64-mingw32-gcc mkfw_run_example.c -I.. -lopengl32 -lgdi32 -lwinmm -lole32 -lavrt -o mkfw_run_example.exe -O2
