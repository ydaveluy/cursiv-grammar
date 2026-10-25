# Generate the parser and its language module.
cursiv_generate_language(ID pegen
    GRAMMAR    pegen/core/PegenGrammar.hpp
    NAMESPACE  pegen::Pegen
    EXTENSIONS .gram
    OUT        pegen/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pegen/core/*.cpp)
add_library(pegenCore ${_core})
target_link_libraries(pegenCore PUBLIC pegenGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pegen/cli/*.cpp)
if(_cli)
  add_executable(pegen_cli ${_cli})
  target_link_libraries(pegen_cli
                        PRIVATE pegenCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pegen/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/pegen/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pegen/lsp/main.cpp)
  add_library(pegenLsp ${_lsp})
  target_link_libraries(pegenLsp PUBLIC pegenCore cursiv::lsp)
  add_executable(pegen_lsp pegen/lsp/main.cpp)
  target_link_libraries(pegen_lsp PRIVATE pegenLsp)
endif()

if(TARGET pegenLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pegen/bench/main.cpp")
  cursiv_add_benchmark(pegen
      SOURCES pegen/bench/main.cpp)
endif()

cursiv_package_language(pegen)
