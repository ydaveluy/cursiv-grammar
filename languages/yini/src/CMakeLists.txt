# Generate the parser and its language module.
cursiv_generate_language(ID yini
    GRAMMAR    yini/core/YiniGrammar.hpp
    NAMESPACE  yini::Yini
    EXTENSIONS .yini
    OUT        yini/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/yini/core/*.cpp)
add_library(yiniCore ${_core})
target_link_libraries(yiniCore PUBLIC yiniGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/yini/cli/*.cpp)
if(_cli)
  add_executable(yini_cli ${_cli})
  target_link_libraries(yini_cli
                        PRIVATE yiniCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/yini/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/yini/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/yini/lsp/main.cpp)
  add_library(yiniLsp ${_lsp})
  target_link_libraries(yiniLsp PUBLIC yiniCore cursiv::lsp)
  add_executable(yini_lsp yini/lsp/main.cpp)
  target_link_libraries(yini_lsp PRIVATE yiniLsp)
endif()

if(TARGET yiniLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/yini/bench/main.cpp")
  cursiv_add_benchmark(yini
      SOURCES yini/bench/main.cpp)
endif()

cursiv_package_language(yini)
