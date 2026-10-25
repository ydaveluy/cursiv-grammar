# Generate the parser and its language module.
cursiv_generate_language(ID hypertalk
    GRAMMAR    hypertalk/core/HypertalkGrammar.hpp
    NAMESPACE  hypertalk::Hypertalk
    EXTENSIONS .hypertalk
    OUT        hypertalk/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/hypertalk/core/*.cpp)
add_library(hypertalkCore ${_core})
target_link_libraries(hypertalkCore PUBLIC hypertalkGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/hypertalk/cli/*.cpp)
if(_cli)
  add_executable(hypertalk_cli ${_cli})
  target_link_libraries(hypertalk_cli
                        PRIVATE hypertalkCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/hypertalk/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/hypertalk/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/hypertalk/lsp/main.cpp)
  add_library(hypertalkLsp ${_lsp})
  target_link_libraries(hypertalkLsp PUBLIC hypertalkCore cursiv::lsp)
  add_executable(hypertalk_lsp hypertalk/lsp/main.cpp)
  target_link_libraries(hypertalk_lsp PRIVATE hypertalkLsp)
endif()

if(TARGET hypertalkLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hypertalk/bench/main.cpp")
  cursiv_add_benchmark(hypertalk
      SOURCES hypertalk/bench/main.cpp)
endif()

cursiv_package_language(hypertalk)
