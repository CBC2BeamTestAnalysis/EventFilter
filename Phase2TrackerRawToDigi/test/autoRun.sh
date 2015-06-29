#!/bin/bash

python autoUnpacker.py
cd temp
source runall.sh
cd ..
rm -rd temp


