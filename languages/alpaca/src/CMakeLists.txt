# Generate the parser and its language module.
cursiv_generate_language(ID alpaca
    GRAMMAR    alpaca/core/AlpacaGrammar.hpp
    NAMESPACE  alpaca::Alpaca
    EXTENSIONS .alp
    OUT        alpaca/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alpaca/core/*.cpp)
add_library(alpacaCore ${_core})
target_link_libraries(alpacaCore PUBLIC alpacaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alpaca/cli/*.cpp)
if(_cli)
  add_executable(alpaca_cli ${_cli})
  target_link_libraries(alpaca_cli
                        PRIVATE alpacaCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/alpaca/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/alpaca/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/alpaca/lsp/main.cpp)
  add_library(alpacaLsp ${_lsp})
  target_link_libraries(alpacaLsp PUBLIC alpacaCore cursiv::lsp)
  add_executable(alpaca_lsp alpaca/lsp/main.cpp)
  target_link_libraries(alpaca_lsp PRIVATE alpacaLsp)
endif()

if(TARGET alpacaLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/alpaca/bench/main.cpp")
  cursiv_add_benchmark(alpaca
      SOURCES alpaca/bench/main.cpp)
endif()

cursiv_package_language(alpaca)
