# Generate the parser and its language module.
cursiv_generate_language(ID cayenne
    GRAMMAR    cayenne/core/CayenneGrammar.hpp
    NAMESPACE  cayenne::Cayenne
    EXTENSIONS .cayenne
    OUT        cayenne/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cayenne/core/*.cpp)
add_library(cayenneCore ${_core})
target_link_libraries(cayenneCore PUBLIC cayenneGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cayenne/cli/*.cpp)
if(_cli)
  add_executable(cayenne_cli ${_cli})
  target_link_libraries(cayenne_cli
                        PRIVATE cayenneCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cayenne/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cayenne/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cayenne/lsp/main.cpp)
  add_library(cayenneLsp ${_lsp})
  target_link_libraries(cayenneLsp PUBLIC cayenneCore cursiv::lsp)
  add_executable(cayenne_lsp cayenne/lsp/main.cpp)
  target_link_libraries(cayenne_lsp PRIVATE cayenneLsp)
endif()

if(TARGET cayenneLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cayenne/bench/main.cpp")
  cursiv_add_benchmark(cayenne
      SOURCES cayenne/bench/main.cpp)
endif()

cursiv_package_language(cayenne)
