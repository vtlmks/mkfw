#!/bin/bash
gcc mkfw_joystick_example.c -I../.. -lX11 -lXi -lGL -lm -o joystick_example -O2
