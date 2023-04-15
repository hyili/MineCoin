cmake_minimum_required(VERSION 3.23)
project(boost)

if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNDEBUG)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 20)
find_library(PTHREAD_LIB NAMES pthread)
find_library(SSL_LIB NAMES ssl)
find_library(CRYPTO_LIB NAMES crypto)

set(Boost_USE_STATIC_LIBS ON)

file(GLOB_RECURSE PROJECT_SOURCE_SHARE src/*.cpp)
add_library(${PROJECT_NAME}-share OBJECT ${PROJECT_SOURCE_SHARE})
target_include_directories(${PROJECT_NAME}-share BEFORE PRIVATE /home/hyili/BOOM/boost include common)

file(GLOB_RECURSE PROJECT_SOURCE_WEBSOCKET_CLIENT test/websocket_client.cpp)
add_executable(${PROJECT_NAME}-websocket-client ${PROJECT_SOURCE_WEBSOCKET_CLIENT})
target_include_directories(${PROJECT_NAME}-websocket-client BEFORE PRIVATE /home/hyili/BOOM/boost include common)
target_link_libraries(${PROJECT_NAME}-websocket-client ${PTHREAD_LIB} ${SSL_LIB} ${CRYPTO_LIB} $<TARGET_OBJECTS:${PROJECT_NAME}-share>)

file(GLOB_RECURSE PROJECT_SOURCE_WEBSOCKET_CLIENT_ASYNC_SSL test/websocket_client_async_ssl.cpp)
add_executable(${PROJECT_NAME}-websocket-client-async-ssl ${PROJECT_SOURCE_WEBSOCKET_CLIENT_ASYNC_SSL})
target_include_directories(${PROJECT_NAME}-websocket-client-async-ssl BEFORE PRIVATE /home/hyili/BOOM/boost include common)
target_link_libraries(${PROJECT_NAME}-websocket-client-async-ssl ${PTHREAD_LIB} ${SSL_LIB} ${CRYPTO_LIB} $<TARGET_OBJECTS:${PROJECT_NAME}-share>)

file(GLOB_RECURSE PROJECT_SOURCE_HTTP_CLIENT test/http_client.cpp)
add_executable(${PROJECT_NAME}-http-client ${PROJECT_SOURCE_HTTP_CLIENT})
target_include_directories(${PROJECT_NAME}-http-client BEFORE PRIVATE /home/hyili/BOOM/boost include common)
target_link_libraries(${PROJECT_NAME}-http-client ${PTHREAD_LIB} ${SSL_LIB} ${CRYPTO_LIB} $<TARGET_OBJECTS:boost-share>)
