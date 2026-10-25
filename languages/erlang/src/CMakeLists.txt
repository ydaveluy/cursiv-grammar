# Generate the parser and its language module.
cursiv_generate_language(ID erlang
    GRAMMAR    erlang/core/ErlangGrammar.hpp
    NAMESPACE  erlang::Erlang
    EXTENSIONS .erl
    OUT        erlang/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/erlang/core/*.cpp)
add_library(erlangCore ${_core})
target_link_libraries(erlangCore PUBLIC erlangGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/erlang/cli/*.cpp)
if(_cli)
  add_executable(erlang_cli ${_cli})
  target_link_libraries(erlang_cli
                        PRIVATE erlangCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/erlang/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/erlang/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/erlang/lsp/main.cpp)
  add_library(erlangLsp ${_lsp})
  target_link_libraries(erlangLsp PUBLIC erlangCore cursiv::lsp)
  add_executable(erlang_lsp erlang/lsp/main.cpp)
  target_link_libraries(erlang_lsp PRIVATE erlangLsp)
endif()

if(TARGET erlangLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/erlang/bench/main.cpp")
  cursiv_add_benchmark(erlang
      SOURCES erlang/bench/main.cpp)
endif()

cursiv_package_language(erlang)
