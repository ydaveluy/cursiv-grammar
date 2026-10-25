# Generate the parser and its language module.
cursiv_generate_language(ID python
    GRAMMAR    python/core/PythonGrammar.hpp
    NAMESPACE  python::Python
    EXTENSIONS .py
    OUT        python/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/python/core/*.cpp)
add_library(pythonCore ${_core})
target_link_libraries(pythonCore PUBLIC pythonGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/python/cli/*.cpp)
if(_cli)
  add_executable(python_cli ${_cli})
  target_link_libraries(python_cli
                        PRIVATE pythonCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/python/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/python/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/python/lsp/main.cpp)
  add_library(pythonLsp ${_lsp})
  target_link_libraries(pythonLsp PUBLIC pythonCore cursiv::lsp)
  add_executable(python_lsp python/lsp/main.cpp)
  target_link_libraries(python_lsp PRIVATE pythonLsp)
endif()

if(TARGET pythonLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/python/bench/main.cpp")
  cursiv_add_benchmark(python
      SOURCES python/bench/main.cpp)
endif()

cursiv_package_language(python)
