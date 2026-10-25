# Generate the parser and its language module.
cursiv_generate_language(ID elixir
    GRAMMAR    elixir/core/ElixirGrammar.hpp
    NAMESPACE  elixir::Elixir
    EXTENSIONS .ex
    OUT        elixir/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/elixir/core/*.cpp)
add_library(elixirCore ${_core})
target_link_libraries(elixirCore PUBLIC elixirGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/elixir/cli/*.cpp)
if(_cli)
  add_executable(elixir_cli ${_cli})
  target_link_libraries(elixir_cli
                        PRIVATE elixirCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/elixir/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/elixir/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/elixir/lsp/main.cpp)
  add_library(elixirLsp ${_lsp})
  target_link_libraries(elixirLsp PUBLIC elixirCore cursiv::lsp)
  add_executable(elixir_lsp elixir/lsp/main.cpp)
  target_link_libraries(elixir_lsp PRIVATE elixirLsp)
endif()

if(TARGET elixirLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/elixir/bench/main.cpp")
  cursiv_add_benchmark(elixir
      SOURCES elixir/bench/main.cpp)
endif()

cursiv_package_language(elixir)
