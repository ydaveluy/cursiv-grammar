cursiv_generate_language(ID         xsmp
    GRAMMAR    xsmp/core/XsmpGrammar.hpp
    NAMESPACE  xsmp::Xsmp
    EXTENSIONS .xsmpcat
    OUT        xsmp/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xsmp/core/*.cpp)
add_library(xsmpCore ${_core})
target_link_libraries(xsmpCore PUBLIC xsmpGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xsmp/cli/*.cpp)
add_executable(xsmp_cli ${_cli})
target_link_libraries(xsmp_cli PRIVATE xsmpCore cursiv::cli)

file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xsmp/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/xsmp/lsp/main.cpp)
add_library(xsmpLsp ${_lsp})
target_link_libraries(xsmpLsp PUBLIC xsmpCore cursiv::lsp)
add_executable(xsmp_lsp xsmp/lsp/main.cpp)
target_link_libraries(xsmp_lsp PRIVATE xsmpLsp)

cursiv_add_benchmark(xsmp SOURCES xsmp/bench/main.cpp)

cursiv_package_language(xsmp)
