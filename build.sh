#!/bin/sh

gcc -Wall -O2 -g -c `pkg-config --cflags dbus-1` istvup.c
gcc -Wall -O2 -g -o istvup istvup.o `pkg-config --libs dbus-1`
