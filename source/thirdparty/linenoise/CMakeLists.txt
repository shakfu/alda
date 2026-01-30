# Linenoise with tree-sitter syntax highlighting (Lua only)
# Stripped-down version for psnd with only Lua grammar support

cmake_minimum_required(VERSION 3.10)

# C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform-specific terminal implementation
if(WIN32)
    set(TERMINAL_SRC src/internal/terminal_windows.c)
else()
    set(TERMINAL_SRC src/internal/terminal_posix.c)
endif()

# Module source files
set(MODULE_SOURCES
    src/internal/utf8.c
    src/internal/history.c
    src/internal/completion.c
    src/internal/keyparser.c
    src/internal/render.c
)

# Core library source files
set(LINENOISE_SOURCES
    src/linenoise.c
    ${MODULE_SOURCES}
    ${TERMINAL_SRC}
)

# Linenoise library (static)
add_library(linenoise STATIC ${LINENOISE_SOURCES})

target_include_directories(linenoise
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/internal
)

# Silence warnings in linenoise
if(NOT MSVC)
    target_compile_options(linenoise PRIVATE -Wno-unused-parameter)
endif()

# ==============================================================================
# Tree-sitter core library
# ==============================================================================
set(TREESITTER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tree-sitter)

add_library(tree-sitter STATIC
    ${TREESITTER_DIR}/lib/src/lib.c
)
target_include_directories(tree-sitter
    PUBLIC
        ${TREESITTER_DIR}/lib/include
    PRIVATE
        ${TREESITTER_DIR}/lib/src
)
if(MSVC)
    target_compile_options(tree-sitter PRIVATE /w)
else()
    target_compile_options(tree-sitter PRIVATE -w)
endif()

# ==============================================================================
# Tree-sitter Lua grammar
# ==============================================================================
set(TREESITTER_LUA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tree-sitter-lua)

add_library(tree-sitter-lua STATIC
    ${TREESITTER_LUA_DIR}/src/parser.c
    ${TREESITTER_LUA_DIR}/src/scanner.c
)
target_include_directories(tree-sitter-lua
    PRIVATE
        ${TREESITTER_LUA_DIR}/src
        ${TREESITTER_DIR}/lib/include
)
if(MSVC)
    target_compile_options(tree-sitter-lua PRIVATE /w)
else()
    target_compile_options(tree-sitter-lua PRIVATE -w)
endif()

# ==============================================================================
# Syntax highlighting theme library
# ==============================================================================
add_library(linenoise-theme STATIC
    src/syntax/theme.c
)
target_include_directories(linenoise-theme
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ==============================================================================
# Lua highlighter module
# ==============================================================================
add_library(linenoise-highlight-lua STATIC
    src/syntax/lua.c
)
target_include_directories(linenoise-highlight-lua
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/internal
        ${TREESITTER_DIR}/lib/include
)
target_link_libraries(linenoise-highlight-lua
    PRIVATE linenoise-theme tree-sitter tree-sitter-lua
)

message(STATUS "linenoise: tree-sitter Lua syntax highlighting enabled")
