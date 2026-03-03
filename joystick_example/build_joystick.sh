#!/bin/bash
gcc mkfw_joystick_example.c -I.. -lX11 -lXi -lm -o joystick_example -O2
