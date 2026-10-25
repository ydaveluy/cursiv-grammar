# Generate the parser and its language module.
cursiv_generate_language(ID csv
    GRAMMAR    csv/core/CsvGrammar.hpp
    NAMESPACE  csv::Csv
    EXTENSIONS .csv
    OUT        csv/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/csv/core/*.cpp)
add_library(csvCore ${_core})
target_link_libraries(csvCore PUBLIC csvGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/csv/cli/*.cpp)
if(_cli)
  add_executable(csv_cli ${_cli})
  target_link_libraries(csv_cli
                        PRIVATE csvCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/csv/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/csv/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/csv/lsp/main.cpp)
  add_library(csvLsp ${_lsp})
  target_link_libraries(csvLsp PUBLIC csvCore cursiv::lsp)
  add_executable(csv_lsp csv/lsp/main.cpp)
  target_link_libraries(csv_lsp PRIVATE csvLsp)
endif()

if(TARGET csvLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/csv/bench/main.cpp")
  cursiv_add_benchmark(csv
      SOURCES csv/bench/main.cpp)
endif()

cursiv_package_language(csv)
