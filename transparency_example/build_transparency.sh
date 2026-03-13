#!/bin/bash
gcc mkfw_transparency_example.c -I.. -lX11 -lXi -lGL -lXrandr -lm -o transparency_example -O2
