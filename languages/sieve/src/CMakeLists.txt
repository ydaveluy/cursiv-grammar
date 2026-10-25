# Generate the parser and its language module.
cursiv_generate_language(ID sieve
    GRAMMAR    sieve/core/SieveGrammar.hpp
    NAMESPACE  sieve::Sieve
    EXTENSIONS .sieve
    OUT        sieve/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sieve/core/*.cpp)
add_library(sieveCore ${_core})
target_link_libraries(sieveCore PUBLIC sieveGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sieve/cli/*.cpp)
if(_cli)
  add_executable(sieve_cli ${_cli})
  target_link_libraries(sieve_cli
                        PRIVATE sieveCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sieve/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/sieve/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sieve/lsp/main.cpp)
  add_library(sieveLsp ${_lsp})
  target_link_libraries(sieveLsp PUBLIC sieveCore cursiv::lsp)
  add_executable(sieve_lsp sieve/lsp/main.cpp)
  target_link_libraries(sieve_lsp PRIVATE sieveLsp)
endif()

if(TARGET sieveLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sieve/bench/main.cpp")
  cursiv_add_benchmark(sieve
      SOURCES sieve/bench/main.cpp)
endif()

cursiv_package_language(sieve)
