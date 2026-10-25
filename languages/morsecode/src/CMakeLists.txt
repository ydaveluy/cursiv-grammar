# Generate the parser and its language module.
cursiv_generate_language(ID morsecode
    GRAMMAR    morsecode/core/MorsecodeGrammar.hpp
    NAMESPACE  morsecode::Morsecode
    EXTENSIONS .morsecode
    OUT        morsecode/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/morsecode/core/*.cpp)
add_library(morsecodeCore ${_core})
target_link_libraries(morsecodeCore PUBLIC morsecodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/morsecode/cli/*.cpp)
if(_cli)
  add_executable(morsecode_cli ${_cli})
  target_link_libraries(morsecode_cli
                        PRIVATE morsecodeCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/morsecode/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/morsecode/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/morsecode/lsp/main.cpp)
  add_library(morsecodeLsp ${_lsp})
  target_link_libraries(morsecodeLsp PUBLIC morsecodeCore cursiv::lsp)
  add_executable(morsecode_lsp morsecode/lsp/main.cpp)
  target_link_libraries(morsecode_lsp PRIVATE morsecodeLsp)
endif()

if(TARGET morsecodeLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/morsecode/bench/main.cpp")
  cursiv_add_benchmark(morsecode
      SOURCES morsecode/bench/main.cpp)
endif()

cursiv_package_language(morsecode)
