# Generate the parser and its language module.
cursiv_generate_language(ID flatbuffers
    GRAMMAR    flatbuffers/core/FlatbuffersGrammar.hpp
    NAMESPACE  flatbuffers::Flatbuffers
    EXTENSIONS .fbs
    OUT        flatbuffers/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers/core/*.cpp)
add_library(flatbuffersCore ${_core})
target_link_libraries(flatbuffersCore PUBLIC flatbuffersGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers/cli/*.cpp)
if(_cli)
  add_executable(flatbuffers_cli ${_cli})
  target_link_libraries(flatbuffers_cli
                        PRIVATE flatbuffersCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers/lsp/main.cpp)
  add_library(flatbuffersLsp ${_lsp})
  target_link_libraries(flatbuffersLsp PUBLIC flatbuffersCore cursiv::lsp)
  add_executable(flatbuffers_lsp flatbuffers/lsp/main.cpp)
  target_link_libraries(flatbuffers_lsp PRIVATE flatbuffersLsp)
endif()

if(TARGET flatbuffersLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers/bench/main.cpp")
  cursiv_add_benchmark(flatbuffers
      SOURCES flatbuffers/bench/main.cpp)
endif()

cursiv_package_language(flatbuffers)
