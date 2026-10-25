# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `protobufGenerated` (the compiled library) and `protobuf_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID protobuf
    GRAMMAR    protobuf/core/ProtobufGrammar.hpp
    NAMESPACE  protobuf::Protobuf
    EXTENSIONS .proto
    OUT        protobuf/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/core/*.cpp)
add_library(protobufCore ${_core})
target_link_libraries(protobufCore PUBLIC protobufGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/cli/*.cpp)
add_executable(protobuf_cli ${_cli})
target_link_libraries(protobuf_cli
                      PRIVATE protobufCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/lsp/main.cpp)
add_library(protobufLsp ${_lsp})
target_link_libraries(protobufLsp PUBLIC protobufCore cursiv::lsp)
add_executable(protobuf_lsp protobuf/lsp/main.cpp)
target_link_libraries(protobuf_lsp PRIVATE protobufLsp)

cursiv_add_benchmark(protobuf SOURCES protobuf/bench/main.cpp)

cursiv_package_language(protobuf)
