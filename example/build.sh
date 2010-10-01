#!/usr/bin/env sh
gcc -I../src/ -Wall -O3 -march=native -Wno-multichar main.c -lpthread
