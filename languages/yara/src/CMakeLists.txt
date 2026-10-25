# Generate the parser and its language module.
cursiv_generate_language(ID yara
    GRAMMAR    yara/core/YaraGrammar.hpp
    NAMESPACE  yara::Yara
    EXTENSIONS .yar
    OUT        yara/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/yara/core/*.cpp)
add_library(yaraCore ${_core})
target_link_libraries(yaraCore PUBLIC yaraGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/yara/cli/*.cpp)
if(_cli)
  add_executable(yara_cli ${_cli})
  target_link_libraries(yara_cli
                        PRIVATE yaraCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/yara/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/yara/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/yara/lsp/main.cpp)
  add_library(yaraLsp ${_lsp})
  target_link_libraries(yaraLsp PUBLIC yaraCore cursiv::lsp)
  add_executable(yara_lsp yara/lsp/main.cpp)
  target_link_libraries(yara_lsp PRIVATE yaraLsp)
endif()

if(TARGET yaraLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/yara/bench/main.cpp")
  cursiv_add_benchmark(yara
      SOURCES yara/bench/main.cpp)
endif()

cursiv_package_language(yara)
