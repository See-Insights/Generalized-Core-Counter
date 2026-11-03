#!/bin/zsh
# Local toolchain build of the spike (AI_DEVELOPMENT_WORKFLOW.md §3 Stage 7). Usage: spike/build.sh <fresh scratch dir>
set -e
S=$1; [ -n "$S" ] && [ ! -e "$S" ] || { echo "give a scratch dir that does not exist yet"; exit 1; }
R=${0:A:h:h}
mkdir -p $S/tree/src $S/tree/lib
cp $R/spike/src/spike.cpp $S/tree/src/
cp $R/spike/project.properties $S/tree/
cp -R $R/lib/AB1805_RK $S/tree/lib/
export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH
cd ~/.particle/toolchains/deviceOS/6.6.0/main
make -s PLATFORM=boron APPDIR=$S/tree TARGET_DIR=$S/tree/localbuild \
     DEVICE_OS_PATH=~/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE=$S/bp
ls -l $S/tree/localbuild
