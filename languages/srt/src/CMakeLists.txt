# Generate the parser and its language module.
cursiv_generate_language(ID srt
    GRAMMAR    srt/core/SrtGrammar.hpp
    NAMESPACE  srt::Srt
    EXTENSIONS .srt
    OUT        srt/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/srt/core/*.cpp)
add_library(srtCore ${_core})
target_link_libraries(srtCore PUBLIC srtGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/srt/cli/*.cpp)
if(_cli)
  add_executable(srt_cli ${_cli})
  target_link_libraries(srt_cli
                        PRIVATE srtCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/srt/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/srt/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/srt/lsp/main.cpp)
  add_library(srtLsp ${_lsp})
  target_link_libraries(srtLsp PUBLIC srtCore cursiv::lsp)
  add_executable(srt_lsp srt/lsp/main.cpp)
  target_link_libraries(srt_lsp PRIVATE srtLsp)
endif()

if(TARGET srtLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/srt/bench/main.cpp")
  cursiv_add_benchmark(srt
      SOURCES srt/bench/main.cpp)
endif()

cursiv_package_language(srt)
