#!/bin/bash
gcc mkfw_threaded_example.c -I../.. -lX11 -lXi -lGL -lm -lpthread -o threaded_example -O2
