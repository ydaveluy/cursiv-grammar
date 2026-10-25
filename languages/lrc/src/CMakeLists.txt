# Generate the parser and its language module.
cursiv_generate_language(ID lrc
    GRAMMAR    lrc/core/LrcGrammar.hpp
    NAMESPACE  lrc::Lrc
    EXTENSIONS .lrc
    OUT        lrc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lrc/core/*.cpp)
add_library(lrcCore ${_core})
target_link_libraries(lrcCore PUBLIC lrcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lrc/cli/*.cpp)
if(_cli)
  add_executable(lrc_cli ${_cli})
  target_link_libraries(lrc_cli
                        PRIVATE lrcCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lrc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lrc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lrc/lsp/main.cpp)
  add_library(lrcLsp ${_lsp})
  target_link_libraries(lrcLsp PUBLIC lrcCore cursiv::lsp)
  add_executable(lrc_lsp lrc/lsp/main.cpp)
  target_link_libraries(lrc_lsp PRIVATE lrcLsp)
endif()

if(TARGET lrcLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lrc/bench/main.cpp")
  cursiv_add_benchmark(lrc
      SOURCES lrc/bench/main.cpp)
endif()

cursiv_package_language(lrc)
