#!/bin/bash

nc 127.0.0.1 5555 | tee vmout
NC_PID=$!

make qemu

kill $NC_PID 2>/dev/null