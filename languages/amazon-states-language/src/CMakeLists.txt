# Generate the parser and its language module.
cursiv_generate_language(ID amazonstateslanguage
    GRAMMAR    amazonstateslanguage/core/AmazonStatesLanguageGrammar.hpp
    NAMESPACE  amazonstateslanguage::AmazonStatesLanguage
    EXTENSIONS .asl
    OUT        amazonstateslanguage/generated)

# The field-name vocabulary (~120 keyword terminals sharing a handful of
# large alternations) needs more than the compiler's default template
# recursion depth.
if(CMAKE_CROSSCOMPILING)
  cursiv_host_manifest("target_compile_options(amazonstateslanguage_gen PRIVATE -ftemplate-depth=4000)")
else()
  target_compile_options(amazonstateslanguage_gen PRIVATE -ftemplate-depth=4000)
endif()
target_compile_options(amazonstateslanguageGenerated PRIVATE -ftemplate-depth=4000)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguage/core/*.cpp)
add_library(amazonstateslanguageCore ${_core})
target_link_libraries(amazonstateslanguageCore PUBLIC amazonstateslanguageGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguage/cli/*.cpp)
if(_cli)
  add_executable(amazonstateslanguage_cli ${_cli})
  target_link_libraries(amazonstateslanguage_cli
                        PRIVATE amazonstateslanguageCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguage/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguage/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguage/lsp/main.cpp)
  add_library(amazonstateslanguageLsp ${_lsp})
  target_link_libraries(amazonstateslanguageLsp PUBLIC amazonstateslanguageCore cursiv::lsp)
  add_executable(amazonstateslanguage_lsp amazonstateslanguage/lsp/main.cpp)
  target_link_libraries(amazonstateslanguage_lsp PRIVATE amazonstateslanguageLsp)
endif()

if(TARGET amazonstateslanguageLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguage/bench/main.cpp")
  cursiv_add_benchmark(amazonstateslanguage
      SOURCES amazonstateslanguage/bench/main.cpp)
endif()

cursiv_package_language(amazonstateslanguage)
