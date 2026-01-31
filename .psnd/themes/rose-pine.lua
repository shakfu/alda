-- Rose Pine Theme for Loki Editor
-- Based on https://rosepinetheme.com/
--
-- Palette (Main):
--   base: #191724         surface: #1f1d2e       overlay: #26233a
--   muted: #6e6a86        subtle: #908caa        text: #e0def4
--   love: #eb6f92         gold: #f6c177          rose: #ebbcba
--   pine: #31748f         foam: #9ccfd8          iris: #c4a7e7

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=224, g=222, b=244},  -- #e0def4 text
        nonprint  = {r=110, g=106, b=134},  -- #6e6a86 muted
        comment   = {r=110, g=106, b=134},  -- #6e6a86 muted
        mlcomment = {r=110, g=106, b=134},  -- #6e6a86 muted
        keyword1  = {r=49,  g=116, b=143},  -- #31748f pine
        keyword2  = {r=156, g=207, b=216},  -- #9ccfd8 foam
        string    = {r=246, g=193, b=119},  -- #f6c177 gold
        number    = {r=235, g=188, b=186},  -- #ebbcba rose
        match     = {r=246, g=193, b=119},  -- #f6c177 gold

        -- Functions
        ["function"]      = {r=235, g=188, b=186},  -- #ebbcba rose
        function_builtin  = {r=156, g=207, b=216},  -- #9ccfd8 foam
        function_call     = {r=235, g=188, b=186},  -- #ebbcba rose
        function_method   = {r=235, g=188, b=186},  -- #ebbcba rose
        function_macro    = {r=196, g=167, b=231},  -- #c4a7e7 iris

        -- Variables
        variable          = {r=224, g=222, b=244},  -- #e0def4 text
        variable_builtin  = {r=235, g=111, b=146},  -- #eb6f92 love
        variable_parameter= {r=196, g=167, b=231},  -- #c4a7e7 iris
        variable_field    = {r=224, g=222, b=244},  -- #e0def4 text
        variable_property = {r=224, g=222, b=244},  -- #e0def4 text

        -- Keywords
        keyword_control   = {r=49,  g=116, b=143},  -- #31748f pine
        keyword_function  = {r=49,  g=116, b=143},  -- #31748f pine
        keyword_return    = {r=49,  g=116, b=143},  -- #31748f pine
        keyword_operator  = {r=144, g=140, b=170},  -- #908caa subtle
        keyword_import    = {r=49,  g=116, b=143},  -- #31748f pine
        keyword_type      = {r=156, g=207, b=216},  -- #9ccfd8 foam
        keyword_modifier  = {r=49,  g=116, b=143},  -- #31748f pine

        -- Strings
        string_escape     = {r=196, g=167, b=231},  -- #c4a7e7 iris
        string_regex      = {r=235, g=111, b=146},  -- #eb6f92 love
        string_special    = {r=246, g=193, b=119},  -- #f6c177 gold

        -- Numbers
        number_float      = {r=235, g=188, b=186},  -- #ebbcba rose

        -- Literals
        boolean           = {r=235, g=188, b=186},  -- #ebbcba rose
        constant          = {r=235, g=188, b=186},  -- #ebbcba rose
        constant_builtin  = {r=235, g=111, b=146},  -- #eb6f92 love

        -- Comments
        comment_doc       = {r=144, g=140, b=170},  -- #908caa subtle

        -- Types
        type              = {r=156, g=207, b=216},  -- #9ccfd8 foam
        type_builtin      = {r=156, g=207, b=216},  -- #9ccfd8 foam
        type_parameter    = {r=196, g=167, b=231},  -- #c4a7e7 iris
        type_qualifier    = {r=49,  g=116, b=143},  -- #31748f pine

        -- Operators and punctuation
        operator          = {r=144, g=140, b=170},  -- #908caa subtle
        punctuation       = {r=144, g=140, b=170},  -- #908caa subtle
        punctuation_bracket   = {r=144, g=140, b=170},  -- #908caa subtle
        punctuation_delimiter = {r=110, g=106, b=134},  -- #6e6a86 muted

        -- Special
        constructor       = {r=156, g=207, b=216},  -- #9ccfd8 foam
        namespace         = {r=196, g=167, b=231},  -- #c4a7e7 iris
        module            = {r=196, g=167, b=231},  -- #c4a7e7 iris
        label             = {r=246, g=193, b=119},  -- #f6c177 gold
        tag               = {r=156, g=207, b=216},  -- #9ccfd8 foam
        tag_attribute     = {r=196, g=167, b=231},  -- #c4a7e7 iris
        preprocessor      = {r=196, g=167, b=231},  -- #c4a7e7 iris

        -- Errors and warnings
        error             = {r=235, g=111, b=146},  -- #eb6f92 love
        warning           = {r=246, g=193, b=119},  -- #f6c177 gold
    })

    if loki.status then
        loki.status("Rose Pine theme loaded")
    end
end
