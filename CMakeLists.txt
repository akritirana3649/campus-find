cmake_minimum_required(VERSION 3.20)

project(CampusFind LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(Crow)

add_executable(CampusFind server.cpp)

target_include_directories(CampusFind PRIVATE
    "C:/msys64/ucrt64/include/mariadb"
)

target_link_directories(CampusFind PRIVATE
    "C:/msys64/ucrt64/lib"
)

target_link_libraries(CampusFind PRIVATE
    Crow::Crow
    mariadb
    ws2_32
    mswsock
)