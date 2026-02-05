# MHS (MicroHaskell) language tests
#
# Only build tests when MHS integration is enabled

if(NOT ENABLE_MHS_INTEGRATION)
    message(STATUS "MHS tests: Skipped (MHS integration disabled)")
    return()
endif()

# Helper macro for mhs tests
macro(add_mhs_test TEST_NAME)
    add_executable(test_mhs_${TEST_NAME}
        test_${TEST_NAME}.c
        ${PSND_ROOT_DIR}/source/testing/test_framework.c
    )
    target_include_directories(test_mhs_${TEST_NAME} PRIVATE
        ${PSND_ROOT_DIR}/source/testing
        ${PSND_ROOT_DIR}/source/core/include
        ${PSND_ROOT_DIR}/source/core/shared
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
    target_link_libraries(test_mhs_${TEST_NAME} PRIVATE
        mhs
        shared
        $<$<NOT:$<PLATFORM_ID:Windows>>:m>
    )
    add_test(
        NAME mhs_${TEST_NAME}_tests
        COMMAND $<TARGET_FILE:test_mhs_${TEST_NAME}>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_tests_properties(mhs_${TEST_NAME}_tests PROPERTIES LABELS "unit")
endmacro()

# Add MHS tests
add_mhs_test(midi_ffi)
add_mhs_test(context)
