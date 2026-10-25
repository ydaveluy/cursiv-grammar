# Generate the parser and its language module.
cursiv_generate_language(ID romannumerals
    GRAMMAR    romannumerals/core/RomannumeralsGrammar.hpp
    NAMESPACE  romannumerals::Romannumerals
    EXTENSIONS .romannumerals
    OUT        romannumerals/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/romannumerals/core/*.cpp)
add_library(romannumeralsCore ${_core})
target_link_libraries(romannumeralsCore PUBLIC romannumeralsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/romannumerals/cli/*.cpp)
if(_cli)
  add_executable(romannumerals_cli ${_cli})
  target_link_libraries(romannumerals_cli
                        PRIVATE romannumeralsCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/romannumerals/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/romannumerals/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/romannumerals/lsp/main.cpp)
  add_library(romannumeralsLsp ${_lsp})
  target_link_libraries(romannumeralsLsp PUBLIC romannumeralsCore cursiv::lsp)
  add_executable(romannumerals_lsp romannumerals/lsp/main.cpp)
  target_link_libraries(romannumerals_lsp PRIVATE romannumeralsLsp)
endif()

if(TARGET romannumeralsLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/romannumerals/bench/main.cpp")
  cursiv_add_benchmark(romannumerals
      SOURCES romannumerals/bench/main.cpp)
endif()

cursiv_package_language(romannumerals)
