# pico_sdk_import.cmake
# Fetch or point to your local Pico SDK
include(FetchContent)
if (NOT PICO_SDK_PATH)
    set(PICO_SDK_FETCH_FROM_GIT on)
    FetchContent_Declare(
            pico_sdk
            GIT_REPOSITORY https://github.com/raspberrypi/pico-sdk
            GIT_TAG        master
    )
    FetchContent_MakeAvailable(pico_sdk)
else()
    message(STATUS "Using local PICO_SDK_PATH=${PICO_SDK_PATH}")
    include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
endif()
