# Generate the parser and its language module.
cursiv_generate_language(ID swiftfin
    GRAMMAR    swiftfin/core/SwiftFinGrammar.hpp
    NAMESPACE  swiftfin::SwiftFin
    EXTENSIONS .swiftfin
    OUT        swiftfin/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/swiftfin/core/*.cpp)
add_library(swiftfinCore ${_core})
target_link_libraries(swiftfinCore PUBLIC swiftfinGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/swiftfin/cli/*.cpp)
if(_cli)
  add_executable(swiftfin_cli ${_cli})
  target_link_libraries(swiftfin_cli
                        PRIVATE swiftfinCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/swiftfin/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/swiftfin/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/swiftfin/lsp/main.cpp)
  add_library(swiftfinLsp ${_lsp})
  target_link_libraries(swiftfinLsp PUBLIC swiftfinCore cursiv::lsp)
  add_executable(swiftfin_lsp swiftfin/lsp/main.cpp)
  target_link_libraries(swiftfin_lsp PRIVATE swiftfinLsp)
endif()

if(TARGET swiftfinLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/swiftfin/bench/main.cpp")
  cursiv_add_benchmark(swiftfin
      SOURCES swiftfin/bench/main.cpp)
endif()

cursiv_package_language(swiftfin)
