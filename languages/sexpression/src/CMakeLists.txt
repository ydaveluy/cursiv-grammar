# Generate the parser and its language module.
cursiv_generate_language(ID sexpression
    GRAMMAR    sexpression/core/SexpressionGrammar.hpp
    NAMESPACE  sexpression::Sexpression
    EXTENSIONS .sexpression
    OUT        sexpression/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sexpression/core/*.cpp)
add_library(sexpressionCore ${_core})
target_link_libraries(sexpressionCore PUBLIC sexpressionGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sexpression/cli/*.cpp)
if(_cli)
  add_executable(sexpression_cli ${_cli})
  target_link_libraries(sexpression_cli
                        PRIVATE sexpressionCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sexpression/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/sexpression/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sexpression/lsp/main.cpp)
  add_library(sexpressionLsp ${_lsp})
  target_link_libraries(sexpressionLsp PUBLIC sexpressionCore cursiv::lsp)
  add_executable(sexpression_lsp sexpression/lsp/main.cpp)
  target_link_libraries(sexpression_lsp PRIVATE sexpressionLsp)
endif()

if(TARGET sexpressionLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sexpression/bench/main.cpp")
  cursiv_add_benchmark(sexpression
      SOURCES sexpression/bench/main.cpp)
endif()

cursiv_package_language(sexpression)
