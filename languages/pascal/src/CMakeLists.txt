# Generate the parser and its language module.
cursiv_generate_language(ID pascal
    GRAMMAR    pascal/core/PascalGrammar.hpp
    NAMESPACE  pascal::Pascal
    EXTENSIONS .pas
    OUT        pascal/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pascal/core/*.cpp)
add_library(pascalCore ${_core})
target_link_libraries(pascalCore PUBLIC pascalGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pascal/cli/*.cpp)
if(_cli)
  add_executable(pascal_cli ${_cli})
  target_link_libraries(pascal_cli
                        PRIVATE pascalCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pascal/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/pascal/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pascal/lsp/main.cpp)
  add_library(pascalLsp ${_lsp})
  target_link_libraries(pascalLsp PUBLIC pascalCore cursiv::lsp)
  add_executable(pascal_lsp pascal/lsp/main.cpp)
  target_link_libraries(pascal_lsp PRIVATE pascalLsp)
endif()

if(TARGET pascalLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pascal/bench/main.cpp")
  cursiv_add_benchmark(pascal
      SOURCES pascal/bench/main.cpp)
endif()

cursiv_package_language(pascal)
