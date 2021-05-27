cmake_minimum_required(VERSION 3.8)
project(reshade_grabber)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c ${RESHADE_SOURCE_DIR}/dll_config.cpp)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ConvertRawImage.cpp)

find_package(OpenMP REQUIRED)
find_package(PNG REQUIRED)
find_package(ReShade REQUIRED)

add_library(reshade_grabber SHARED ${SOURCES})
set_target_properties(reshade_grabber PROPERTIES SUFFIX ".addon")
target_link_libraries(reshade_grabber PRIVATE PNG::PNG)
target_include_directories(reshade_grabber PRIVATE ${RESHADE_SOURCE_DIR} ${RESHADE_INCLUDE_DIR} ${IMGUI_INCLUDE_DIR})

add_executable(convert_raw_image src/ConvertRawImage.cpp src/ImageRaw.hpp src/ImageRaw.cpp src/IO.hpp src/IO.cpp)
target_link_libraries(convert_raw_image PNG::PNG)

if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

install(
        TARGETS reshade_grabber
        RUNTIME DESTINATION addons
)
install(
        TARGETS convert_raw_image
        RUNTIME DESTINATION addons
)
