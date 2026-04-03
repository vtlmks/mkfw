#!/bin/bash
emcc mkfw_run_example.c -I.. -sUSE_WEBGL2=1 -sFULL_ES3=1 -sAUDIO_WORKLET=1 -sWASM_WORKERS=1 -o mkfw_run_example.html -O2
