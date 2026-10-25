# Generate the parser and its language module.
cursiv_generate_language(ID stacktrace
    GRAMMAR    stacktrace/core/StacktraceGrammar.hpp
    NAMESPACE  stacktrace::Stacktrace
    EXTENSIONS .stacktrace
    OUT        stacktrace/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stacktrace/core/*.cpp)
add_library(stacktraceCore ${_core})
target_link_libraries(stacktraceCore PUBLIC stacktraceGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stacktrace/cli/*.cpp)
if(_cli)
  add_executable(stacktrace_cli ${_cli})
  target_link_libraries(stacktrace_cli
                        PRIVATE stacktraceCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/stacktrace/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/stacktrace/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/stacktrace/lsp/main.cpp)
  add_library(stacktraceLsp ${_lsp})
  target_link_libraries(stacktraceLsp PUBLIC stacktraceCore cursiv::lsp)
  add_executable(stacktrace_lsp stacktrace/lsp/main.cpp)
  target_link_libraries(stacktrace_lsp PRIVATE stacktraceLsp)
endif()

if(TARGET stacktraceLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stacktrace/bench/main.cpp")
  cursiv_add_benchmark(stacktrace
      SOURCES stacktrace/bench/main.cpp)
endif()

cursiv_package_language(stacktrace)
