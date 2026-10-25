# Generate the parser and its language module.
cursiv_generate_language(ID mumath
    GRAMMAR    mumath/core/MumathGrammar.hpp
    NAMESPACE  mumath::Mumath
    EXTENSIONS .mu
    OUT        mumath/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mumath/core/*.cpp)
add_library(mumathCore ${_core})
target_link_libraries(mumathCore PUBLIC mumathGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mumath/cli/*.cpp)
if(_cli)
  add_executable(mumath_cli ${_cli})
  target_link_libraries(mumath_cli
                        PRIVATE mumathCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mumath/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/mumath/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/mumath/lsp/main.cpp)
  add_library(mumathLsp ${_lsp})
  target_link_libraries(mumathLsp PUBLIC mumathCore cursiv::lsp)
  add_executable(mumath_lsp mumath/lsp/main.cpp)
  target_link_libraries(mumath_lsp PRIVATE mumathLsp)
endif()

if(TARGET mumathLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mumath/bench/main.cpp")
  cursiv_add_benchmark(mumath
      SOURCES mumath/bench/main.cpp)
endif()

cursiv_package_language(mumath)
