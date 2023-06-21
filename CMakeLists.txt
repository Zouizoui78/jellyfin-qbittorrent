cmake_minimum_required(VERSION 3.0)

set(PROJECT jellyfin-qbittorrent)
set(VERSION 0.1)
set(PRJ_VER ${PROJECT}-${VERSION})

# --- Configuration ---

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options($<$<CONFIG:Debug>:-DDEBUG>$<$<CONFIG:Release>:-DRELEASE>)

if (UNIX)
    add_compile_options(-DLINUX)
endif (UNIX)

# --- Project ---

project(
    ${PROJECT}
    VERSION ${VERSION}
    LANGUAGES CXX
)

set(
    SRC
    src/main.cpp
    src/Monitor.cpp
)

add_executable(${PROJECT} ${SRC})

target_include_directories(
    ${PROJECT}
    PRIVATE src
)

find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

target_link_libraries(
    ${PROJECT}
    PRIVATE httplib::httplib
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE spdlog::spdlog
)
