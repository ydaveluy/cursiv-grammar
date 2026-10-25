cursiv_generate_language(ID         ini
    GRAMMAR    ini/core/IniGrammar.hpp
    NAMESPACE  ini::INI
    EXTENSIONS .ini
    OUT        ini/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ini/core/*.cpp)
add_library(iniCore ${_core})
target_link_libraries(iniCore PUBLIC iniGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ini/cli/*.cpp)
add_executable(ini_cli ${_cli})
target_link_libraries(ini_cli PRIVATE iniCore cursiv::cli)

file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ini/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ini/lsp/main.cpp)
add_library(iniLsp ${_lsp})
target_link_libraries(iniLsp PUBLIC iniCore cursiv::lsp)
add_executable(ini_lsp ini/lsp/main.cpp)
target_link_libraries(ini_lsp PRIVATE iniLsp)

cursiv_add_benchmark(ini SOURCES ini/bench/main.cpp)

cursiv_package_language(ini)
