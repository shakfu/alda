# tomlc99 - TOML parser library
# MIT License - https://github.com/cktan/tomlc99

add_library(tomlc99 STATIC toml.c)

target_include_directories(tomlc99 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Suppress warnings in third-party code
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tomlc99 PRIVATE -w)
endif()
