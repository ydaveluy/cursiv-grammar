# THE WASI CROSS BUILD, as a toolchain file (the `wasi` preset points
# here). wasi-sdk 34 is the pinned release. Cursiv throws: `-fwasm-exceptions`
# at both compile and link selects the `eh` half of its dual `eh`/`noeh` sysroot.
set(CMAKE_SYSTEM_NAME WASI)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR wasm32)

if(NOT WASI_SDK)
  set(WASI_SDK $ENV{WASI_SDK})
endif()
# The conventional spots, so a preset works with no environment at all.
if(NOT WASI_SDK)
  foreach(_wasi_candidate "$ENV{HOME}/wasi-sdk-34.0" "/opt/wasi-sdk")
    if(EXISTS "${_wasi_candidate}/bin/clang++")
      set(WASI_SDK "${_wasi_candidate}")
      break()
    endif()
  endforeach()
endif()
if(NOT WASI_SDK)
  message(FATAL_ERROR
      "set -DWASI_SDK=<wasi-sdk-34+ directory> (or install it at "
      "~/wasi-sdk-34.0 or /opt/wasi-sdk). The wasm target builds ONLY with "
      "the SDK's own clang -- gcc cannot emit wasm.")
endif()

# CARRIED INTO try_compile, which re-runs this file in a scope the cache does
# not reach: without this line CMake probes the compiler with no compiler set.
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    WASI_SDK CURSIV_WASI_THREADS CURSIV_WASI_MAX_MEMORY)

set(CMAKE_C_COMPILER   ${WASI_SDK}/bin/clang)
set(CMAKE_CXX_COMPILER ${WASI_SDK}/bin/clang++)
set(CMAKE_AR           ${WASI_SDK}/bin/llvm-ar)
set(CMAKE_RANLIB       ${WASI_SDK}/bin/llvm-ranlib)
set(CMAKE_SYSROOT      ${WASI_SDK}/share/wasi-sysroot)
# THREADS ARE A DIFFERENT TARGET, not a flag: `wasm32-wasip1-threads` has its
# own sysroot and libc++. It is THE production target (vscode/node), so it is
# the default; OFF builds the threadless `wasm32-wasip1`, inline-only.
option(CURSIV_WASI_THREADS "Build for wasm32-wasip1-threads" ON)
if(CURSIV_WASI_THREADS)
  set(_wasi_wasi_target wasm32-wasip1-threads)
else()
  set(_wasi_wasi_target wasm32-wasip1)
endif()
set(CMAKE_C_COMPILER_TARGET   ${_wasi_wasi_target})
set(CMAKE_CXX_COMPILER_TARGET ${_wasi_wasi_target})

# `-mllvm -wasm-use-legacy-eh=false` selects the EH scheme the runtime still
# accepts; the _WASI_EMULATED_* defines are what lets `<csignal>` be included
# on a target with no signals.
set(_wasi_wasi_emul "-D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_PROCESS_CLOCKS")
if(CURSIV_WASI_THREADS)
  # Shared memory is what a thread shares; without these the module keeps a
  # private memory and `thread-spawn` has nothing to hand a worker.
  string(APPEND _wasi_wasi_emul " -pthread")
  # A shared memory must declare its maximum at link time (wasm format). It is
  # an addressing cap, not a reservation: pages are committed as the heap
  # grows. Must be a multiple of 65536; 4 GiB is the wasm32 ceiling.
  set(CURSIV_WASI_MAX_MEMORY 1073741824 CACHE STRING
      "Shared-memory maximum handed to the linker (bytes)")
  set(_wasi_wasi_shared
      "-Wl,--import-memory,--shared-memory,--max-memory=${CURSIV_WASI_MAX_MEMORY}")
endif()
set(_wasi_wasi_eh "-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false")
set(CMAKE_C_FLAGS_INIT   "${_wasi_wasi_eh} ${_wasi_wasi_emul}")
set(CMAKE_CXX_FLAGS_INIT "${_wasi_wasi_eh} ${_wasi_wasi_emul}")
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-fwasm-exceptions -Wno-unused-command-line-argument -lunwind -lwasi-emulated-signal -lwasi-emulated-process-clocks ${_wasi_wasi_shared}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BEFORE)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# A CROSS BUILD STILL HAS TO RUN WHAT IT BUILDS: a wasm test binary is a file
# the host cannot start, so `ctest` needs this hook to reach the runner beside
# this file. It is what makes the wasm tests and the wasm bench runnable at all.
# The runner lives in the engine's checkout, wherever FetchContent (or
# CURSIV_SOURCE_DIR) puts it; the file is there by the time a test runs.
find_program(CURSIV_NODE node)
if(CURSIV_SOURCE_DIR)
  set(_wasi_runner ${CURSIV_SOURCE_DIR}/tools/wasi-run.mjs)
else()
  set(_wasi_runner ${CMAKE_BINARY_DIR}/_deps/cursiv-src/tools/wasi-run.mjs)
endif()
if(CURSIV_NODE)
  set(CMAKE_CROSSCOMPILING_EMULATOR
      ${CURSIV_NODE} --no-warnings
      ${_wasi_runner})
endif()
