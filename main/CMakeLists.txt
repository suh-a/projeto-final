# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Inicialização do pico_sdk a partir da instalação local
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

project(main C CXX ASM)

pico_sdk_init()

# Adiciona o executável com o nome "main"
add_executable(main
    main.c
    inc/ssd1306.c
    
)

pico_set_program_name(main "main")
pico_set_program_version(main "0.1")

# Gerar o header PIO a partir do arquivo main.pio (do controle da matriz)
pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/main.pio)

target_link_libraries(main 
    pico_stdlib
    hardware_adc
    hardware_pwm
    hardware_pio
    hardware_i2c
    hardware_clocks
)

target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

pico_enable_stdio_uart(main 1)
pico_enable_stdio_usb(main 1)

pico_add_extra_outputs(main)
