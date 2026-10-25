# Generate the parser and its language module.
cursiv_generate_language(ID abnf
    GRAMMAR    abnf/core/AbnfGrammar.hpp
    NAMESPACE  abnf::Abnf
    EXTENSIONS .abnf
    OUT        abnf/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/abnf/core/*.cpp)
add_library(abnfCore ${_core})
target_link_libraries(abnfCore PUBLIC abnfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/abnf/cli/*.cpp)
if(_cli)
  add_executable(abnf_cli ${_cli})
  target_link_libraries(abnf_cli
                        PRIVATE abnfCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/abnf/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/abnf/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/abnf/lsp/main.cpp)
  add_library(abnfLsp ${_lsp})
  target_link_libraries(abnfLsp PUBLIC abnfCore cursiv::lsp)
  add_executable(abnf_lsp abnf/lsp/main.cpp)
  target_link_libraries(abnf_lsp PRIVATE abnfLsp)
endif()

if(TARGET abnfLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/abnf/bench/main.cpp")
  cursiv_add_benchmark(abnf
      SOURCES abnf/bench/main.cpp)
endif()

cursiv_package_language(abnf)
