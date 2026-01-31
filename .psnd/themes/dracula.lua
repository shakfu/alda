-- Dracula Theme for Loki Editor
-- Based on https://draculatheme.com
--
-- Palette:
--   Background: #282a36    Current Line: #44475a    Selection: #44475a
--   Foreground: #f8f8f2    Comment: #6272a4
--   Cyan: #8be9fd          Green: #50fa7b           Orange: #ffb86c
--   Pink: #ff79c6          Purple: #bd93f9          Red: #ff5555
--   Yellow: #f1fa8c

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=248, g=248, b=242},  -- #f8f8f2 Foreground
        nonprint  = {r=68,  g=71,  b=90},   -- #44475a Current Line
        comment   = {r=98,  g=114, b=164},  -- #6272a4 Comment
        mlcomment = {r=98,  g=114, b=164},  -- #6272a4 Comment
        keyword1  = {r=255, g=121, b=198},  -- #ff79c6 Pink
        keyword2  = {r=139, g=233, b=253},  -- #8be9fd Cyan
        string    = {r=241, g=250, b=140},  -- #f1fa8c Yellow
        number    = {r=189, g=147, b=249},  -- #bd93f9 Purple
        match     = {r=80,  g=250, b=123},  -- #50fa7b Green

        -- Functions
        ["function"]      = {r=80,  g=250, b=123},  -- #50fa7b Green
        function_builtin  = {r=139, g=233, b=253},  -- #8be9fd Cyan
        function_call     = {r=80,  g=250, b=123},  -- #50fa7b Green
        function_method   = {r=80,  g=250, b=123},  -- #50fa7b Green
        function_macro    = {r=139, g=233, b=253},  -- #8be9fd Cyan

        -- Variables
        variable          = {r=248, g=248, b=242},  -- #f8f8f2 Foreground
        variable_builtin  = {r=189, g=147, b=249},  -- #bd93f9 Purple (self/this)
        variable_parameter= {r=255, g=184, b=108},  -- #ffb86c Orange
        variable_field    = {r=248, g=248, b=242},  -- #f8f8f2 Foreground
        variable_property = {r=248, g=248, b=242},  -- #f8f8f2 Foreground

        -- Keywords
        keyword_control   = {r=255, g=121, b=198},  -- #ff79c6 Pink
        keyword_function  = {r=139, g=233, b=253},  -- #8be9fd Cyan
        keyword_return    = {r=255, g=121, b=198},  -- #ff79c6 Pink
        keyword_operator  = {r=255, g=121, b=198},  -- #ff79c6 Pink
        keyword_import    = {r=255, g=121, b=198},  -- #ff79c6 Pink
        keyword_type      = {r=139, g=233, b=253},  -- #8be9fd Cyan
        keyword_modifier  = {r=255, g=121, b=198},  -- #ff79c6 Pink

        -- Strings
        string_escape     = {r=255, g=121, b=198},  -- #ff79c6 Pink
        string_regex      = {r=255, g=85,  b=85},   -- #ff5555 Red
        string_special    = {r=241, g=250, b=140},  -- #f1fa8c Yellow

        -- Numbers
        number_float      = {r=189, g=147, b=249},  -- #bd93f9 Purple

        -- Literals
        boolean           = {r=189, g=147, b=249},  -- #bd93f9 Purple
        constant          = {r=189, g=147, b=249},  -- #bd93f9 Purple
        constant_builtin  = {r=189, g=147, b=249},  -- #bd93f9 Purple

        -- Comments
        comment_doc       = {r=98,  g=114, b=164},  -- #6272a4 Comment

        -- Types
        type              = {r=139, g=233, b=253},  -- #8be9fd Cyan
        type_builtin      = {r=139, g=233, b=253},  -- #8be9fd Cyan
        type_parameter    = {r=255, g=184, b=108},  -- #ffb86c Orange
        type_qualifier    = {r=255, g=121, b=198},  -- #ff79c6 Pink

        -- Operators and punctuation
        operator          = {r=255, g=121, b=198},  -- #ff79c6 Pink
        punctuation       = {r=248, g=248, b=242},  -- #f8f8f2 Foreground
        punctuation_bracket   = {r=248, g=248, b=242},  -- #f8f8f2 Foreground
        punctuation_delimiter = {r=248, g=248, b=242},  -- #f8f8f2 Foreground

        -- Special
        constructor       = {r=139, g=233, b=253},  -- #8be9fd Cyan
        namespace         = {r=255, g=121, b=198},  -- #ff79c6 Pink
        module            = {r=255, g=121, b=198},  -- #ff79c6 Pink
        label             = {r=139, g=233, b=253},  -- #8be9fd Cyan
        tag               = {r=255, g=121, b=198},  -- #ff79c6 Pink
        tag_attribute     = {r=80,  g=250, b=123},  -- #50fa7b Green
        preprocessor      = {r=255, g=121, b=198},  -- #ff79c6 Pink

        -- Errors and warnings
        error             = {r=255, g=85,  b=85},   -- #ff5555 Red
        warning           = {r=255, g=184, b=108},  -- #ffb86c Orange
    })

    if loki.status then
        loki.status("Dracula theme loaded")
    end
end
