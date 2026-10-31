# Command-line build for the Drumboy Pro firmware (bootloader + application).
#
# This mirrors the STM32CubeIDE project settings (.cproject) so the same sources can be
# built locally and in CI without the IDE. STM32CubeIDE remains fully supported; this
# file does not touch the IDE projects.
#
#   cmake --preset firmware        (configure; see CMakePresets.json)
#   cmake --build --preset firmware
#
# Outputs (in build/):
#   Drumboy-Pro-Boot-H723.elf/.bin/.map
#   Drumboy-Pro-App-H723.elf/.bin/.map
#   firmware/Drumboy_Pro_Fw_<MM>_<mm>.bin   SD-card update package for the application

cmake_minimum_required(VERSION 3.22)

project(DrumboyPro LANGUAGES C CXX ASM)

# Optimization levels match the CubeIDE "Debug" configurations, which are the ones the
# shipped firmware and the .launch files use. The application's DSP engine relies on
# -Ofast; do not lower it without checking audio performance on hardware.
set(DRUMBOY_APP_OPT  "-Ofast" CACHE STRING "Optimization flags for the application")
set(DRUMBOY_BOOT_OPT "-Os"    CACHE STRING "Optimization flags for the bootloader")

find_package(Python3 COMPONENTS Interpreter)

set(MCU_FLAGS -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb)

set(SW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/02 Software")

function(drumboy_firmware target project_dir opt_flags)
    set(dir "${SW_DIR}/${project_dir}")

    file(GLOB_RECURSE sources CONFIGURE_DEPENDS
        "${dir}/Core/*.c"   "${dir}/Core/*.cpp"   "${dir}/Core/*.s"
        "${dir}/Drivers/*.c"
        "${dir}/FATFS/*.c"
        "${dir}/Middlewares/*.c")

    add_executable(${target} ${sources})
    set_target_properties(${target} PROPERTIES OUTPUT_NAME "${project_dir}")

    target_include_directories(${target} PRIVATE
        "${dir}/Core/Inc"
        "${dir}/FATFS/Target"
        "${dir}/FATFS/App"
        "${dir}/Drivers/STM32H7xx_HAL_Driver/Inc"
        "${dir}/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy"
        "${dir}/Middlewares/Third_Party/FatFs/src"
        "${dir}/Drivers/CMSIS/Device/ST/STM32H7xx/Include"
        "${dir}/Drivers/CMSIS/Include"
        "${dir}/Core/Library/Global"
        "${dir}/Core/Library/Lcd"
        "${dir}/Core/Library/Dac"
        "${dir}/Core/Library/Controller")

    target_compile_definitions(${target} PRIVATE USE_PWR_LDO_SUPPLY USE_HAL_DRIVER STM32H723xx)

    target_compile_options(${target} PRIVATE
        ${MCU_FLAGS}
        $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp>
        $<$<NOT:$<COMPILE_LANGUAGE:ASM>>:${opt_flags} -g3 -ffast-math -ffunction-sections -fdata-sections -Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-use-cxa-atexit>)

    target_link_options(${target} PRIVATE
        ${MCU_FLAGS}
        "-T${dir}/STM32H723ZGTX_FLASH.ld"
        --specs=nosys.specs --specs=nano.specs
        -static
        -Wl,--gc-sections
        "-Wl,-Map=$<TARGET_FILE_DIR:${target}>/${project_dir}.map"
        -Wl,--print-memory-usage)

    target_link_libraries(${target} PRIVATE -Wl,--start-group c m stdc++ supc++ -Wl,--end-group)

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target}> $<TARGET_FILE_DIR:${target}>/${project_dir}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target}>
        COMMENT "Creating ${project_dir}.bin")
endfunction()

drumboy_firmware(boot "Drumboy-Pro-Boot-H723" "${DRUMBOY_BOOT_OPT}")
drumboy_firmware(app  "Drumboy-Pro-App-H723"  "${DRUMBOY_APP_OPT}")

# SD-card update package. The version is read from the application's Global.h
# (kVersionMajor / kVersionMinor), so bumping it there is the single source of truth.
if(Python3_Interpreter_FOUND)
    set(pkg_tool "${SW_DIR}/Firmware/make_fw_package.py")
    add_custom_target(package ALL
        COMMAND ${Python3_EXECUTABLE} "${pkg_tool}"
                "$<TARGET_FILE_DIR:app>/Drumboy-Pro-App-H723.bin"
                --version-header "${SW_DIR}/Drumboy-Pro-App-H723/Core/Library/Global/Global.h"
                --out-dir "${CMAKE_BINARY_DIR}/firmware"
        DEPENDS app "${pkg_tool}"
        COMMENT "Building SD-card firmware package")
else()
    message(WARNING "Python 3 not found: the SD-card firmware package will not be built.")
endif()
