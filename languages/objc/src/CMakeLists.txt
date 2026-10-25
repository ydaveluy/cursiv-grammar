# Generate the parser and its language module.
cursiv_generate_language(ID objc
    GRAMMAR    objc/core/ObjcGrammar.hpp
    NAMESPACE  objc::Objc
    EXTENSIONS .m
    OUT        objc/generated)

# The AST bindings exceed GCC's default constexpr evaluation budget.
target_compile_options(objcGenerated PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-fconstexpr-ops-limit=2147483647>)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/objc/core/*.cpp)
add_library(objcCore ${_core})
target_link_libraries(objcCore PUBLIC objcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/objc/cli/*.cpp)
if(_cli)
  add_executable(objc_cli ${_cli})
  target_link_libraries(objc_cli
                        PRIVATE objcCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/objc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/objc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/objc/lsp/main.cpp)
  add_library(objcLsp ${_lsp})
  target_link_libraries(objcLsp PUBLIC objcCore cursiv::lsp)
  add_executable(objc_lsp objc/lsp/main.cpp)
  target_link_libraries(objc_lsp PRIVATE objcLsp)
endif()

if(TARGET objcLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/objc/bench/main.cpp")
  cursiv_add_benchmark(objc
      SOURCES objc/bench/main.cpp)
endif()

cursiv_package_language(objc)
