-- Basic 16-Color Theme for Loki Editor
-- Simple theme using standard ANSI colors for basic terminals
--
-- Uses standard 16-color ANSI palette:
--   0=black, 1=red, 2=green, 3=yellow, 4=blue, 5=magenta, 6=cyan, 7=white
--   8=bright black (gray), 9-15=bright versions

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=192, g=192, b=192},  -- white/fg
        nonprint  = {r=128, g=128, b=128},  -- gray (8)
        comment   = {r=128, g=128, b=128},  -- gray (8)
        mlcomment = {r=128, g=128, b=128},  -- gray (8)
        keyword1  = {r=128, g=0,   b=128},  -- magenta (5)
        keyword2  = {r=0,   g=128, b=128},  -- cyan (6)
        string    = {r=0,   g=128, b=0},    -- green (2)
        number    = {r=128, g=128, b=0},    -- yellow (3)
        match     = {r=128, g=128, b=0},    -- yellow (3)

        -- Functions
        ["function"]      = {r=0,   g=0,   b=128},  -- blue (4)
        function_builtin  = {r=0,   g=128, b=128},  -- cyan (6)
        function_call     = {r=0,   g=0,   b=128},  -- blue (4)
        function_method   = {r=0,   g=0,   b=128},  -- blue (4)
        function_macro    = {r=0,   g=128, b=128},  -- cyan (6)

        -- Variables
        variable          = {r=192, g=192, b=192},  -- white/fg
        variable_builtin  = {r=128, g=0,   b=0},    -- red (1)
        variable_parameter= {r=0,   g=128, b=128},  -- cyan (6)
        variable_field    = {r=192, g=192, b=192},  -- white/fg
        variable_property = {r=192, g=192, b=192},  -- white/fg

        -- Keywords
        keyword_control   = {r=128, g=0,   b=128},  -- magenta (5)
        keyword_function  = {r=128, g=0,   b=128},  -- magenta (5)
        keyword_return    = {r=128, g=0,   b=128},  -- magenta (5)
        keyword_operator  = {r=128, g=0,   b=128},  -- magenta (5)
        keyword_import    = {r=128, g=0,   b=128},  -- magenta (5)
        keyword_type      = {r=128, g=128, b=0},    -- yellow (3)
        keyword_modifier  = {r=128, g=0,   b=128},  -- magenta (5)

        -- Strings
        string_escape     = {r=0,   g=128, b=128},  -- cyan (6)
        string_regex      = {r=0,   g=128, b=128},  -- cyan (6)
        string_special    = {r=0,   g=128, b=0},    -- green (2)

        -- Numbers
        number_float      = {r=128, g=128, b=0},    -- yellow (3)

        -- Literals
        boolean           = {r=128, g=128, b=0},    -- yellow (3)
        constant          = {r=128, g=128, b=0},    -- yellow (3)
        constant_builtin  = {r=128, g=128, b=0},    -- yellow (3)

        -- Comments
        comment_doc       = {r=128, g=128, b=128},  -- gray (8)

        -- Types
        type              = {r=0,   g=128, b=128},  -- cyan (6)
        type_builtin      = {r=0,   g=128, b=128},  -- cyan (6)
        type_parameter    = {r=0,   g=128, b=128},  -- cyan (6)
        type_qualifier    = {r=128, g=0,   b=128},  -- magenta (5)

        -- Operators and punctuation
        operator          = {r=192, g=192, b=192},  -- white/fg
        punctuation       = {r=192, g=192, b=192},  -- white/fg
        punctuation_bracket   = {r=192, g=192, b=192},  -- white/fg
        punctuation_delimiter = {r=192, g=192, b=192},  -- white/fg

        -- Special
        constructor       = {r=128, g=128, b=0},    -- yellow (3)
        namespace         = {r=0,   g=128, b=128},  -- cyan (6)
        module            = {r=0,   g=128, b=128},  -- cyan (6)
        label             = {r=128, g=128, b=0},    -- yellow (3)
        tag               = {r=128, g=0,   b=0},    -- red (1)
        tag_attribute     = {r=128, g=128, b=0},    -- yellow (3)
        preprocessor      = {r=128, g=0,   b=128},  -- magenta (5)

        -- Errors and warnings
        error             = {r=128, g=0,   b=0},    -- red (1)
        warning           = {r=128, g=128, b=0},    -- yellow (3)
    })

    if loki.status then
        loki.status("Basic 16-color theme loaded")
    end
end
