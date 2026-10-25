# Generate the parser and its language module.
cursiv_generate_language(ID eiffel
    GRAMMAR    eiffel/core/EiffelGrammar.hpp
    NAMESPACE  eiffel::Eiffel
    EXTENSIONS .e
    OUT        eiffel/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/eiffel/core/*.cpp)
add_library(eiffelCore ${_core})
target_link_libraries(eiffelCore PUBLIC eiffelGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/eiffel/cli/*.cpp)
if(_cli)
  add_executable(eiffel_cli ${_cli})
  target_link_libraries(eiffel_cli
                        PRIVATE eiffelCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/eiffel/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/eiffel/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/eiffel/lsp/main.cpp)
  add_library(eiffelLsp ${_lsp})
  target_link_libraries(eiffelLsp PUBLIC eiffelCore cursiv::lsp)
  add_executable(eiffel_lsp eiffel/lsp/main.cpp)
  target_link_libraries(eiffel_lsp PRIVATE eiffelLsp)
endif()

if(TARGET eiffelLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/eiffel/bench/main.cpp")
  cursiv_add_benchmark(eiffel
      SOURCES eiffel/bench/main.cpp)
endif()

cursiv_package_language(eiffel)
