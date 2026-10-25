# Generate the parser and its language module.
cursiv_generate_language(ID lisp
    GRAMMAR    lisp/core/LispGrammar.hpp
    NAMESPACE  lisp::Lisp
    EXTENSIONS .lisp
    OUT        lisp/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lisp/core/*.cpp)
add_library(lispCore ${_core})
target_link_libraries(lispCore PUBLIC lispGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lisp/cli/*.cpp)
if(_cli)
  add_executable(lisp_cli ${_cli})
  target_link_libraries(lisp_cli
                        PRIVATE lispCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lisp/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lisp/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lisp/lsp/main.cpp)
  add_library(lispLsp ${_lsp})
  target_link_libraries(lispLsp PUBLIC lispCore cursiv::lsp)
  add_executable(lisp_lsp lisp/lsp/main.cpp)
  target_link_libraries(lisp_lsp PRIVATE lispLsp)
endif()

if(TARGET lispLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lisp/bench/main.cpp")
  cursiv_add_benchmark(lisp
      SOURCES lisp/bench/main.cpp)
endif()

cursiv_package_language(lisp)
