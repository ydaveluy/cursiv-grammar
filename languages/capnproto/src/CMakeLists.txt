# Generate the parser and its language module.
cursiv_generate_language(ID capnproto
    GRAMMAR    capnproto/core/CapnprotoGrammar.hpp
    NAMESPACE  capnproto::Capnproto
    EXTENSIONS .capnp
    OUT        capnproto/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/capnproto/core/*.cpp)
add_library(capnprotoCore ${_core})
target_link_libraries(capnprotoCore PUBLIC capnprotoGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/capnproto/cli/*.cpp)
if(_cli)
  add_executable(capnproto_cli ${_cli})
  target_link_libraries(capnproto_cli
                        PRIVATE capnprotoCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/capnproto/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/capnproto/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/capnproto/lsp/main.cpp)
  add_library(capnprotoLsp ${_lsp})
  target_link_libraries(capnprotoLsp PUBLIC capnprotoCore cursiv::lsp)
  add_executable(capnproto_lsp capnproto/lsp/main.cpp)
  target_link_libraries(capnproto_lsp PRIVATE capnprotoLsp)
endif()

if(TARGET capnprotoLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/capnproto/bench/main.cpp")
  cursiv_add_benchmark(capnproto
      SOURCES capnproto/bench/main.cpp)
endif()

cursiv_package_language(capnproto)
