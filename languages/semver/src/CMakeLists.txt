# Generate the parser and its language module.
cursiv_generate_language(ID semver
    GRAMMAR    semver/core/SemverGrammar.hpp
    NAMESPACE  semver::Semver
    EXTENSIONS .semver
    OUT        semver/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/semver/core/*.cpp)
add_library(semverCore ${_core})
target_link_libraries(semverCore PUBLIC semverGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/semver/cli/*.cpp)
if(_cli)
  add_executable(semver_cli ${_cli})
  target_link_libraries(semver_cli
                        PRIVATE semverCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/semver/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/semver/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/semver/lsp/main.cpp)
  add_library(semverLsp ${_lsp})
  target_link_libraries(semverLsp PUBLIC semverCore cursiv::lsp)
  add_executable(semver_lsp semver/lsp/main.cpp)
  target_link_libraries(semver_lsp PRIVATE semverLsp)
endif()

if(TARGET semverLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/semver/bench/main.cpp")
  cursiv_add_benchmark(semver
      SOURCES semver/bench/main.cpp)
endif()

cursiv_package_language(semver)
