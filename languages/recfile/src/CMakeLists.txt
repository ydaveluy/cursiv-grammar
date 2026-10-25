# Generate the parser and its language module.
cursiv_generate_language(ID recfile
    GRAMMAR    recfile/core/RecfileGrammar.hpp
    NAMESPACE  recfile::Recfile
    EXTENSIONS .rec
    OUT        recfile/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/recfile/core/*.cpp)
add_library(recfileCore ${_core})
target_link_libraries(recfileCore PUBLIC recfileGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/recfile/cli/*.cpp)
if(_cli)
  add_executable(recfile_cli ${_cli})
  target_link_libraries(recfile_cli
                        PRIVATE recfileCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/recfile/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/recfile/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/recfile/lsp/main.cpp)
  add_library(recfileLsp ${_lsp})
  target_link_libraries(recfileLsp PUBLIC recfileCore cursiv::lsp)
  add_executable(recfile_lsp recfile/lsp/main.cpp)
  target_link_libraries(recfile_lsp PRIVATE recfileLsp)
endif()

if(TARGET recfileLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/recfile/bench/main.cpp")
  cursiv_add_benchmark(recfile
      SOURCES recfile/bench/main.cpp)
endif()

cursiv_package_language(recfile)
