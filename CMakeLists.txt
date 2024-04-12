cmake_minimum_required(VERSION 3.13)

set(NAME pico_ttp)
set(URL "https://github.com/dcooperdalrymple/pico-ttp")

add_library(${NAME} INTERFACE)

pico_generate_pio_header(${NAME} ${CMAKE_CURRENT_LIST_DIR}/src/ttp_serial.pio)

target_sources(${NAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/ttp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ttp229.cpp
)

target_include_directories(${NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

# Pull in pico libraries that we need
target_link_libraries(${NAME} INTERFACE pico_stdlib hardware_gpio hardware_pio hardware_clocks hardware_irq)
