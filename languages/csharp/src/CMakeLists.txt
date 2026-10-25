# Generate the parser and its language module.
cursiv_generate_language(ID csharp
    GRAMMAR    csharp/core/CSharpGrammar.hpp
    NAMESPACE  csharp::CSharp
    EXTENSIONS .cs
    OUT        csharp/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/csharp/core/*.cpp)
add_library(csharpCore ${_core})
target_link_libraries(csharpCore PUBLIC csharpGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/csharp/cli/*.cpp)
if(_cli)
  add_executable(csharp_cli ${_cli})
  target_link_libraries(csharp_cli
                        PRIVATE csharpCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/csharp/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/csharp/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/csharp/lsp/main.cpp)
  add_library(csharpLsp ${_lsp})
  target_link_libraries(csharpLsp PUBLIC csharpCore cursiv::lsp)
  add_executable(csharp_lsp csharp/lsp/main.cpp)
  target_link_libraries(csharp_lsp PRIVATE csharpLsp)
endif()

if(TARGET csharpLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/csharp/bench/main.cpp")
  cursiv_add_benchmark(csharp
      SOURCES csharp/bench/main.cpp)
endif()

cursiv_package_language(csharp)
