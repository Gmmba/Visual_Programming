cmake_minimum_required(VERSION 3.14)
project(LocationServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_package(cppzmq REQUIRED)
find_package(PostgreSQL REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)
pkg_check_modules(PQ REQUIRED libpq)

set(THIRD_PARTY ${PROJECT_SOURCE_DIR}/third_party)
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${THIRD_PARTY}/imgui
    ${THIRD_PARTY}/implot
    ${PQXX_INCLUDE_DIRS}
    ${PQ_INCLUDE_DIRS}
)

add_library(imgui STATIC
    ${THIRD_PARTY}/imgui/imgui.cpp
    ${THIRD_PARTY}/imgui/imgui_draw.cpp
    ${THIRD_PARTY}/imgui/imgui_demo.cpp
    ${THIRD_PARTY}/imgui/imgui_tables.cpp
    ${THIRD_PARTY}/imgui/imgui_widgets.cpp
    ${THIRD_PARTY}/imgui/backends/imgui_impl_sdl2.cpp
    ${THIRD_PARTY}/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${THIRD_PARTY}/imgui)
target_compile_options(imgui PRIVATE -fPIC)

add_library(implot STATIC
    ${THIRD_PARTY}/implot/implot.cpp
    ${THIRD_PARTY}/implot/implot_demo.cpp
    ${THIRD_PARTY}/implot/implot_items.cpp
)
target_include_directories(implot PUBLIC ${THIRD_PARTY}/implot)
target_link_libraries(implot PRIVATE imgui)
target_compile_options(implot PRIVATE -fPIC)

add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE
    imgui
    implot
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${ZMQ_LIBRARIES}
    cppzmq
    PostgreSQL::PostgreSQL
    ${PQXX_LIBRARIES}
    ${PQ_LIBRARIES}
)

link_directories(${PQXX_LIBRARY_DIRS})
link_directories(${PQ_LIBRARY_DIRS})