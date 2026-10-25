# Generate the parser and its language module.
cursiv_generate_language(ID informix
    GRAMMAR    informix/core/InformixGrammar.hpp
    NAMESPACE  informix::Informix
    EXTENSIONS .4gl
    OUT        informix/generated)

# The AST bindings exceed GCC's default constexpr evaluation budget.
target_compile_options(informixGenerated PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-fconstexpr-ops-limit=2147483647>)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/informix/core/*.cpp)
add_library(informixCore ${_core})
target_link_libraries(informixCore PUBLIC informixGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/informix/cli/*.cpp)
if(_cli)
  add_executable(informix_cli ${_cli})
  target_link_libraries(informix_cli
                        PRIVATE informixCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/informix/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/informix/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/informix/lsp/main.cpp)
  add_library(informixLsp ${_lsp})
  target_link_libraries(informixLsp PUBLIC informixCore cursiv::lsp)
  add_executable(informix_lsp informix/lsp/main.cpp)
  target_link_libraries(informix_lsp PRIVATE informixLsp)
endif()

if(TARGET informixLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/informix/bench/main.cpp")
  cursiv_add_benchmark(informix
      SOURCES informix/bench/main.cpp)
endif()

cursiv_package_language(informix)
