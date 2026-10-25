# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `metricGenerated` (the compiled library) and `metric_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID metric
    GRAMMAR    metric/core/MetricGrammar.hpp
    NAMESPACE  metric::Metric
    EXTENSIONS .metric
    OUT        metric/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/metric/core/*.cpp)
add_library(metricCore ${_core})
target_link_libraries(metricCore PUBLIC metricGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/metric/cli/*.cpp)
add_executable(metric_cli ${_cli})
target_link_libraries(metric_cli
                      PRIVATE metricCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/metric/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/metric/lsp/main.cpp)
add_library(metricLsp ${_lsp})
target_link_libraries(metricLsp PUBLIC metricCore cursiv::lsp)
add_executable(metric_lsp metric/lsp/main.cpp)
target_link_libraries(metric_lsp PRIVATE metricLsp)

cursiv_add_benchmark(metric SOURCES metric/bench/main.cpp)

cursiv_package_language(metric)
