# Generate the parser and its language module.
cursiv_generate_language(ID kuka
    GRAMMAR    kuka/core/KukaGrammar.hpp
    NAMESPACE  kuka::Kuka
    EXTENSIONS .kuka
    OUT        kuka/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kuka/core/*.cpp)
add_library(kukaCore ${_core})
target_link_libraries(kukaCore PUBLIC kukaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kuka/cli/*.cpp)
if(_cli)
  add_executable(kuka_cli ${_cli})
  target_link_libraries(kuka_cli
                        PRIVATE kukaCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/kuka/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/kuka/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/kuka/lsp/main.cpp)
  add_library(kukaLsp ${_lsp})
  target_link_libraries(kukaLsp PUBLIC kukaCore cursiv::lsp)
  add_executable(kuka_lsp kuka/lsp/main.cpp)
  target_link_libraries(kuka_lsp PRIVATE kukaLsp)
endif()

if(TARGET kukaLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/kuka/bench/main.cpp")
  cursiv_add_benchmark(kuka
      SOURCES kuka/bench/main.cpp)
endif()

cursiv_package_language(kuka)
