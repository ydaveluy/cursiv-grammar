# Generate the parser and its language module.
cursiv_generate_language(ID protobuf
    GRAMMAR    protobuf/core/ProtobufGrammar.hpp
    NAMESPACE  protobuf::Protobuf
    EXTENSIONS .proto
    OUT        protobuf/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/core/*.cpp)
add_library(protobufCore ${_core})
target_link_libraries(protobufCore PUBLIC protobufGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/cli/*.cpp)
if(_cli)
  add_executable(protobuf_cli ${_cli})
  target_link_libraries(protobuf_cli
                        PRIVATE protobufCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/lsp/main.cpp)
  add_library(protobufLsp ${_lsp})
  target_link_libraries(protobufLsp PUBLIC protobufCore cursiv::lsp)
  add_executable(protobuf_lsp protobuf/lsp/main.cpp)
  target_link_libraries(protobuf_lsp PRIVATE protobufLsp)
endif()

if(TARGET protobufLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/bench/main.cpp")
  cursiv_add_benchmark(protobuf
      SOURCES protobuf/bench/main.cpp)
endif()

cursiv_package_language(protobuf)
