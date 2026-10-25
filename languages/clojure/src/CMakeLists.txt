# Generate the parser and its language module.
cursiv_generate_language(ID clojure
    GRAMMAR    clojure/core/ClojureGrammar.hpp
    NAMESPACE  clojure::Clojure
    EXTENSIONS .clj
    OUT        clojure/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clojure/core/*.cpp)
add_library(clojureCore ${_core})
target_link_libraries(clojureCore PUBLIC clojureGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clojure/cli/*.cpp)
if(_cli)
  add_executable(clojure_cli ${_cli})
  target_link_libraries(clojure_cli
                        PRIVATE clojureCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/clojure/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/clojure/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/clojure/lsp/main.cpp)
  add_library(clojureLsp ${_lsp})
  target_link_libraries(clojureLsp PUBLIC clojureCore cursiv::lsp)
  add_executable(clojure_lsp clojure/lsp/main.cpp)
  target_link_libraries(clojure_lsp PRIVATE clojureLsp)
endif()

if(TARGET clojureLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/clojure/bench/main.cpp")
  cursiv_add_benchmark(clojure
      SOURCES clojure/bench/main.cpp)
endif()

cursiv_package_language(clojure)
