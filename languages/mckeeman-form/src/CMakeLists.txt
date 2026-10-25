# Generate the parser and its language module.
cursiv_generate_language(ID mckeemanform
    GRAMMAR    mckeemanform/core/MckeemanFormGrammar.hpp
    NAMESPACE  mckeemanform::MckeemanForm
    EXTENSIONS .mckeemanform
    OUT        mckeemanform/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mckeemanform/core/*.cpp)
add_library(mckeemanformCore ${_core})
target_link_libraries(mckeemanformCore PUBLIC mckeemanformGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mckeemanform/cli/*.cpp)
if(_cli)
  add_executable(mckeemanform_cli ${_cli})
  target_link_libraries(mckeemanform_cli
                        PRIVATE mckeemanformCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mckeemanform/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/mckeemanform/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/mckeemanform/lsp/main.cpp)
  add_library(mckeemanformLsp ${_lsp})
  target_link_libraries(mckeemanformLsp PUBLIC mckeemanformCore cursiv::lsp)
  add_executable(mckeemanform_lsp mckeemanform/lsp/main.cpp)
  target_link_libraries(mckeemanform_lsp PRIVATE mckeemanformLsp)
endif()

if(TARGET mckeemanformLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mckeemanform/bench/main.cpp")
  cursiv_add_benchmark(mckeemanform
      SOURCES mckeemanform/bench/main.cpp)
endif()

cursiv_package_language(mckeemanform)
