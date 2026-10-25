# Generate the parser and its language module.
cursiv_generate_language(ID orwell
    GRAMMAR    orwell/core/OrwellGrammar.hpp
    NAMESPACE  orwell::Orwell
    EXTENSIONS .orwell
    OUT        orwell/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/orwell/core/*.cpp)
add_library(orwellCore ${_core})
target_link_libraries(orwellCore PUBLIC orwellGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/orwell/cli/*.cpp)
if(_cli)
  add_executable(orwell_cli ${_cli})
  target_link_libraries(orwell_cli
                        PRIVATE orwellCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/orwell/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/orwell/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/orwell/lsp/main.cpp)
  add_library(orwellLsp ${_lsp})
  target_link_libraries(orwellLsp PUBLIC orwellCore cursiv::lsp)
  add_executable(orwell_lsp orwell/lsp/main.cpp)
  target_link_libraries(orwell_lsp PRIVATE orwellLsp)
endif()

if(TARGET orwellLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/orwell/bench/main.cpp")
  cursiv_add_benchmark(orwell
      SOURCES orwell/bench/main.cpp)
endif()

cursiv_package_language(orwell)
