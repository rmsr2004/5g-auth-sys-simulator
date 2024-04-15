#!/bin/bash
clear

make -f makefiles/bckoff_makefile all
make -f makefiles/bckoff_makefile clean

make -f makefiles/sm_makefile all
make -f makefiles/sm_makefile clean

make -f makefiles/mobile_makefile all
make -f makefiles/mobile_makefile clean
