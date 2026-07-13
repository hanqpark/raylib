cmake_minimum_required(VERSION 3.15)
project(HFT_Raylib_Sandbox CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 최적화 및 경고 플래그
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -Wextra -Wpedantic -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")

# Mac / Rosetta2 Homebrew 경로 매핑
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

# [수정] 소스 파일 경로를 src/ 내부로 명시적 변경
set(SOURCES
    src/main.cpp
    src/Engine.hpp
    src/Window.hpp
    src/Config.hpp
)

add_executable(hft_sandbox ${SOURCES})

# 컴파일러가 src/ 내부의 헤더 파일들을 서로 쉽게 찾을 수 있도록 인클루드 경로 주입
target_include_directories(hft_sandbox PRIVATE src)

if(APPLE)
    target_link_libraries(hft_sandbox 
        raylib 
        "-framework OpenGL" 
        "-framework Cocoa" 
        "-framework IOKit" 
        "-framework CoreVideo"
    )
endif()