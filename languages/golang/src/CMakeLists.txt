# Generate the parser and its language module.
cursiv_generate_language(ID golang
    GRAMMAR    golang/core/GolangGrammar.hpp
    NAMESPACE  golang::Golang
    EXTENSIONS .go
    OUT        golang/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/golang/core/*.cpp)
add_library(golangCore ${_core})
target_link_libraries(golangCore PUBLIC golangGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/golang/cli/*.cpp)
if(_cli)
  add_executable(golang_cli ${_cli})
  target_link_libraries(golang_cli
                        PRIVATE golangCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/golang/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/golang/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/golang/lsp/main.cpp)
  add_library(golangLsp ${_lsp})
  target_link_libraries(golangLsp PUBLIC golangCore cursiv::lsp)
  add_executable(golang_lsp golang/lsp/main.cpp)
  target_link_libraries(golang_lsp PRIVATE golangLsp)
endif()

if(TARGET golangLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/golang/bench/main.cpp")
  cursiv_add_benchmark(golang
      SOURCES golang/bench/main.cpp)
endif()

cursiv_package_language(golang)
