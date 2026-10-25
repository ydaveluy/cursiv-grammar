# Generate the parser and its language module.
cursiv_generate_language(ID ada
    GRAMMAR    ada/core/AdaGrammar.hpp
    NAMESPACE  ada::Ada
    EXTENSIONS .ada
    OUT        ada/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ada/core/*.cpp)
add_library(adaCore ${_core})
target_link_libraries(adaCore PUBLIC adaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ada/cli/*.cpp)
if(_cli)
  add_executable(ada_cli ${_cli})
  target_link_libraries(ada_cli
                        PRIVATE adaCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ada/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/ada/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ada/lsp/main.cpp)
  add_library(adaLsp ${_lsp})
  target_link_libraries(adaLsp PUBLIC adaCore cursiv::lsp)
  add_executable(ada_lsp ada/lsp/main.cpp)
  target_link_libraries(ada_lsp PRIVATE adaLsp)
endif()

if(TARGET adaLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ada/bench/main.cpp")
  cursiv_add_benchmark(ada
      SOURCES ada/bench/main.cpp)
endif()

cursiv_package_language(ada)
