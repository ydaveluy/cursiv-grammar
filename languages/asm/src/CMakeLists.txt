# Generate the parser and its language module.
cursiv_generate_language(ID nasm
    GRAMMAR    nasm/core/NasmGrammar.hpp
    NAMESPACE  nasm::Nasm
    EXTENSIONS .asm
    OUT        nasm/generated)

# The mnemonic and register vocabularies alone are ~2450 terminals, past the
# default 8-word (512-bit) ceiling of the generator's per-state terminal
# sets; cursivLrGen (linked into nasm_gen) builds the tables, so it needs
# the same raised ceiling.
if(CMAKE_CROSSCOMPILING)
  cursiv_host_manifest("target_compile_definitions(nasm_gen PRIVATE CURSIV_SET_WORDS=64u)")
  cursiv_host_manifest("target_compile_definitions(cursivLrGen PRIVATE CURSIV_SET_WORDS=64u)")
else()
  target_compile_definitions(nasm_gen PRIVATE CURSIV_SET_WORDS=64u)
  if(TARGET cursivLrGen)
    target_compile_definitions(cursivLrGen PRIVATE CURSIV_SET_WORDS=64u)
  endif()
endif()

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/nasm/core/*.cpp)
add_library(nasmCore ${_core})
target_link_libraries(nasmCore PUBLIC nasmGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/nasm/cli/*.cpp)
if(_cli)
  add_executable(nasm_cli ${_cli})
  target_link_libraries(nasm_cli
                        PRIVATE nasmCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/nasm/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/nasm/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/nasm/lsp/main.cpp)
  add_library(nasmLsp ${_lsp})
  target_link_libraries(nasmLsp PUBLIC nasmCore cursiv::lsp)
  add_executable(nasm_lsp nasm/lsp/main.cpp)
  target_link_libraries(nasm_lsp PRIVATE nasmLsp)
endif()

if(TARGET nasmLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nasm/bench/main.cpp")
  cursiv_add_benchmark(nasm
      SOURCES nasm/bench/main.cpp)
endif()

cursiv_package_language(nasm)
