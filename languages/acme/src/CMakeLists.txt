# Generate the parser and its language module.
cursiv_generate_language(ID acme
    GRAMMAR    acme/core/AcmeGrammar.hpp
    NAMESPACE  acme::Acme
    EXTENSIONS .acme
    OUT        acme/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/acme/core/*.cpp)
add_library(acmeCore ${_core})
target_link_libraries(acmeCore PUBLIC acmeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/acme/cli/*.cpp)
if(_cli)
  add_executable(acme_cli ${_cli})
  target_link_libraries(acme_cli
                        PRIVATE acmeCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/acme/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/acme/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/acme/lsp/main.cpp)
  add_library(acmeLsp ${_lsp})
  target_link_libraries(acmeLsp PUBLIC acmeCore cursiv::lsp)
  add_executable(acme_lsp acme/lsp/main.cpp)
  target_link_libraries(acme_lsp PRIVATE acmeLsp)
endif()

if(TARGET acmeLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/acme/bench/main.cpp")
  cursiv_add_benchmark(acme
      SOURCES acme/bench/main.cpp)
endif()

cursiv_package_language(acme)
