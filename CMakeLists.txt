cmake_minimum_required(VERSION 3.7)
project(Trawler)

set(CMAKE_CXX_STANDARD 14)
find_package(Boost COMPONENTS system filesystem REQUIRED)

set(SOURCE_FILES main.cpp)
add_executable(Trawler ${SOURCE_FILES})

find_library(LCURLPP curlpp lib)
find_library(LZIP zip lib)
target_link_libraries(${PROJECT_NAME} ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${LCURLPP} ${LZIP} curl)