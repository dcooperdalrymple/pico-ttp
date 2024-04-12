# pico-ttp
TonTouch TTP229 hardware driver for the Raspberry Pi Pico C++ SDK.

## Installation
Make sure that [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) is installed correctly before following the instructions below.

### Example Compilation
Clone the library using `git clone https://github.com/dcooperdalrymple/pico-ttp` into a suitable location. Run the following commands to build the library example and ensure that your machine is capable of building this software.

````
cd pico-ttp/example
mkdir build && cd build
cmake ..
make
````

### Install Extension into Your Project
In order to add this library as an extension of your project, insert it as a submodule using `git submodule add https://github.com/dcooperdalrymple/pico-ttp.git` into the desired location. In the `CMakeLists.txt` file, insert `add_subdirectory(./{PATH_TO_SUBMODULE}/pico-ttp)` below your source files (ie: `add_executable(...)`). Then add `pico_rotary_encoder` to your list of _target_link_libraries_ such as `target_link_libraries(... pico_rotary_encoder ...)`.

## Usage
See `./example/pico-ttp-example.cpp` for full implementation.
