# Generate the parser and its language module.
cursiv_generate_language(ID cpp
    GRAMMAR    cpp/core/CppGrammar.hpp
    NAMESPACE  cpp::Cpp
    EXTENSIONS .cpp
    OUT        cpp/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cpp/core/*.cpp)
add_library(cppCore ${_core})
target_link_libraries(cppCore PUBLIC cppGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cli/*.cpp)
if(_cli)
  add_executable(cpp_cli ${_cli})
  target_link_libraries(cpp_cli
                        PRIVATE cppCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cpp/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cpp/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cpp/lsp/main.cpp)
  add_library(cppLsp ${_lsp})
  target_link_libraries(cppLsp PUBLIC cppCore cursiv::lsp)
  add_executable(cpp_lsp cpp/lsp/main.cpp)
  target_link_libraries(cpp_lsp PRIVATE cppLsp)
endif()

if(TARGET cppLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cpp/bench/main.cpp")
  cursiv_add_benchmark(cpp
      SOURCES cpp/bench/main.cpp)
endif()

cursiv_package_language(cpp)
