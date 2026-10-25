# Generate the parser and its language module.
cursiv_generate_language(ID vmf
    GRAMMAR    vmf/core/VmfGrammar.hpp
    NAMESPACE  vmf::Vmf
    EXTENSIONS .vmf
    OUT        vmf/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vmf/core/*.cpp)
add_library(vmfCore ${_core})
target_link_libraries(vmfCore PUBLIC vmfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vmf/cli/*.cpp)
if(_cli)
  add_executable(vmf_cli ${_cli})
  target_link_libraries(vmf_cli
                        PRIVATE vmfCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vmf/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/vmf/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/vmf/lsp/main.cpp)
  add_library(vmfLsp ${_lsp})
  target_link_libraries(vmfLsp PUBLIC vmfCore cursiv::lsp)
  add_executable(vmf_lsp vmf/lsp/main.cpp)
  target_link_libraries(vmf_lsp PRIVATE vmfLsp)
endif()

if(TARGET vmfLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vmf/bench/main.cpp")
  cursiv_add_benchmark(vmf
      SOURCES vmf/bench/main.cpp)
endif()

cursiv_package_language(vmf)
