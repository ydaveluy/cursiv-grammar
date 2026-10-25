# Generate the parser and its language module.
cursiv_generate_language(ID smtlibv2
    GRAMMAR    smtlibv2/core/Smtlibv2Grammar.hpp
    NAMESPACE  smtlibv2::Smtlibv2
    EXTENSIONS .smt2
    OUT        smtlibv2/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smtlibv2/core/*.cpp)
add_library(smtlibv2Core ${_core})
target_link_libraries(smtlibv2Core PUBLIC smtlibv2Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smtlibv2/cli/*.cpp)
if(_cli)
  add_executable(smtlibv2_cli ${_cli})
  target_link_libraries(smtlibv2_cli
                        PRIVATE smtlibv2Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/smtlibv2/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/smtlibv2/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/smtlibv2/lsp/main.cpp)
  add_library(smtlibv2Lsp ${_lsp})
  target_link_libraries(smtlibv2Lsp PUBLIC smtlibv2Core cursiv::lsp)
  add_executable(smtlibv2_lsp smtlibv2/lsp/main.cpp)
  target_link_libraries(smtlibv2_lsp PRIVATE smtlibv2Lsp)
endif()

if(TARGET smtlibv2Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/smtlibv2/bench/main.cpp")
  cursiv_add_benchmark(smtlibv2
      SOURCES smtlibv2/bench/main.cpp)
endif()

cursiv_package_language(smtlibv2)
