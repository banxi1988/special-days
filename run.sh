#!/bin/bash -ex
clang -std=gnu11 -lncurses -Ideps/codetalks src/main.c -o special_days --debug && ./special_days