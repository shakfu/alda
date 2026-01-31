-- Tokyo Night Theme for Loki Editor
-- Based on https://github.com/enkia/tokyo-night-vscode-theme
--
-- Palette:
--   bg: #1a1b26           fg: #a9b1d6
--   red: #f7768e          orange: #ff9e64
--   yellow: #e0af68       green: #9ece6a
--   teal: #73daca         cyan: #7dcfff
--   blue: #7aa2f7         purple: #bb9af7
--   magenta: #9d7cd8      comment: #565f89

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=169, g=177, b=214},  -- #a9b1d6 fg
        nonprint  = {r=86,  g=95,  b=137},  -- #565f89 comment
        comment   = {r=86,  g=95,  b=137},  -- #565f89 comment
        mlcomment = {r=86,  g=95,  b=137},  -- #565f89 comment
        keyword1  = {r=187, g=154, b=247},  -- #bb9af7 purple
        keyword2  = {r=45,  g=212, b=191},  -- #2dc4bf teal
        string    = {r=158, g=206, b=106},  -- #9ece6a green
        number    = {r=255, g=158, b=100},  -- #ff9e64 orange
        match     = {r=224, g=175, b=104},  -- #e0af68 yellow

        -- Functions
        ["function"]      = {r=122, g=162, b=247},  -- #7aa2f7 blue
        function_builtin  = {r=125, g=207, b=255},  -- #7dcfff cyan
        function_call     = {r=122, g=162, b=247},  -- #7aa2f7 blue
        function_method   = {r=122, g=162, b=247},  -- #7aa2f7 blue
        function_macro    = {r=125, g=207, b=255},  -- #7dcfff cyan

        -- Variables
        variable          = {r=169, g=177, b=214},  -- #a9b1d6 fg
        variable_builtin  = {r=247, g=118, b=142},  -- #f7768e red
        variable_parameter= {r=224, g=175, b=104},  -- #e0af68 yellow
        variable_field    = {r=115, g=218, b=202},  -- #73daca teal
        variable_property = {r=115, g=218, b=202},  -- #73daca teal

        -- Keywords
        keyword_control   = {r=187, g=154, b=247},  -- #bb9af7 purple
        keyword_function  = {r=187, g=154, b=247},  -- #bb9af7 purple
        keyword_return    = {r=187, g=154, b=247},  -- #bb9af7 purple
        keyword_operator  = {r=137, g=221, b=255},  -- #89ddff cyan
        keyword_import    = {r=187, g=154, b=247},  -- #bb9af7 purple
        keyword_type      = {r=45,  g=212, b=191},  -- #2dc4bf teal
        keyword_modifier  = {r=187, g=154, b=247},  -- #bb9af7 purple

        -- Strings
        string_escape     = {r=137, g=221, b=255},  -- #89ddff cyan
        string_regex      = {r=255, g=158, b=100},  -- #ff9e64 orange
        string_special    = {r=158, g=206, b=106},  -- #9ece6a green

        -- Numbers
        number_float      = {r=255, g=158, b=100},  -- #ff9e64 orange

        -- Literals
        boolean           = {r=255, g=158, b=100},  -- #ff9e64 orange
        constant          = {r=255, g=158, b=100},  -- #ff9e64 orange
        constant_builtin  = {r=255, g=158, b=100},  -- #ff9e64 orange

        -- Comments
        comment_doc       = {r=86,  g=95,  b=137},  -- #565f89 comment

        -- Types
        type              = {r=45,  g=212, b=191},  -- #2dc4bf teal
        type_builtin      = {r=45,  g=212, b=191},  -- #2dc4bf teal
        type_parameter    = {r=224, g=175, b=104},  -- #e0af68 yellow
        type_qualifier    = {r=187, g=154, b=247},  -- #bb9af7 purple

        -- Operators and punctuation
        operator          = {r=137, g=221, b=255},  -- #89ddff cyan
        punctuation       = {r=169, g=177, b=214},  -- #a9b1d6 fg
        punctuation_bracket   = {r=169, g=177, b=214},  -- #a9b1d6 fg
        punctuation_delimiter = {r=169, g=177, b=214},  -- #a9b1d6 fg

        -- Special
        constructor       = {r=122, g=162, b=247},  -- #7aa2f7 blue
        namespace         = {r=122, g=162, b=247},  -- #7aa2f7 blue
        module            = {r=122, g=162, b=247},  -- #7aa2f7 blue
        label             = {r=122, g=162, b=247},  -- #7aa2f7 blue
        tag               = {r=247, g=118, b=142},  -- #f7768e red
        tag_attribute     = {r=187, g=154, b=247},  -- #bb9af7 purple
        preprocessor      = {r=187, g=154, b=247},  -- #bb9af7 purple

        -- Errors and warnings
        error             = {r=247, g=118, b=142},  -- #f7768e red
        warning           = {r=224, g=175, b=104},  -- #e0af68 yellow
    })

    if loki.status then
        loki.status("Tokyo Night theme loaded")
    end
end
