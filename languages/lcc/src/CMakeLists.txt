# Generate the parser and its language module.
cursiv_generate_language(ID lcc
    GRAMMAR    lcc/core/LccGrammar.hpp
    NAMESPACE  lcc::Lcc
    EXTENSIONS .lcc
    OUT        lcc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lcc/core/*.cpp)
add_library(lccCore ${_core})
target_link_libraries(lccCore PUBLIC lccGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lcc/cli/*.cpp)
if(_cli)
  add_executable(lcc_cli ${_cli})
  target_link_libraries(lcc_cli
                        PRIVATE lccCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lcc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lcc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lcc/lsp/main.cpp)
  add_library(lccLsp ${_lsp})
  target_link_libraries(lccLsp PUBLIC lccCore cursiv::lsp)
  add_executable(lcc_lsp lcc/lsp/main.cpp)
  target_link_libraries(lcc_lsp PRIVATE lccLsp)
endif()

if(TARGET lccLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lcc/bench/main.cpp")
  cursiv_add_benchmark(lcc
      SOURCES lcc/bench/main.cpp)
endif()

cursiv_package_language(lcc)
