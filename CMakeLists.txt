cmake_minimum_required(VERSION 3.25)

project(ShortMarch)

set(CMAKE_CXX_STANDARD 17)

set(LONGMARCH_DISABLE_PYTHON ON)

set(SHADERS_DIR_ABSOLUTE "${PROJECT_SOURCE_DIR}/src/shaders")

add_subdirectory(external/LongMarch)
add_subdirectory(src)

target_compile_definitions(ShortMarchDemo PRIVATE
    # 定义一个名为 SHADERS_DIR 的宏，值为绝对路径字符串
    SHADERS_DIR="${SHADERS_DIR_ABSOLUTE}"
)