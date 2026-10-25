# Generate the parser and its language module.
cursiv_generate_language(ID qif
    GRAMMAR    qif/core/QifGrammar.hpp
    NAMESPACE  qif::Qif
    EXTENSIONS .qif
    OUT        qif/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/qif/core/*.cpp)
add_library(qifCore ${_core})
target_link_libraries(qifCore PUBLIC qifGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/qif/cli/*.cpp)
if(_cli)
  add_executable(qif_cli ${_cli})
  target_link_libraries(qif_cli
                        PRIVATE qifCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/qif/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/qif/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/qif/lsp/main.cpp)
  add_library(qifLsp ${_lsp})
  target_link_libraries(qifLsp PUBLIC qifCore cursiv::lsp)
  add_executable(qif_lsp qif/lsp/main.cpp)
  target_link_libraries(qif_lsp PRIVATE qifLsp)
endif()

if(TARGET qifLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/qif/bench/main.cpp")
  cursiv_add_benchmark(qif
      SOURCES qif/bench/main.cpp)
endif()

cursiv_package_language(qif)
