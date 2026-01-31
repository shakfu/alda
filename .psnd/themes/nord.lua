-- Nord Theme for Loki Editor
-- Based on https://www.nordtheme.com
--
-- This theme demonstrates all 51 highlight types available.
-- Colors use the Nord palette:
--   Polar Night: #2e3440, #3b4252, #434c5e, #4c566a
--   Snow Storm:  #d8dee9, #e5e9f0, #eceff4
--   Frost:       #8fbcbb, #88c0d0, #81a1c1, #5e81ac
--   Aurora:      #bf616a (red), #d08770 (orange), #ebcb8b (yellow),
--                #a3be8c (green), #b48ead (purple)

return function()
    loki.set_theme({
        -- =====================================================================
        -- Base types (0-8)
        -- =====================================================================
        normal    = {r=216, g=222, b=233},  -- #d8dee9 Snow Storm
        nonprint  = {r=76,  g=86,  b=106},  -- #4c566a Polar Night (dimmed)
        comment   = {r=136, g=192, b=208},  -- #88c0d0 Frost (cyan)
        mlcomment = {r=136, g=192, b=208},  -- #88c0d0 Frost (cyan)
        keyword1  = {r=129, g=161, b=193},  -- #81a1c1 Frost (blue)
        keyword2  = {r=143, g=188, b=187},  -- #8fbcbb Frost (teal)
        string    = {r=163, g=190, b=140},  -- #a3be8c Aurora (green)
        number    = {r=180, g=142, b=173},  -- #b48ead Aurora (purple)
        match     = {r=235, g=203, b=139},  -- #ebcb8b Aurora (yellow)

        -- =====================================================================
        -- Extended types (9-23) - Tree-sitter vocabulary
        -- =====================================================================

        -- Functions
        ["function"]      = {r=136, g=192, b=208},  -- #88c0d0 Frost (definitions)
        function_builtin  = {r=94,  g=129, b=172},  -- #5e81ac Frost (builtins)
        function_call     = {r=136, g=192, b=208},  -- #88c0d0 Frost (calls)

        -- Variables
        variable_builtin  = {r=208, g=135, b=112},  -- #d08770 Aurora orange (self/this)
        variable_parameter= {r=216, g=222, b=233},  -- #d8dee9 Snow Storm

        -- Operators and punctuation
        operator          = {r=129, g=161, b=193},  -- #81a1c1 Frost (blue)
        punctuation       = {r=216, g=222, b=233},  -- #d8dee9 Snow Storm

        -- Special types
        constructor       = {r=143, g=188, b=187},  -- #8fbcbb Frost (teal)
        namespace         = {r=129, g=161, b=193},  -- #81a1c1 Frost (blue)
        label             = {r=235, g=203, b=139},  -- #ebcb8b Aurora (yellow)
        tag               = {r=129, g=161, b=193},  -- #81a1c1 Frost (blue)

        -- Keyword subtypes
        keyword_control   = {r=129, g=161, b=193},  -- #81a1c1 Frost (if/else/for)
        keyword_function  = {r=129, g=161, b=193},  -- #81a1c1 Frost (function/def)
        keyword_return    = {r=129, g=161, b=193},  -- #81a1c1 Frost (return/yield)

        -- Constants
        constant_builtin  = {r=94,  g=129, b=172},  -- #5e81ac Frost (nil/null)

        -- =====================================================================
        -- Additional extended types (24-50) - Full tree-sitter vocabulary
        -- =====================================================================

        -- Variables (extended)
        variable          = {r=216, g=222, b=233},  -- #d8dee9 Snow Storm
        variable_field    = {r=143, g=188, b=187},  -- #8fbcbb Frost (struct fields)
        variable_property = {r=143, g=188, b=187},  -- #8fbcbb Frost (properties)

        -- Additional keyword subtypes
        keyword_operator  = {r=129, g=161, b=193},  -- #81a1c1 Frost (and/or/not)
        keyword_import    = {r=129, g=161, b=193},  -- #81a1c1 Frost (import/require)
        keyword_type      = {r=129, g=161, b=193},  -- #81a1c1 Frost (type/class)
        keyword_modifier  = {r=129, g=161, b=193},  -- #81a1c1 Frost (public/private)

        -- String subtypes
        string_escape     = {r=235, g=203, b=139},  -- #ebcb8b Aurora yellow (\n, \t)
        string_regex      = {r=208, g=135, b=112},  -- #d08770 Aurora orange
        string_special    = {r=163, g=190, b=140},  -- #a3be8c Aurora green

        -- Number subtypes
        number_float      = {r=180, g=142, b=173},  -- #b48ead Aurora purple

        -- Literals
        boolean           = {r=94,  g=129, b=172},  -- #5e81ac Frost (true/false)
        constant          = {r=143, g=188, b=187},  -- #8fbcbb Frost (teal)

        -- Comment subtypes
        comment_doc       = {r=136, g=192, b=208},  -- #88c0d0 Frost (doc comments)

        -- Function subtypes
        function_method   = {r=136, g=192, b=208},  -- #88c0d0 Frost (methods)
        function_macro    = {r=180, g=142, b=173},  -- #b48ead Aurora purple (macros)

        -- Types
        type              = {r=143, g=188, b=187},  -- #8fbcbb Frost (teal)
        type_builtin      = {r=143, g=188, b=187},  -- #8fbcbb Frost (int, string)
        type_parameter    = {r=143, g=188, b=187},  -- #8fbcbb Frost (generics)
        type_qualifier    = {r=129, g=161, b=193},  -- #81a1c1 Frost (const/volatile)

        -- Punctuation subtypes
        punctuation_bracket   = {r=216, g=222, b=233},  -- #d8dee9 Snow Storm
        punctuation_delimiter = {r=216, g=222, b=233},  -- #d8dee9 Snow Storm

        -- Additional special types
        module            = {r=143, g=188, b=187},  -- #8fbcbb Frost (teal)
        tag_attribute     = {r=143, g=188, b=187},  -- #8fbcbb Frost (HTML attrs)
        preprocessor      = {r=180, g=142, b=173},  -- #b48ead Aurora purple (#define)

        -- Errors and warnings
        error             = {r=191, g=97,  b=106},  -- #bf616a Aurora red
        warning           = {r=235, g=203, b=139},  -- #ebcb8b Aurora yellow
    })

    if loki.status then
        loki.status("Nord theme loaded (51 highlight types)")
    end
end
