# CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(VerificationBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# 查找TgBot库
find_path(TGBOT_INCLUDE_DIR tgbot/tgbot.h
    PATHS /usr/local/include /usr/include
)

find_library(TGBOT_LIBRARY
    NAMES TgBot tgbot
    PATHS /usr/local/lib /usr/lib /usr/lib/x86_64-linux-gnu
)

if(NOT TGBOT_INCLUDE_DIR OR NOT TGBOT_LIBRARY)
    message(FATAL_ERROR "TgBot library not found")
endif()

add_executable(${PROJECT_NAME} src/main.cpp)

# 添加包含目录
target_include_directories(${PROJECT_NAME} PRIVATE ${TGBOT_INCLUDE_DIR})

# 链接所需的库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${TGBOT_LIBRARY}
    Boost::system 
    OpenSSL::SSL 
    OpenSSL::Crypto
    CURL::libcurl
    pthread
)