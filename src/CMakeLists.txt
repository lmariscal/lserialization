find_package(ergo REQUIRED)

file(GLOB_RECURSE TEST_SOURCES
  ${CMAKE_SOURCE_DIR}/src/*.cc
)

add_executable(test ${TEST_SOURCES})

set_target_properties(test PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_definitions(test PUBLIC TEST_DEBUG=1)
endif()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/dist/bin)

set_target_properties(test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/bin)
set_target_properties(test PROPERTIES OUTPUT_NAME test)

target_link_libraries(test PRIVATE ergo)
target_link_libraries(test PRIVATE nlohmann_json::nlohmann_json)

# Set sources of IDEs

file(GLOB_RECURSE TEST_HEADERS
  ${CMAKE_SOURCE_DIR}/src/*.hh
)

list(APPEND TEST_SOURCES ${TEST_HEADERS})

target_sources(test PUBLIC ${TEST_SOURCES})
