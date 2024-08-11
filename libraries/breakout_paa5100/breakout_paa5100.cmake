if(NOT TARGET pmw3901)
  include(${CMAKE_CURRENT_LIST_DIR}/../../drivers/pmw3901/pmw3901.cmake)
endif()

set(LIB_NAME breakout_paa5100)
add_library(${LIB_NAME} INTERFACE)

target_sources(${LIB_NAME} INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/${LIB_NAME}.cpp
)

target_include_directories(${LIB_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR})

# Pull in pico libraries that we need
target_link_libraries(${LIB_NAME} INTERFACE pico_stdlib pmw3901)
