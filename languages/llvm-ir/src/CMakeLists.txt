# Generate the parser and its language module.
cursiv_generate_language(ID llvmir
    GRAMMAR    llvmir/core/LlvmIrGrammar.hpp
    NAMESPACE  llvmir::LlvmIr
    EXTENSIONS .ll
    OUT        llvmir/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/llvmir/core/*.cpp)
add_library(llvmirCore ${_core})
target_link_libraries(llvmirCore PUBLIC llvmirGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/llvmir/cli/*.cpp)
if(_cli)
  add_executable(llvmir_cli ${_cli})
  target_link_libraries(llvmir_cli
                        PRIVATE llvmirCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/llvmir/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/llvmir/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/llvmir/lsp/main.cpp)
  add_library(llvmirLsp ${_lsp})
  target_link_libraries(llvmirLsp PUBLIC llvmirCore cursiv::lsp)
  add_executable(llvmir_lsp llvmir/lsp/main.cpp)
  target_link_libraries(llvmir_lsp PRIVATE llvmirLsp)
endif()

if(TARGET llvmirLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llvmir/bench/main.cpp")
  cursiv_add_benchmark(llvmir
      SOURCES llvmir/bench/main.cpp)
endif()

cursiv_package_language(llvmir)
