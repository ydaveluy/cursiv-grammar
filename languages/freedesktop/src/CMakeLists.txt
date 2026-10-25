# Generate the parser and its language module.
cursiv_generate_language(ID desktopentry
    GRAMMAR    desktopentry/core/DesktopEntryGrammar.hpp
    NAMESPACE  desktopentry::DesktopEntry
    EXTENSIONS .desktop
    OUT        desktopentry/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/desktopentry/core/*.cpp)
add_library(desktopentryCore ${_core})
target_link_libraries(desktopentryCore PUBLIC desktopentryGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/desktopentry/cli/*.cpp)
if(_cli)
  add_executable(desktopentry_cli ${_cli})
  target_link_libraries(desktopentry_cli
                        PRIVATE desktopentryCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/desktopentry/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/desktopentry/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/desktopentry/lsp/main.cpp)
  add_library(desktopentryLsp ${_lsp})
  target_link_libraries(desktopentryLsp PUBLIC desktopentryCore cursiv::lsp)
  add_executable(desktopentry_lsp desktopentry/lsp/main.cpp)
  target_link_libraries(desktopentry_lsp PRIVATE desktopentryLsp)
endif()

if(TARGET desktopentryLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/desktopentry/bench/main.cpp")
  cursiv_add_benchmark(desktopentry
      SOURCES desktopentry/bench/main.cpp)
endif()

cursiv_package_language(desktopentry)
