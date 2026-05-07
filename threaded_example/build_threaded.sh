#!/bin/bash
gcc mkfw_threaded_example.c -I../.. -lm -lpthread -ldl -o threaded_example -O2
