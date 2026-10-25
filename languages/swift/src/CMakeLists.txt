# Generate the parser and its language module.
cursiv_generate_language(ID swift
    GRAMMAR    swift/core/SwiftGrammar.hpp
    NAMESPACE  swift::Swift
    EXTENSIONS .swift
    OUT        swift/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/swift/core/*.cpp)
add_library(swiftCore ${_core})
target_link_libraries(swiftCore PUBLIC swiftGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/swift/cli/*.cpp)
if(_cli)
  add_executable(swift_cli ${_cli})
  target_link_libraries(swift_cli
                        PRIVATE swiftCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/swift/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/swift/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/swift/lsp/main.cpp)
  add_library(swiftLsp ${_lsp})
  target_link_libraries(swiftLsp PUBLIC swiftCore cursiv::lsp)
  add_executable(swift_lsp swift/lsp/main.cpp)
  target_link_libraries(swift_lsp PRIVATE swiftLsp)
endif()

if(TARGET swiftLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/swift/bench/main.cpp")
  cursiv_add_benchmark(swift
      SOURCES swift/bench/main.cpp)
endif()

cursiv_package_language(swift)
