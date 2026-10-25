# Generate the parser and its language module.
cursiv_generate_language(ID snobol
    GRAMMAR    snobol/core/SnobolGrammar.hpp
    NAMESPACE  snobol::Snobol
    EXTENSIONS .sno
    OUT        snobol/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/snobol/core/*.cpp)
add_library(snobolCore ${_core})
target_link_libraries(snobolCore PUBLIC snobolGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/snobol/cli/*.cpp)
if(_cli)
  add_executable(snobol_cli ${_cli})
  target_link_libraries(snobol_cli
                        PRIVATE snobolCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/snobol/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/snobol/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/snobol/lsp/main.cpp)
  add_library(snobolLsp ${_lsp})
  target_link_libraries(snobolLsp PUBLIC snobolCore cursiv::lsp)
  add_executable(snobol_lsp snobol/lsp/main.cpp)
  target_link_libraries(snobol_lsp PRIVATE snobolLsp)
endif()

if(TARGET snobolLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/snobol/bench/main.cpp")
  cursiv_add_benchmark(snobol
      SOURCES snobol/bench/main.cpp)
endif()

cursiv_package_language(snobol)
