# Generate the parser and its language module.
cursiv_generate_language(ID bencoding
    GRAMMAR    bencoding/core/BencodingGrammar.hpp
    NAMESPACE  bencoding::Bencoding
    EXTENSIONS .torrent
    OUT        bencoding/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bencoding/core/*.cpp)
add_library(bencodingCore ${_core})
target_link_libraries(bencodingCore PUBLIC bencodingGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bencoding/cli/*.cpp)
if(_cli)
  add_executable(bencoding_cli ${_cli})
  target_link_libraries(bencoding_cli
                        PRIVATE bencodingCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bencoding/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/bencoding/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bencoding/lsp/main.cpp)
  add_library(bencodingLsp ${_lsp})
  target_link_libraries(bencodingLsp PUBLIC bencodingCore cursiv::lsp)
  add_executable(bencoding_lsp bencoding/lsp/main.cpp)
  target_link_libraries(bencoding_lsp PRIVATE bencodingLsp)
endif()

if(TARGET bencodingLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bencoding/bench/main.cpp")
  cursiv_add_benchmark(bencoding
      SOURCES bencoding/bench/main.cpp)
endif()

cursiv_package_language(bencoding)
