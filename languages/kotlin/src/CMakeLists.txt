# Generate the parser and its language module.
cursiv_generate_language(ID kotlin
    GRAMMAR    kotlin/core/KotlinGrammar.hpp
    NAMESPACE  kotlin::Kotlin
    EXTENSIONS .kt
    OUT        kotlin/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kotlin/core/*.cpp)
add_library(kotlinCore ${_core})
target_link_libraries(kotlinCore PUBLIC kotlinGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kotlin/cli/*.cpp)
if(_cli)
  add_executable(kotlin_cli ${_cli})
  target_link_libraries(kotlin_cli
                        PRIVATE kotlinCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/kotlin/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/kotlin/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/kotlin/lsp/main.cpp)
  add_library(kotlinLsp ${_lsp})
  target_link_libraries(kotlinLsp PUBLIC kotlinCore cursiv::lsp)
  add_executable(kotlin_lsp kotlin/lsp/main.cpp)
  target_link_libraries(kotlin_lsp PRIVATE kotlinLsp)
endif()

if(TARGET kotlinLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/kotlin/bench/main.cpp")
  cursiv_add_benchmark(kotlin
      SOURCES kotlin/bench/main.cpp)
endif()

cursiv_package_language(kotlin)
