#!/bin/bash -ex
clang -std=gnu11 -Ideps/codetalks src/main.c -o special_days --debug && ./special_days