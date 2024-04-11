#!/bin/bash
clear

make -f makefiles/bckoff_makefile
make -f makefiles/bckoff_makefile clean

make -f makefiles/sm_makefile
make -f makefiles/sm_makefile clean

make -f makefiles/mobile_makefile
make -f makefiles/mobile_makefile clean
