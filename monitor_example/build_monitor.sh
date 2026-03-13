#!/bin/bash
gcc mkfw_monitor_example.c -I.. -lX11 -lXi -lGL -lXrandr -lm -o monitor_example -O2
