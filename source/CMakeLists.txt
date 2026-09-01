# source/CMakeLists.txt - Main source tree entry point

# Build thirdparty dependencies first
add_subdirectory(thirdparty)

# Build test framework library
add_subdirectory(testing)

# Build optional plugins
add_subdirectory(plugins)

# Build core libraries (shared, loki)
add_subdirectory(core)

# Discover and build language modules
add_subdirectory(langs)

# ENABLE_MHS_INTEGRATION is set by source/langs/mhs, which is configured after
# core and therefore after the test that needs it. Apply the define here rather
# than restating the platform rule in the test's own CMakeLists, where it would
# drift.
if(TARGET test_main_dispatch AND ENABLE_MHS_INTEGRATION)
    target_compile_definitions(test_main_dispatch PRIVATE PSND_MHS_ENABLED=1)
endif()
