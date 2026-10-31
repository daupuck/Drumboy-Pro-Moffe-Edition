# CMake toolchain file for the Arm GNU Toolchain (arm-none-eabi).
#
# The compiler is looked up in, in order:
#   1. -DARM_TOOLCHAIN_DIR=<dir>   (the folder that contains bin/arm-none-eabi-gcc)
#   2. the ARM_TOOLCHAIN_DIR environment variable
#   3. PATH
#
# Pinned version used by CI and recommended locally: Arm GNU Toolchain 13.3.rel1.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Bare-metal: test-compile static libraries instead of linking executables.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NOT ARM_TOOLCHAIN_DIR AND DEFINED ENV{ARM_TOOLCHAIN_DIR})
    set(ARM_TOOLCHAIN_DIR "$ENV{ARM_TOOLCHAIN_DIR}")
endif()
# Forward the setting to try_compile() sub-projects.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ARM_TOOLCHAIN_DIR)

set(_hints "")
if(ARM_TOOLCHAIN_DIR)
    set(_hints HINTS "${ARM_TOOLCHAIN_DIR}/bin" NO_DEFAULT_PATH)
endif()

find_program(CMAKE_C_COMPILER   arm-none-eabi-gcc     ${_hints} REQUIRED)
find_program(CMAKE_CXX_COMPILER arm-none-eabi-g++     ${_hints} REQUIRED)
find_program(CMAKE_ASM_COMPILER arm-none-eabi-gcc     ${_hints} REQUIRED)
find_program(CMAKE_OBJCOPY      arm-none-eabi-objcopy ${_hints} REQUIRED)
find_program(CMAKE_SIZE         arm-none-eabi-size    ${_hints} REQUIRED)

set(CMAKE_EXECUTABLE_SUFFIX_C   .elf)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .elf)
set(CMAKE_EXECUTABLE_SUFFIX_ASM .elf)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
