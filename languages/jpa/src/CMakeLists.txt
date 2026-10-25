# Generate the parser and its language module.
cursiv_generate_language(ID jpa
    GRAMMAR    jpa/core/JpaGrammar.hpp
    NAMESPACE  jpa::Jpa
    EXTENSIONS .jpql
    OUT        jpa/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/jpa/core/*.cpp)
add_library(jpaCore ${_core})
target_link_libraries(jpaCore PUBLIC jpaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/jpa/cli/*.cpp)
if(_cli)
  add_executable(jpa_cli ${_cli})
  target_link_libraries(jpa_cli
                        PRIVATE jpaCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/jpa/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/jpa/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/jpa/lsp/main.cpp)
  add_library(jpaLsp ${_lsp})
  target_link_libraries(jpaLsp PUBLIC jpaCore cursiv::lsp)
  add_executable(jpa_lsp jpa/lsp/main.cpp)
  target_link_libraries(jpa_lsp PRIVATE jpaLsp)
endif()

if(TARGET jpaLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/jpa/bench/main.cpp")
  cursiv_add_benchmark(jpa
      SOURCES jpa/bench/main.cpp)
endif()

cursiv_package_language(jpa)
