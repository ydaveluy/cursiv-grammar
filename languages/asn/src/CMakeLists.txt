# Generate the parser and its language module.
cursiv_generate_language(ID asn
    GRAMMAR    asn/core/AsnGrammar.hpp
    NAMESPACE  asn::Asn
    EXTENSIONS .asn
    OUT        asn/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/asn/core/*.cpp)
add_library(asnCore ${_core})
target_link_libraries(asnCore PUBLIC asnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/asn/cli/*.cpp)
if(_cli)
  add_executable(asn_cli ${_cli})
  target_link_libraries(asn_cli
                        PRIVATE asnCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/asn/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/asn/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/asn/lsp/main.cpp)
  add_library(asnLsp ${_lsp})
  target_link_libraries(asnLsp PUBLIC asnCore cursiv::lsp)
  add_executable(asn_lsp asn/lsp/main.cpp)
  target_link_libraries(asn_lsp PRIVATE asnLsp)
endif()

if(TARGET asnLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/asn/bench/main.cpp")
  cursiv_add_benchmark(asn
      SOURCES asn/bench/main.cpp)
endif()

cursiv_package_language(asn)
