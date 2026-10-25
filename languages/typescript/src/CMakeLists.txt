# Generate the parser and its language module.
cursiv_generate_language(ID typescript
    GRAMMAR    typescript/core/TypeScriptGrammar.hpp
    NAMESPACE  typescript::TypeScript
    EXTENSIONS .ts
    OUT        typescript/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/typescript/core/*.cpp)
add_library(typescriptCore ${_core})
target_link_libraries(typescriptCore PUBLIC typescriptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/typescript/cli/*.cpp)
if(_cli)
  add_executable(typescript_cli ${_cli})
  target_link_libraries(typescript_cli
                        PRIVATE typescriptCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/typescript/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/typescript/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/typescript/lsp/main.cpp)
  add_library(typescriptLsp ${_lsp})
  target_link_libraries(typescriptLsp PUBLIC typescriptCore cursiv::lsp)
  add_executable(typescript_lsp typescript/lsp/main.cpp)
  target_link_libraries(typescript_lsp PRIVATE typescriptLsp)
endif()

if(TARGET typescriptLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/typescript/bench/main.cpp")
  cursiv_add_benchmark(typescript
      SOURCES typescript/bench/main.cpp)
endif()

cursiv_package_language(typescript)
