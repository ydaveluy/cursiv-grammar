# Generate the parser and its language module.
cursiv_generate_language(ID propcalc
    GRAMMAR    propcalc/core/PropcalcGrammar.hpp
    NAMESPACE  propcalc::Propcalc
    EXTENSIONS .propcalc
    OUT        propcalc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/propcalc/core/*.cpp)
add_library(propcalcCore ${_core})
target_link_libraries(propcalcCore PUBLIC propcalcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/propcalc/cli/*.cpp)
if(_cli)
  add_executable(propcalc_cli ${_cli})
  target_link_libraries(propcalc_cli
                        PRIVATE propcalcCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/propcalc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/propcalc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/propcalc/lsp/main.cpp)
  add_library(propcalcLsp ${_lsp})
  target_link_libraries(propcalcLsp PUBLIC propcalcCore cursiv::lsp)
  add_executable(propcalc_lsp propcalc/lsp/main.cpp)
  target_link_libraries(propcalc_lsp PRIVATE propcalcLsp)
endif()

if(TARGET propcalcLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/propcalc/bench/main.cpp")
  cursiv_add_benchmark(propcalc
      SOURCES propcalc/bench/main.cpp)
endif()

cursiv_package_language(propcalc)
