# CMake toolchain file for OpenWRT
#
# How to use:
#
# 1. Clone OpenWRT into your ~/.local directory
# git clone -b chaos_calmer git://github.com/openwrt/openwrt.git
#
# 2. Configure OpenWRT (only need to set target sytem, e.g. Atheros AR7xxx/AR9xxx for TP-Link Archer C7)
# make prereq
#
# 3. Build the cross compiler (does not matter if the build fails after the cross compiler was built)
# make
#
# 4. Build Huasi for OpenWRT
# make openwrt


# target system
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

# get openwrt directory
set(OPENWRT $ENV{HOME}/.local/openwrt)

# set cross compiler
set(CMAKE_C_COMPILER ${OPENWRT}/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-gcc)
set(CMAKE_CXX_COMPILER ${OPENWRT}/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-g++)

# set target environment 
set(CMAKE_FIND_ROOT_PATH ${OPENWRT}/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2)

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
