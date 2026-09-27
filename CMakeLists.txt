cmake_minimum_required(VERSION 3.22)
project(ArmServoController C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(STM32CUBE_H7_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/STM32CubeH7" CACHE PATH
    "Path to the official STM32CubeH7 repository")
set(FREERTOS_KERNEL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/FreeRTOS-Kernel" CACHE PATH
    "Path to the official FreeRTOS-Kernel repository")

set(HAL_DIR "${STM32CUBE_H7_ROOT}/Drivers/STM32H7xx_HAL_Driver")
set(CMSIS_DEVICE_DIR "${STM32CUBE_H7_ROOT}/Drivers/CMSIS/Device/ST/STM32H7xx")
set(CMSIS_CORE_DIR "${STM32CUBE_H7_ROOT}/Drivers/CMSIS")
set(FREERTOS_PORT_DIR "${FREERTOS_KERNEL_ROOT}/portable/GCC/ARM_CM7/r0p1")

foreach(required_path IN ITEMS
        "${HAL_DIR}/Inc/stm32h7xx_hal.h"
        "${CMSIS_DEVICE_DIR}/Include/stm32h723xx.h"
        "${FREERTOS_KERNEL_ROOT}/tasks.c"
        "${FREERTOS_PORT_DIR}/port.c")
    if(NOT EXISTS "${required_path}")
        message(FATAL_ERROR "Required vendor file not found: ${required_path}\n"
            "See README.md for dependency setup.")
    endif()
endforeach()

set(HAL_SOURCES
    ${HAL_DIR}/Src/stm32h7xx_hal.c
    ${HAL_DIR}/Src/stm32h7xx_hal_cortex.c
    ${HAL_DIR}/Src/stm32h7xx_hal_dma.c
    ${HAL_DIR}/Src/stm32h7xx_hal_dma_ex.c
    ${HAL_DIR}/Src/stm32h7xx_hal_flash.c
    ${HAL_DIR}/Src/stm32h7xx_hal_flash_ex.c
    ${HAL_DIR}/Src/stm32h7xx_hal_gpio.c
    ${HAL_DIR}/Src/stm32h7xx_hal_pwr.c
    ${HAL_DIR}/Src/stm32h7xx_hal_pwr_ex.c
    ${HAL_DIR}/Src/stm32h7xx_hal_rcc.c
    ${HAL_DIR}/Src/stm32h7xx_hal_rcc_ex.c
    ${HAL_DIR}/Src/stm32h7xx_hal_uart.c
    ${HAL_DIR}/Src/stm32h7xx_hal_uart_ex.c)

set(FREERTOS_SOURCES
    ${FREERTOS_KERNEL_ROOT}/tasks.c
    ${FREERTOS_KERNEL_ROOT}/queue.c
    ${FREERTOS_KERNEL_ROOT}/list.c
    ${FREERTOS_KERNEL_ROOT}/timers.c
    ${FREERTOS_KERNEL_ROOT}/event_groups.c
    ${FREERTOS_KERNEL_ROOT}/stream_buffer.c
    ${FREERTOS_PORT_DIR}/port.c
    ${FREERTOS_KERNEL_ROOT}/portable/MemMang/heap_4.c)

add_executable(${PROJECT_NAME}
    Core/Src/main.c
    app/source/app_freertos.c
    Core/Src/stm32h7xx_it.c
    app/source/app_servo_test.c
    bsp/source/bsp_board.c
    drivers/ws2812/ws2812.c
    drivers/ws2812/ws2812_palette.c
    drivers/servo/scs_protocol.c
    drivers/servo/sm30bl.c
    ${CMSIS_DEVICE_DIR}/Source/Templates/system_stm32h7xx.c
    ${CMSIS_DEVICE_DIR}/Source/Templates/gcc/startup_stm32h723xx.s
    ${HAL_SOURCES}
    ${FREERTOS_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

target_include_directories(${PROJECT_NAME} PRIVATE
    Core/Inc
    app/include
    bsp/include
    drivers/servo
    drivers/ws2812
    config
    ${HAL_DIR}/Inc
    ${HAL_DIR}/Inc/Legacy
    ${CMSIS_DEVICE_DIR}/Include
    ${CMSIS_CORE_DIR}/Include
    ${FREERTOS_KERNEL_ROOT}/include
    ${FREERTOS_PORT_DIR})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    STM32H723xx
    USE_HAL_DRIVER)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-mcpu=cortex-m7;-mthumb;-mfpu=fpv5-d16;-mfloat-abi=hard;-ffunction-sections;-fdata-sections;-Wall;-Wextra;-Wshadow;-Wdouble-promotion;-Wformat=2>
    $<$<COMPILE_LANGUAGE:ASM>:-mcpu=cortex-m7;-mthumb;-mfpu=fpv5-d16;-mfloat-abi=hard;-x;assembler-with-cpp>)

target_link_options(${PROJECT_NAME} PRIVATE
    -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32H723VGTx_FLASH.ld
    -Wl,--gc-sections -Wl,-Map=${PROJECT_NAME}.map
    --specs=nano.specs --specs=nosys.specs)

target_link_libraries(${PROJECT_NAME} PRIVATE m)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>)
