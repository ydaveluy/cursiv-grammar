# Generate the parser and its language module.
cursiv_generate_language(ID rust
    GRAMMAR    rust/core/RustGrammar.hpp
    NAMESPACE  rust::Rust
    EXTENSIONS .rs
    OUT        rust/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rust/core/*.cpp)
add_library(rustCore ${_core})
target_link_libraries(rustCore PUBLIC rustGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rust/cli/*.cpp)
if(_cli)
  add_executable(rust_cli ${_cli})
  target_link_libraries(rust_cli
                        PRIVATE rustCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rust/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/rust/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rust/lsp/main.cpp)
  add_library(rustLsp ${_lsp})
  target_link_libraries(rustLsp PUBLIC rustCore cursiv::lsp)
  add_executable(rust_lsp rust/lsp/main.cpp)
  target_link_libraries(rust_lsp PRIVATE rustLsp)
endif()

if(TARGET rustLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rust/bench/main.cpp")
  cursiv_add_benchmark(rust
      SOURCES rust/bench/main.cpp)
endif()

cursiv_package_language(rust)
