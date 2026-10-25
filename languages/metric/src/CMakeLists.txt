# Generate the parser and its language module.
cursiv_generate_language(ID metric
    GRAMMAR    metric/core/MetricGrammar.hpp
    NAMESPACE  metric::Metric
    EXTENSIONS .metric
    OUT        metric/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/metric/core/*.cpp)
add_library(metricCore ${_core})
target_link_libraries(metricCore PUBLIC metricGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/metric/cli/*.cpp)
if(_cli)
  add_executable(metric_cli ${_cli})
  target_link_libraries(metric_cli
                        PRIVATE metricCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/metric/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/metric/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/metric/lsp/main.cpp)
  add_library(metricLsp ${_lsp})
  target_link_libraries(metricLsp PUBLIC metricCore cursiv::lsp)
  add_executable(metric_lsp metric/lsp/main.cpp)
  target_link_libraries(metric_lsp PRIVATE metricLsp)
endif()

if(TARGET metricLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/metric/bench/main.cpp")
  cursiv_add_benchmark(metric
      SOURCES metric/bench/main.cpp)
endif()

cursiv_package_language(metric)
