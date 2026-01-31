-- Ayu Dark Theme for Loki Editor
-- Based on https://github.com/ayu-theme/ayu-colors
--
-- Palette:
--   bg: #0d1017           fg: #bfbdb6
--   accent: #e6b450       orange: #ff8f40
--   red: #f07178          green: #aad94c
--   cyan: #73d0ff         blue: #59c2ff
--   purple: #d2a6ff       gray: #626a73

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=191, g=189, b=182},  -- #bfbdb6 fg
        nonprint  = {r=98,  g=106, b=115},  -- #626a73 gray
        comment   = {r=98,  g=106, b=115},  -- #626a73 gray
        mlcomment = {r=98,  g=106, b=115},  -- #626a73 gray
        keyword1  = {r=255, g=143, b=64},   -- #ff8f40 orange (accent)
        keyword2  = {r=115, g=208, b=255},  -- #73d0ff cyan
        string    = {r=170, g=217, b=76},   -- #aad94c green
        number    = {r=210, g=166, b=255},  -- #d2a6ff purple
        match     = {r=230, g=180, b=80},   -- #e6b450 accent

        -- Functions
        ["function"]      = {r=230, g=180, b=80},   -- #e6b450 yellow/accent
        function_builtin  = {r=115, g=208, b=255},  -- #73d0ff cyan
        function_call     = {r=230, g=180, b=80},   -- #e6b450 yellow
        function_method   = {r=230, g=180, b=80},   -- #e6b450 yellow
        function_macro    = {r=255, g=143, b=64},   -- #ff8f40 orange

        -- Variables
        variable          = {r=191, g=189, b=182},  -- #bfbdb6 fg
        variable_builtin  = {r=240, g=113, b=120},  -- #f07178 red
        variable_parameter= {r=89,  g=194, b=255},  -- #59c2ff blue
        variable_field    = {r=191, g=189, b=182},  -- #bfbdb6 fg
        variable_property = {r=191, g=189, b=182},  -- #bfbdb6 fg

        -- Keywords
        keyword_control   = {r=255, g=143, b=64},   -- #ff8f40 orange
        keyword_function  = {r=255, g=143, b=64},   -- #ff8f40 orange
        keyword_return    = {r=255, g=143, b=64},   -- #ff8f40 orange
        keyword_operator  = {r=255, g=143, b=64},   -- #ff8f40 orange
        keyword_import    = {r=255, g=143, b=64},   -- #ff8f40 orange
        keyword_type      = {r=115, g=208, b=255},  -- #73d0ff cyan
        keyword_modifier  = {r=255, g=143, b=64},   -- #ff8f40 orange

        -- Strings
        string_escape     = {r=115, g=208, b=255},  -- #73d0ff cyan
        string_regex      = {r=170, g=217, b=76},   -- #aad94c green
        string_special    = {r=170, g=217, b=76},   -- #aad94c green

        -- Numbers
        number_float      = {r=210, g=166, b=255},  -- #d2a6ff purple

        -- Literals
        boolean           = {r=255, g=143, b=64},   -- #ff8f40 orange
        constant          = {r=210, g=166, b=255},  -- #d2a6ff purple
        constant_builtin  = {r=255, g=143, b=64},   -- #ff8f40 orange

        -- Comments
        comment_doc       = {r=98,  g=106, b=115},  -- #626a73 gray

        -- Types
        type              = {r=115, g=208, b=255},  -- #73d0ff cyan
        type_builtin      = {r=115, g=208, b=255},  -- #73d0ff cyan
        type_parameter    = {r=115, g=208, b=255},  -- #73d0ff cyan
        type_qualifier    = {r=255, g=143, b=64},   -- #ff8f40 orange

        -- Operators and punctuation
        operator          = {r=255, g=143, b=64},   -- #ff8f40 orange
        punctuation       = {r=191, g=189, b=182},  -- #bfbdb6 fg
        punctuation_bracket   = {r=98,  g=106, b=115},  -- #626a73 gray
        punctuation_delimiter = {r=191, g=189, b=182},  -- #bfbdb6 fg

        -- Special
        constructor       = {r=115, g=208, b=255},  -- #73d0ff cyan
        namespace         = {r=89,  g=194, b=255},  -- #59c2ff blue
        module            = {r=89,  g=194, b=255},  -- #59c2ff blue
        label             = {r=230, g=180, b=80},   -- #e6b450 yellow
        tag               = {r=255, g=143, b=64},   -- #ff8f40 orange
        tag_attribute     = {r=115, g=208, b=255},  -- #73d0ff cyan
        preprocessor      = {r=255, g=143, b=64},   -- #ff8f40 orange

        -- Errors and warnings
        error             = {r=240, g=113, b=120},  -- #f07178 red
        warning           = {r=230, g=180, b=80},   -- #e6b450 yellow
    })

    if loki.status then
        loki.status("Ayu Dark theme loaded")
    end
end
