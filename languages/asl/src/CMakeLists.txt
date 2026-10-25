# Generate the parser and its language module.
cursiv_generate_language(ID asl
    GRAMMAR    asl/core/AslGrammar.hpp
    NAMESPACE  asl::Asl
    EXTENSIONS .asl
    OUT        asl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/asl/core/*.cpp)
add_library(aslCore ${_core})
target_link_libraries(aslCore PUBLIC aslGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/asl/cli/*.cpp)
if(_cli)
  add_executable(asl_cli ${_cli})
  target_link_libraries(asl_cli
                        PRIVATE aslCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/asl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/asl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/asl/lsp/main.cpp)
  add_library(aslLsp ${_lsp})
  target_link_libraries(aslLsp PUBLIC aslCore cursiv::lsp)
  add_executable(asl_lsp asl/lsp/main.cpp)
  target_link_libraries(asl_lsp PRIVATE aslLsp)
endif()

if(TARGET aslLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/asl/bench/main.cpp")
  cursiv_add_benchmark(asl
      SOURCES asl/bench/main.cpp)
endif()

cursiv_package_language(asl)
