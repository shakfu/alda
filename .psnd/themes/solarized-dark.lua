-- Solarized Dark Theme for Loki Editor
-- Based on https://ethanschoonover.com/solarized/
--
-- Palette:
--   base03: #002b36       base02: #073642       base01: #586e75
--   base00: #657b83       base0: #839496        base1: #93a1a1
--   base2: #eee8d5        base3: #fdf6e3
--   yellow: #b58900       orange: #cb4b16       red: #dc322f
--   magenta: #d33682      violet: #6c71c4       blue: #268bd2
--   cyan: #2aa198         green: #859900

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=131, g=148, b=150},  -- #839496 base0
        nonprint  = {r=88,  g=110, b=117},  -- #586e75 base01
        comment   = {r=88,  g=110, b=117},  -- #586e75 base01
        mlcomment = {r=88,  g=110, b=117},  -- #586e75 base01
        keyword1  = {r=133, g=153, b=0},    -- #859900 green
        keyword2  = {r=38,  g=139, b=210},  -- #268bd2 blue
        string    = {r=42,  g=161, b=152},  -- #2aa198 cyan
        number    = {r=211, g=54,  b=130},  -- #d33682 magenta
        match     = {r=181, g=137, b=0},    -- #b58900 yellow

        -- Functions
        ["function"]      = {r=38,  g=139, b=210},  -- #268bd2 blue
        function_builtin  = {r=42,  g=161, b=152},  -- #2aa198 cyan
        function_call     = {r=38,  g=139, b=210},  -- #268bd2 blue
        function_method   = {r=38,  g=139, b=210},  -- #268bd2 blue
        function_macro    = {r=203, g=75,  b=22},   -- #cb4b16 orange

        -- Variables
        variable          = {r=131, g=148, b=150},  -- #839496 base0
        variable_builtin  = {r=203, g=75,  b=22},   -- #cb4b16 orange
        variable_parameter= {r=131, g=148, b=150},  -- #839496 base0
        variable_field    = {r=131, g=148, b=150},  -- #839496 base0
        variable_property = {r=131, g=148, b=150},  -- #839496 base0

        -- Keywords
        keyword_control   = {r=133, g=153, b=0},    -- #859900 green
        keyword_function  = {r=133, g=153, b=0},    -- #859900 green
        keyword_return    = {r=133, g=153, b=0},    -- #859900 green
        keyword_operator  = {r=133, g=153, b=0},    -- #859900 green
        keyword_import    = {r=203, g=75,  b=22},   -- #cb4b16 orange
        keyword_type      = {r=181, g=137, b=0},    -- #b58900 yellow
        keyword_modifier  = {r=133, g=153, b=0},    -- #859900 green

        -- Strings
        string_escape     = {r=220, g=50,  b=47},   -- #dc322f red
        string_regex      = {r=42,  g=161, b=152},  -- #2aa198 cyan
        string_special    = {r=42,  g=161, b=152},  -- #2aa198 cyan

        -- Numbers
        number_float      = {r=211, g=54,  b=130},  -- #d33682 magenta

        -- Literals
        boolean           = {r=211, g=54,  b=130},  -- #d33682 magenta
        constant          = {r=211, g=54,  b=130},  -- #d33682 magenta
        constant_builtin  = {r=203, g=75,  b=22},   -- #cb4b16 orange

        -- Comments
        comment_doc       = {r=88,  g=110, b=117},  -- #586e75 base01

        -- Types
        type              = {r=181, g=137, b=0},    -- #b58900 yellow
        type_builtin      = {r=181, g=137, b=0},    -- #b58900 yellow
        type_parameter    = {r=181, g=137, b=0},    -- #b58900 yellow
        type_qualifier    = {r=133, g=153, b=0},    -- #859900 green

        -- Operators and punctuation
        operator          = {r=133, g=153, b=0},    -- #859900 green
        punctuation       = {r=131, g=148, b=150},  -- #839496 base0
        punctuation_bracket   = {r=131, g=148, b=150},  -- #839496 base0
        punctuation_delimiter = {r=88,  g=110, b=117},  -- #586e75 base01

        -- Special
        constructor       = {r=108, g=113, b=196},  -- #6c71c4 violet
        namespace         = {r=108, g=113, b=196},  -- #6c71c4 violet
        module            = {r=108, g=113, b=196},  -- #6c71c4 violet
        label             = {r=42,  g=161, b=152},  -- #2aa198 cyan
        tag               = {r=38,  g=139, b=210},  -- #268bd2 blue
        tag_attribute     = {r=131, g=148, b=150},  -- #839496 base0
        preprocessor      = {r=203, g=75,  b=22},   -- #cb4b16 orange

        -- Errors and warnings
        error             = {r=220, g=50,  b=47},   -- #dc322f red
        warning           = {r=181, g=137, b=0},    -- #b58900 yellow
    })

    if loki.status then
        loki.status("Solarized Dark theme loaded")
    end
end
