-- Palenight Theme for Loki Editor
-- Based on Material Palenight
-- https://github.com/kaicataldo/material.vim
--
-- Palette:
--   bg: #292d3e           fg: #a6accd
--   red: #ff5370          orange: #f78c6c
--   yellow: #ffcb6b       green: #c3e88d
--   cyan: #89ddff         blue: #82aaff
--   purple: #c792ea       pink: #ff9cac
--   comment: #676e95

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=166, g=172, b=205},  -- #a6accd fg
        nonprint  = {r=103, g=110, b=149},  -- #676e95 comment
        comment   = {r=103, g=110, b=149},  -- #676e95 comment
        mlcomment = {r=103, g=110, b=149},  -- #676e95 comment
        keyword1  = {r=199, g=146, b=234},  -- #c792ea purple
        keyword2  = {r=137, g=221, b=255},  -- #89ddff cyan
        string    = {r=195, g=232, b=141},  -- #c3e88d green
        number    = {r=247, g=140, b=108},  -- #f78c6c orange
        match     = {r=255, g=203, b=107},  -- #ffcb6b yellow

        -- Functions
        ["function"]      = {r=130, g=170, b=255},  -- #82aaff blue
        function_builtin  = {r=137, g=221, b=255},  -- #89ddff cyan
        function_call     = {r=130, g=170, b=255},  -- #82aaff blue
        function_method   = {r=130, g=170, b=255},  -- #82aaff blue
        function_macro    = {r=137, g=221, b=255},  -- #89ddff cyan

        -- Variables
        variable          = {r=166, g=172, b=205},  -- #a6accd fg
        variable_builtin  = {r=255, g=83,  b=112},  -- #ff5370 red
        variable_parameter= {r=247, g=140, b=108},  -- #f78c6c orange
        variable_field    = {r=255, g=83,  b=112},  -- #ff5370 red
        variable_property = {r=255, g=83,  b=112},  -- #ff5370 red

        -- Keywords
        keyword_control   = {r=199, g=146, b=234},  -- #c792ea purple
        keyword_function  = {r=199, g=146, b=234},  -- #c792ea purple
        keyword_return    = {r=199, g=146, b=234},  -- #c792ea purple
        keyword_operator  = {r=137, g=221, b=255},  -- #89ddff cyan
        keyword_import    = {r=199, g=146, b=234},  -- #c792ea purple
        keyword_type      = {r=255, g=203, b=107},  -- #ffcb6b yellow
        keyword_modifier  = {r=199, g=146, b=234},  -- #c792ea purple

        -- Strings
        string_escape     = {r=137, g=221, b=255},  -- #89ddff cyan
        string_regex      = {r=255, g=83,  b=112},  -- #ff5370 red
        string_special    = {r=195, g=232, b=141},  -- #c3e88d green

        -- Numbers
        number_float      = {r=247, g=140, b=108},  -- #f78c6c orange

        -- Literals
        boolean           = {r=247, g=140, b=108},  -- #f78c6c orange
        constant          = {r=247, g=140, b=108},  -- #f78c6c orange
        constant_builtin  = {r=247, g=140, b=108},  -- #f78c6c orange

        -- Comments
        comment_doc       = {r=103, g=110, b=149},  -- #676e95 comment

        -- Types
        type              = {r=255, g=203, b=107},  -- #ffcb6b yellow
        type_builtin      = {r=255, g=203, b=107},  -- #ffcb6b yellow
        type_parameter    = {r=255, g=203, b=107},  -- #ffcb6b yellow
        type_qualifier    = {r=199, g=146, b=234},  -- #c792ea purple

        -- Operators and punctuation
        operator          = {r=137, g=221, b=255},  -- #89ddff cyan
        punctuation       = {r=166, g=172, b=205},  -- #a6accd fg
        punctuation_bracket   = {r=166, g=172, b=205},  -- #a6accd fg
        punctuation_delimiter = {r=166, g=172, b=205},  -- #a6accd fg

        -- Special
        constructor       = {r=255, g=203, b=107},  -- #ffcb6b yellow
        namespace         = {r=255, g=203, b=107},  -- #ffcb6b yellow
        module            = {r=255, g=203, b=107},  -- #ffcb6b yellow
        label             = {r=255, g=156, b=172},  -- #ff9cac pink
        tag               = {r=255, g=83,  b=112},  -- #ff5370 red
        tag_attribute     = {r=199, g=146, b=234},  -- #c792ea purple
        preprocessor      = {r=137, g=221, b=255},  -- #89ddff cyan

        -- Errors and warnings
        error             = {r=255, g=83,  b=112},  -- #ff5370 red
        warning           = {r=255, g=203, b=107},  -- #ffcb6b yellow
    })

    if loki.status then
        loki.status("Palenight theme loaded")
    end
end
