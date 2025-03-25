cmake_minimum_required(VERSION 3.11)

set(TARGET_NAME "transport_catalogue")
project(TARGET_NAME CXX)
set(CMAKE_CXX_STANDARD 20)

set(INCLUDE_DIR "./include")
set(
    INCLUDES
    "${INCLUDE_DIR}/json/json.hpp"
    "${INCLUDE_DIR}/json/json_builder.hpp"
    "${INCLUDE_DIR}/json/json_reader.hpp"
    "${INCLUDE_DIR}/map/map_renderer.hpp"
    "${INCLUDE_DIR}/map/svg.hpp"
    "${INCLUDE_DIR}/router/graph.hpp"
    "${INCLUDE_DIR}/router/router.hpp"
    "${INCLUDE_DIR}/router/transport_router.hpp"
    "${INCLUDE_DIR}/transport_catalogue/domain.hpp"
    "${INCLUDE_DIR}/transport_catalogue/request_handler.hpp"
    "${INCLUDE_DIR}/transport_catalogue/transport_catalogue.hpp"
    "${INCLUDE_DIR}/util/geo.hpp"
    "${INCLUDE_DIR}/util/ranges.hpp"
)

set(SRCS_DIR "./src")
set(
    SRCS
    "${SRCS_DIR}/json/json.cpp"
    "${SRCS_DIR}/json/json_builder.cpp"
    "${SRCS_DIR}/json/json_reader.cpp"
    "${SRCS_DIR}/map/map_renderer.cpp"
    "${SRCS_DIR}/map/svg.cpp"
    "${SRCS_DIR}/router/transport_router.cpp"
    "${SRCS_DIR}/transport_catalogue/domain.cpp"
    "${SRCS_DIR}/transport_catalogue/main.cpp"
    "${SRCS_DIR}/transport_catalogue/request_handler.cpp"
    "${SRCS_DIR}/transport_catalogue/transport_catalogue.cpp"
    "${SRCS_DIR}/util/geo.cpp"
)

add_executable(${TARGET_NAME} ${SRCS} ${INCLUDES})

if(CMAKE_SYSTEM_NAME MATCHES "^MINGW")
    set(SYSTEM_LIBS -lstdc++)
else()
    set(SYSTEM_LIBS)
endif()

target_link_libraries(${TARGET_NAME} ${SYSTEM_LIBS}) 
target_include_directories(
    ${TARGET_NAME}
    PUBLIC
    "${INCLUDE_DIR}/json"
    "${INCLUDE_DIR}/map"
    "${INCLUDE_DIR}/router"
    "${INCLUDE_DIR}/transport_catalogue"
    "${INCLUDE_DIR}/util"
)