-- GitHub Light Theme for Loki Editor
-- Inspired by GitHub's light theme
--
-- Palette:
--   Background: #ffffff    Foreground: #24292e
--   Comment: #6a737d       String: #032f62
--   Keyword: #d73a49       Function: #6f42c1
--   Variable: #e36209      Type: #6f42c1

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=36,  g=41,  b=46},   -- #24292e Foreground
        nonprint  = {r=149, g=157, b=165},  -- #959da5 Gray
        comment   = {r=106, g=115, b=125},  -- #6a737d Comment
        mlcomment = {r=106, g=115, b=125},  -- #6a737d Comment
        keyword1  = {r=215, g=58,  b=73},   -- #d73a49 Red
        keyword2  = {r=111, g=66,  b=193},  -- #6f42c1 Purple
        string    = {r=3,   g=47,  b=98},   -- #032f62 Dark blue
        number    = {r=0,   g=92,  b=197},  -- #005cc5 Blue
        match     = {r=255, g=223, b=0},    -- #ffdf00 Yellow

        -- Functions
        ["function"]      = {r=111, g=66,  b=193},  -- #6f42c1 Purple
        function_builtin  = {r=0,   g=92,  b=197},  -- #005cc5 Blue
        function_call     = {r=111, g=66,  b=193},  -- #6f42c1 Purple
        function_method   = {r=111, g=66,  b=193},  -- #6f42c1 Purple
        function_macro    = {r=111, g=66,  b=193},  -- #6f42c1 Purple

        -- Variables
        variable          = {r=36,  g=41,  b=46},   -- #24292e Foreground
        variable_builtin  = {r=0,   g=92,  b=197},  -- #005cc5 Blue
        variable_parameter= {r=227, g=98,  b=9},    -- #e36209 Orange
        variable_field    = {r=0,   g=92,  b=197},  -- #005cc5 Blue
        variable_property = {r=0,   g=92,  b=197},  -- #005cc5 Blue

        -- Keywords
        keyword_control   = {r=215, g=58,  b=73},   -- #d73a49 Red
        keyword_function  = {r=215, g=58,  b=73},   -- #d73a49 Red
        keyword_return    = {r=215, g=58,  b=73},   -- #d73a49 Red
        keyword_operator  = {r=215, g=58,  b=73},   -- #d73a49 Red
        keyword_import    = {r=215, g=58,  b=73},   -- #d73a49 Red
        keyword_type      = {r=215, g=58,  b=73},   -- #d73a49 Red
        keyword_modifier  = {r=215, g=58,  b=73},   -- #d73a49 Red

        -- Strings
        string_escape     = {r=0,   g=92,  b=197},  -- #005cc5 Blue
        string_regex      = {r=3,   g=47,  b=98},   -- #032f62 Dark blue
        string_special    = {r=3,   g=47,  b=98},   -- #032f62 Dark blue

        -- Numbers
        number_float      = {r=0,   g=92,  b=197},  -- #005cc5 Blue

        -- Literals
        boolean           = {r=0,   g=92,  b=197},  -- #005cc5 Blue
        constant          = {r=0,   g=92,  b=197},  -- #005cc5 Blue
        constant_builtin  = {r=0,   g=92,  b=197},  -- #005cc5 Blue

        -- Comments
        comment_doc       = {r=106, g=115, b=125},  -- #6a737d Comment

        -- Types
        type              = {r=111, g=66,  b=193},  -- #6f42c1 Purple
        type_builtin      = {r=111, g=66,  b=193},  -- #6f42c1 Purple
        type_parameter    = {r=227, g=98,  b=9},    -- #e36209 Orange
        type_qualifier    = {r=215, g=58,  b=73},   -- #d73a49 Red

        -- Operators and punctuation
        operator          = {r=215, g=58,  b=73},   -- #d73a49 Red
        punctuation       = {r=36,  g=41,  b=46},   -- #24292e Foreground
        punctuation_bracket   = {r=36,  g=41,  b=46},   -- #24292e Foreground
        punctuation_delimiter = {r=36,  g=41,  b=46},   -- #24292e Foreground

        -- Special
        constructor       = {r=111, g=66,  b=193},  -- #6f42c1 Purple
        namespace         = {r=227, g=98,  b=9},    -- #e36209 Orange
        module            = {r=227, g=98,  b=9},    -- #e36209 Orange
        label             = {r=0,   g=92,  b=197},  -- #005cc5 Blue
        tag               = {r=34,  g=134, b=58},   -- #22863a Green
        tag_attribute     = {r=111, g=66,  b=193},  -- #6f42c1 Purple
        preprocessor      = {r=215, g=58,  b=73},   -- #d73a49 Red

        -- Errors and warnings
        error             = {r=203, g=36,  b=49},   -- #cb2431 Red
        warning           = {r=227, g=98,  b=9},    -- #e36209 Orange
    })

    if loki.status then
        loki.status("GitHub Light theme loaded")
    end
end
