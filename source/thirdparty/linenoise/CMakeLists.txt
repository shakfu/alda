# Linenoise with tree-sitter syntax highlighting
# Includes grammars for: Lua, Haskell, Joy, Scheme, Alda, Csound

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
# Tree-sitter core library (located at thirdparty/tree-sitter)
# ==============================================================================
set(TREESITTER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tree-sitter)

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
# Tree-sitter grammars (located at thirdparty/tree-sitter-grammars/)
# ==============================================================================
set(TREESITTER_GRAMMARS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tree-sitter-grammars)

# Lua grammar
add_library(tree-sitter-lua STATIC
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-lua/src/parser.c
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-lua/src/scanner.c
)
target_include_directories(tree-sitter-lua PRIVATE
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-lua/src
    ${TREESITTER_DIR}/lib/include
)

# Haskell grammar
add_library(tree-sitter-haskell STATIC
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-haskell/src/parser.c
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-haskell/src/scanner.c
)
target_include_directories(tree-sitter-haskell PRIVATE
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-haskell/src
    ${TREESITTER_DIR}/lib/include
)

# Joy grammar
add_library(tree-sitter-joy STATIC
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-joy/src/parser.c
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-joy/src/scanner.c
)
target_include_directories(tree-sitter-joy PRIVATE
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-joy/src
    ${TREESITTER_DIR}/lib/include
)

# Scheme grammar
add_library(tree-sitter-scheme STATIC
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-scheme/src/parser.c
)
target_include_directories(tree-sitter-scheme PRIVATE
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-scheme/src
    ${TREESITTER_DIR}/lib/include
)

# Alda grammar
add_library(tree-sitter-alda STATIC
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-alda/src/parser.c
)
target_include_directories(tree-sitter-alda PRIVATE
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-alda/src
    ${TREESITTER_DIR}/lib/include
)

# Csound grammar
add_library(tree-sitter-csound STATIC
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-csound/src/parser.c
)
target_include_directories(tree-sitter-csound PRIVATE
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-csound/src
    ${TREESITTER_DIR}/lib/include
)

# Prolog grammar (used for Bog language)
add_library(tree-sitter-prolog STATIC
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-prolog/grammars/prolog/src/parser.c
)
target_include_directories(tree-sitter-prolog PRIVATE
    ${TREESITTER_GRAMMARS_DIR}/tree-sitter-prolog/grammars/prolog/src
    ${TREESITTER_DIR}/lib/include
)

# Silence warnings for all grammars
if(MSVC)
    target_compile_options(tree-sitter-lua PRIVATE /w)
    target_compile_options(tree-sitter-haskell PRIVATE /w)
    target_compile_options(tree-sitter-joy PRIVATE /w)
    target_compile_options(tree-sitter-scheme PRIVATE /w)
    target_compile_options(tree-sitter-alda PRIVATE /w)
    target_compile_options(tree-sitter-csound PRIVATE /w)
    target_compile_options(tree-sitter-prolog PRIVATE /w)
else()
    target_compile_options(tree-sitter-lua PRIVATE -w)
    target_compile_options(tree-sitter-haskell PRIVATE -w)
    target_compile_options(tree-sitter-joy PRIVATE -w)
    target_compile_options(tree-sitter-scheme PRIVATE -w)
    target_compile_options(tree-sitter-alda PRIVATE -w)
    target_compile_options(tree-sitter-csound PRIVATE -w)
    target_compile_options(tree-sitter-prolog PRIVATE -w)
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

# ==============================================================================
# Alda highlighter module
# ==============================================================================
add_library(linenoise-highlight-alda STATIC
    src/syntax/alda.c
)
target_include_directories(linenoise-highlight-alda
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/internal
        ${TREESITTER_DIR}/lib/include
)
target_link_libraries(linenoise-highlight-alda
    PRIVATE linenoise-theme tree-sitter tree-sitter-alda
)

# ==============================================================================
# Csound highlighter module
# ==============================================================================
add_library(linenoise-highlight-csound STATIC
    src/syntax/csound.c
)
target_include_directories(linenoise-highlight-csound
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/internal
        ${TREESITTER_DIR}/lib/include
)
target_link_libraries(linenoise-highlight-csound
    PRIVATE linenoise-theme tree-sitter tree-sitter-csound
)

# ==============================================================================
# Joy highlighter module
# ==============================================================================
add_library(linenoise-highlight-joy STATIC
    src/syntax/joy.c
)
target_include_directories(linenoise-highlight-joy
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/internal
        ${TREESITTER_DIR}/lib/include
)
target_link_libraries(linenoise-highlight-joy
    PRIVATE linenoise-theme tree-sitter tree-sitter-joy
)

# ==============================================================================
# Haskell highlighter module
# ==============================================================================
add_library(linenoise-highlight-haskell STATIC
    src/syntax/haskell.c
)
target_include_directories(linenoise-highlight-haskell
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/internal
        ${TREESITTER_DIR}/lib/include
)
target_link_libraries(linenoise-highlight-haskell
    PRIVATE linenoise-theme tree-sitter tree-sitter-haskell
)

# ==============================================================================
# Scheme highlighter module
# ==============================================================================
add_library(linenoise-highlight-scheme STATIC
    src/syntax/scheme.c
)
target_include_directories(linenoise-highlight-scheme
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/internal
        ${TREESITTER_DIR}/lib/include
)
target_link_libraries(linenoise-highlight-scheme
    PRIVATE linenoise-theme tree-sitter tree-sitter-scheme
)

message(STATUS "linenoise: tree-sitter syntax highlighting enabled (Lua, Alda, Csound, Joy, Haskell, Scheme, Prolog)")
