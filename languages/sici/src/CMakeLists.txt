# Generate the parser and its language module.
cursiv_generate_language(ID sici
    GRAMMAR    sici/core/SiciGrammar.hpp
    NAMESPACE  sici::Sici
    EXTENSIONS .sici
    OUT        sici/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sici/core/*.cpp)
add_library(siciCore ${_core})
target_link_libraries(siciCore PUBLIC siciGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sici/cli/*.cpp)
if(_cli)
  add_executable(sici_cli ${_cli})
  target_link_libraries(sici_cli
                        PRIVATE siciCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sici/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/sici/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sici/lsp/main.cpp)
  add_library(siciLsp ${_lsp})
  target_link_libraries(siciLsp PUBLIC siciCore cursiv::lsp)
  add_executable(sici_lsp sici/lsp/main.cpp)
  target_link_libraries(sici_lsp PRIVATE siciLsp)
endif()

if(TARGET siciLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sici/bench/main.cpp")
  cursiv_add_benchmark(sici
      SOURCES sici/bench/main.cpp)
endif()

cursiv_package_language(sici)
