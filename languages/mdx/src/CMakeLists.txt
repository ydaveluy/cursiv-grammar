# Generate the parser and its language module.
cursiv_generate_language(ID mdx
    GRAMMAR    mdx/core/MdxGrammar.hpp
    NAMESPACE  mdx::Mdx
    EXTENSIONS .mdx
    OUT        mdx/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mdx/core/*.cpp)
add_library(mdxCore ${_core})
target_link_libraries(mdxCore PUBLIC mdxGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mdx/cli/*.cpp)
if(_cli)
  add_executable(mdx_cli ${_cli})
  target_link_libraries(mdx_cli
                        PRIVATE mdxCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mdx/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/mdx/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/mdx/lsp/main.cpp)
  add_library(mdxLsp ${_lsp})
  target_link_libraries(mdxLsp PUBLIC mdxCore cursiv::lsp)
  add_executable(mdx_lsp mdx/lsp/main.cpp)
  target_link_libraries(mdx_lsp PRIVATE mdxLsp)
endif()

if(TARGET mdxLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mdx/bench/main.cpp")
  cursiv_add_benchmark(mdx
      SOURCES mdx/bench/main.cpp)
endif()

cursiv_package_language(mdx)
