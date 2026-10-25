# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `swiftGenerated` (the compiled library) and `swift_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID swift
    GRAMMAR    swift/core/SwiftGrammar.hpp
    NAMESPACE  swift::Swift
    EXTENSIONS .swift
    OUT        swift/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/swift/core/*.cpp)
add_library(swiftCore ${_core})
target_link_libraries(swiftCore PUBLIC swiftGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/swift/cli/*.cpp)
add_executable(swift_cli ${_cli})
target_link_libraries(swift_cli
                      PRIVATE swiftCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/swift/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/swift/lsp/main.cpp)
add_library(swiftLsp ${_lsp})
target_link_libraries(swiftLsp PUBLIC swiftCore cursiv::lsp)
add_executable(swift_lsp swift/lsp/main.cpp)
target_link_libraries(swift_lsp PRIVATE swiftLsp)

cursiv_add_benchmark(swift SOURCES swift/bench/main.cpp)

cursiv_package_language(swift)
