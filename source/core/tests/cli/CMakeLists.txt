# CLI integration tests

# Play command integration tests
add_executable(test_play_command
    test_play_command.c
    ${PSND_ROOT_DIR}/source/testing/test_framework.c
)

target_include_directories(test_play_command PRIVATE
    ${PSND_ROOT_DIR}/source/testing
)

# Pass the path to the psnd binary
target_compile_definitions(test_play_command PRIVATE
    PSND_BINARY="$<TARGET_FILE:psnd_bin>"
)

# Link only what we need (the test runs psnd as a subprocess)
target_link_libraries(test_play_command PRIVATE test_framework)

# This test depends on psnd being built
add_dependencies(test_play_command psnd_bin)

add_test(
    NAME cli_play_command_tests
    COMMAND $<TARGET_FILE:test_play_command>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(cli_play_command_tests PROPERTIES LABELS "integration")

# ============================================================================
# Language dispatch unit tests
# ============================================================================

add_executable(test_lang_dispatch
    test_lang_dispatch.c
    ${PSND_ROOT_DIR}/source/core/lang_dispatch.c
    ${PSND_ROOT_DIR}/source/testing/test_framework.c
)

target_include_directories(test_lang_dispatch PRIVATE
    ${PSND_ROOT_DIR}/source/testing
    ${PSND_ROOT_DIR}/source/core
    ${CMAKE_BINARY_DIR}/generated
)

# Only link test framework - lang_dispatch is compiled directly
target_link_libraries(test_lang_dispatch PRIVATE test_framework)

add_test(
    NAME cli_lang_dispatch_tests
    COMMAND $<TARGET_FILE:test_lang_dispatch>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(cli_lang_dispatch_tests PROPERTIES LABELS "unit")

# ============================================================================
# Main dispatch integration tests
# ============================================================================

add_executable(test_main_dispatch
    test_main_dispatch.c
    ${PSND_ROOT_DIR}/source/testing/test_framework.c
)

target_include_directories(test_main_dispatch PRIVATE
    ${PSND_ROOT_DIR}/source/testing
)

# Pass the path to the psnd binary
target_compile_definitions(test_main_dispatch PRIVATE
    PSND_BINARY="$<TARGET_FILE:psnd_bin>"
)

target_link_libraries(test_main_dispatch PRIVATE test_framework)

# This test depends on psnd being built
add_dependencies(test_main_dispatch psnd_bin)

add_test(
    NAME cli_main_dispatch_tests
    COMMAND $<TARGET_FILE:test_main_dispatch>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
# Runs psnd ~30 times, one of them through the embedded MicroHs runtime. Warm,
# that is 4-9s across the three CI platforms; against a cold .mhscache the MHS
# case alone compiles the base packages and reaches ~90s. The ceiling covers the
# cold path, which the 120s default the CI workflows pass would not.
set_tests_properties(cli_main_dispatch_tests PROPERTIES LABELS "integration" TIMEOUT 300)
