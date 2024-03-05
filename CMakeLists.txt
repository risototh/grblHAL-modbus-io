add_library(mbio INTERFACE)

target_sources(mbio INTERFACE
 ${CMAKE_CURRENT_LIST_DIR}/modbus_io.c
)

target_include_directories(mbio INTERFACE ${CMAKE_CURRENT_LIST_DIR})
