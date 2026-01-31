# source/testing/CMakeLists.txt - Test framework library

add_library(test_framework STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_framework.c
)

target_include_directories(test_framework PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PSND_ROOT_DIR}/source/core/include
)

# Memory check self-tests
if(BUILD_TESTING)
    add_executable(test_memcheck_selftest
        ${CMAKE_CURRENT_SOURCE_DIR}/test_memcheck_selftest.c
        ${CMAKE_CURRENT_SOURCE_DIR}/test_framework.c
    )

    target_include_directories(test_memcheck_selftest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_test(NAME memcheck_selftest COMMAND test_memcheck_selftest)
    set_tests_properties(memcheck_selftest PROPERTIES LABELS "unit")
endif()
