#!/bin/bash
gcc mkfw_ui_example.c -I../.. -lX11 -lXi -lGL -lm -o ui_example -O2

