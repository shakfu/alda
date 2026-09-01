# End-to-end smoke test for the embedded MicroHs runtime.
#
# Runs a Haskell module through the psnd binary and checks its output. This is
# the only test that exercises the whole MHS path: VFS init, the embedded base
# and music packages, compilation, and evaluation. Everything else in this
# directory tests C entry points without starting the interpreter.
#
# Invoked by CTest with -DPSND=<binary> -DFIXTURE_DIR=<dir>.

if(NOT PSND OR NOT FIXTURE_DIR)
    message(FATAL_ERROR "smoke_test.cmake requires -DPSND and -DFIXTURE_DIR")
endif()

set(EXPECTED "psnd-mhs-smoke 55 60 500")

# The form mhs documents. It reaches the interpreter only because main.c
# delegates the whole command line to the language; test_main_dispatch.c guards
# that contract directly.
execute_process(
    COMMAND ${PSND} mhs -r ${FIXTURE_DIR}/Smoke.hs
    RESULT_VARIABLE result
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
    TIMEOUT 120
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "psnd mhs exited with ${result}\nstdout:\n${out}\nstderr:\n${err}")
endif()

string(STRIP "${out}" out_stripped)
if(NOT out_stripped MATCHES "${EXPECTED}")
    message(FATAL_ERROR
        "expected output containing '${EXPECTED}'\ngot:\n${out}\nstderr:\n${err}")
endif()

message(STATUS "MHS smoke test passed: ${out_stripped}")
