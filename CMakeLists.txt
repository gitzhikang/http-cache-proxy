cmake_minimum_required(VERSION 3.29)
project(cache_proxy)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 源文件列表
set(SOURCES
        main.cpp
        proxy.cpp
        request.cpp
        response.cpp
        utils.cpp
)

# 头文件列表
set(HEADERS
        info.h
        proxy.h
        request.h
        response.h
        thread_safe_cache.h
        utils.h
)

# 添加可执行文件
add_executable(cache_proxy ${SOURCES} ${HEADERS})

# 包含目录
target_include_directories(cache_proxy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 如果有外部库依赖，可以在这里添加
# find_package(外部库名称 REQUIRED)
# target_link_libraries(cache_proxy PRIVATE 外部库名称)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cache_proxy PRIVATE -Wall -Wextra -pedantic)
elseif(MSVC)
    target_compile_options(cache_proxy PRIVATE /W4)
endif()