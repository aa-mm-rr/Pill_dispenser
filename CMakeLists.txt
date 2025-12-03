cmake_minimum_required(VERSION 3.13)

project(pill_dispenser C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(pico_sdk_import.cmake)
pico_sdk_init()

add_executable(pill_dispenser
        src/main.c
        src/state.c
        src/stepper.c
        src/sensors.c
        src/leds.c
        src/buttons.c
        src/eeprom_at24c256.c
        src/lora.c
        src/util.c
)

set_target_properties(pill_dispenser PROPERTIES SUFFIX ".elf")

target_include_directories(pill_dispenser PRIVATE include)
target_link_libraries(pill_dispenser pico_stdlib hardware_gpio hardware_uart hardware_i2c hardware_timer)

pico_enable_stdio_usb(pill_dispenser 0)
pico_enable_stdio_uart(pill_dispenser 1) # UART0 reserved; using UART1 for LoRa

pico_add_extra_outputs(pill_dispenser)
