# Generate the parser and its language module.
cursiv_generate_language(ID awk
    GRAMMAR    awk/core/AwkGrammar.hpp
    NAMESPACE  awk::Awk
    EXTENSIONS .awk
    OUT        awk/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/awk/core/*.cpp)
add_library(awkCore ${_core})
target_link_libraries(awkCore PUBLIC awkGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/awk/cli/*.cpp)
if(_cli)
  add_executable(awk_cli ${_cli})
  target_link_libraries(awk_cli
                        PRIVATE awkCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/awk/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/awk/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/awk/lsp/main.cpp)
  add_library(awkLsp ${_lsp})
  target_link_libraries(awkLsp PUBLIC awkCore cursiv::lsp)
  add_executable(awk_lsp awk/lsp/main.cpp)
  target_link_libraries(awk_lsp PRIVATE awkLsp)
endif()

if(TARGET awkLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/awk/bench/main.cpp")
  cursiv_add_benchmark(awk
      SOURCES awk/bench/main.cpp)
endif()

cursiv_package_language(awk)
