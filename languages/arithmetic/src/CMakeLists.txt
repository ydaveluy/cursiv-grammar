# Generate the parser and its language module.
cursiv_generate_language(ID arithmetic
    GRAMMAR    arithmetic/core/ArithmeticGrammar.hpp
    NAMESPACE  arithmetic::Arithmetic
    EXTENSIONS .arithmetic
    OUT        arithmetic/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/arithmetic/core/*.cpp)
add_library(arithmeticCore ${_core})
target_link_libraries(arithmeticCore PUBLIC arithmeticGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/arithmetic/cli/*.cpp)
if(_cli)
  add_executable(arithmetic_cli ${_cli})
  target_link_libraries(arithmetic_cli
                        PRIVATE arithmeticCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/arithmetic/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/arithmetic/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/arithmetic/lsp/main.cpp)
  add_library(arithmeticLsp ${_lsp})
  target_link_libraries(arithmeticLsp PUBLIC arithmeticCore cursiv::lsp)
  add_executable(arithmetic_lsp arithmetic/lsp/main.cpp)
  target_link_libraries(arithmetic_lsp PRIVATE arithmeticLsp)
endif()

if(TARGET arithmeticLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/arithmetic/bench/main.cpp")
  cursiv_add_benchmark(arithmetic
      SOURCES arithmetic/bench/main.cpp)
endif()

cursiv_package_language(arithmetic)
