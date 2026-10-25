# Generate the parser and its language module.
cursiv_generate_language(ID fasta
    GRAMMAR    fasta/core/FastaGrammar.hpp
    NAMESPACE  fasta::Fasta
    EXTENSIONS .fasta
    OUT        fasta/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fasta/core/*.cpp)
add_library(fastaCore ${_core})
target_link_libraries(fastaCore PUBLIC fastaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fasta/cli/*.cpp)
if(_cli)
  add_executable(fasta_cli ${_cli})
  target_link_libraries(fasta_cli
                        PRIVATE fastaCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fasta/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/fasta/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/fasta/lsp/main.cpp)
  add_library(fastaLsp ${_lsp})
  target_link_libraries(fastaLsp PUBLIC fastaCore cursiv::lsp)
  add_executable(fasta_lsp fasta/lsp/main.cpp)
  target_link_libraries(fasta_lsp PRIVATE fastaLsp)
endif()

if(TARGET fastaLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fasta/bench/main.cpp")
  cursiv_add_benchmark(fasta
      SOURCES fasta/bench/main.cpp)
endif()

cursiv_package_language(fasta)
