cmake_minimum_required (VERSION 3.0)

project(rcas)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -Wno-missing-braces")

if(${EMSCRIPTEN})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -s USE_WEBGL2=1 -s USE_GLFW=3 -s FULL_ES3=1 -s WASM=1 -std=c++11")
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/index.html DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
else()
    find_package(glfw3 3.3 REQUIRED)
    find_package(OpenGL REQUIRED)
endif()

include_directories(src)
include_directories(SYSTEM external ${GLFW3_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS})

add_executable(index
    external/imgui.h
    external/imgui.cpp
    external/imgui_demo.cpp
    external/imgui_draw.cpp
    src/imgui_impl_glfw.h
    src/imgui_impl_glfw.cpp
    src/imgui_impl_opengl3.h
    src/imgui_impl_opengl3.cpp
    src/main.cpp
)

source_group(external REGULAR_EXPRESSION external)
source_group(src REGULAR_EXPRESSION src)

set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17")

if(${EMSCRIPTEN})
    target_link_libraries(index websocket.js ${GLFW3_LIBRARIES} ${OPENGL_LIBRARIES})   
else()
    target_link_libraries(index ${GLFW3_LIBRARIES} ${OPENGL_LIBRARIES})   
endif()
