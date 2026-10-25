# Generate the parser and its language module.
cursiv_generate_language(ID callable
    GRAMMAR    callable/core/CallableGrammar.hpp
    NAMESPACE  callable::Callable
    EXTENSIONS .callable
    OUT        callable/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/callable/core/*.cpp)
add_library(callableCore ${_core})
target_link_libraries(callableCore PUBLIC callableGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/callable/cli/*.cpp)
if(_cli)
  add_executable(callable_cli ${_cli})
  target_link_libraries(callable_cli
                        PRIVATE callableCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/callable/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/callable/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/callable/lsp/main.cpp)
  add_library(callableLsp ${_lsp})
  target_link_libraries(callableLsp PUBLIC callableCore cursiv::lsp)
  add_executable(callable_lsp callable/lsp/main.cpp)
  target_link_libraries(callable_lsp PRIVATE callableLsp)
endif()

if(TARGET callableLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/callable/bench/main.cpp")
  cursiv_add_benchmark(callable
      SOURCES callable/bench/main.cpp)
endif()

cursiv_package_language(callable)
