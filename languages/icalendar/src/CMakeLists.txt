# Generate the parser and its language module.
cursiv_generate_language(ID icalendar
    GRAMMAR    icalendar/core/IcalendarGrammar.hpp
    NAMESPACE  icalendar::Icalendar
    EXTENSIONS .ics
    OUT        icalendar/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/icalendar/core/*.cpp)
add_library(icalendarCore ${_core})
target_link_libraries(icalendarCore PUBLIC icalendarGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/icalendar/cli/*.cpp)
if(_cli)
  add_executable(icalendar_cli ${_cli})
  target_link_libraries(icalendar_cli
                        PRIVATE icalendarCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/icalendar/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/icalendar/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/icalendar/lsp/main.cpp)
  add_library(icalendarLsp ${_lsp})
  target_link_libraries(icalendarLsp PUBLIC icalendarCore cursiv::lsp)
  add_executable(icalendar_lsp icalendar/lsp/main.cpp)
  target_link_libraries(icalendar_lsp PRIVATE icalendarLsp)
endif()

if(TARGET icalendarLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/icalendar/bench/main.cpp")
  cursiv_add_benchmark(icalendar
      SOURCES icalendar/bench/main.cpp)
endif()

cursiv_package_language(icalendar)
