# CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(VerificationBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 确保这里的路径指向您在 Windows 上的 vcpkg 安装位置
# set(CMAKE_TOOLCHAIN_FILE "D:/Code/CPP_Study/ArisuMaid_bot/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

find_package(Boost REQUIRED COMPONENTS system)
find_package(TgBot REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp)

# 链接 TgBot 库，它会自动链接 OpenSSL, Curl, Boost 等
target_link_libraries(${PROJECT_NAME} PRIVATE TgBot::TgBot)