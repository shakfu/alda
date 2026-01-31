-- Kanagawa Theme for Loki Editor
-- Based on https://github.com/rebelot/kanagawa.nvim
-- Inspired by Katsushika Hokusai's "The Great Wave off Kanagawa"
--
-- Palette:
--   sumiInk: #16161d      fujiWhite: #dcd7ba   oldWhite: #c8c093
--   waveBlue: #223249     springBlue: #7fb4ca  crystalBlue: #7e9cd8
--   springGreen: #98bb6c  boatYellow: #938aa9  carpYellow: #e6c384
--   sakuraPink: #d27e99   peachRed: #ff5d62    surimiOrange: #ffa066
--   roninYellow: #ff9e3b  autumnGreen: #76946a autumnRed: #c34043
--   waveAqua: #6a9589     dragonBlue: #658594  oniViolet: #957fb8

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=220, g=215, b=186},  -- #dcd7ba fujiWhite
        nonprint  = {r=102, g=102, b=102},  -- #666666 fuji gray
        comment   = {r=102, g=102, b=102},  -- #666666 fuji gray
        mlcomment = {r=102, g=102, b=102},  -- #666666 fuji gray
        keyword1  = {r=149, g=127, b=184},  -- #957fb8 oniViolet
        keyword2  = {r=106, g=149, b=137},  -- #6a9589 waveAqua
        string    = {r=152, g=187, b=108},  -- #98bb6c springGreen
        number    = {r=210, g=126, b=153},  -- #d27e99 sakuraPink
        match     = {r=230, g=195, b=132},  -- #e6c384 carpYellow

        -- Functions
        ["function"]      = {r=126, g=156, b=216},  -- #7e9cd8 crystalBlue
        function_builtin  = {r=106, g=149, b=137},  -- #6a9589 waveAqua
        function_call     = {r=126, g=156, b=216},  -- #7e9cd8 crystalBlue
        function_method   = {r=126, g=156, b=216},  -- #7e9cd8 crystalBlue
        function_macro    = {r=255, g=160, b=102},  -- #ffa066 surimiOrange

        -- Variables
        variable          = {r=220, g=215, b=186},  -- #dcd7ba fujiWhite
        variable_builtin  = {r=255, g=93,  b=98},   -- #ff5d62 peachRed
        variable_parameter= {r=230, g=195, b=132},  -- #e6c384 carpYellow
        variable_field    = {r=127, g=180, b=202},  -- #7fb4ca springBlue
        variable_property = {r=127, g=180, b=202},  -- #7fb4ca springBlue

        -- Keywords
        keyword_control   = {r=149, g=127, b=184},  -- #957fb8 oniViolet
        keyword_function  = {r=149, g=127, b=184},  -- #957fb8 oniViolet
        keyword_return    = {r=149, g=127, b=184},  -- #957fb8 oniViolet
        keyword_operator  = {r=255, g=93,  b=98},   -- #ff5d62 peachRed
        keyword_import    = {r=149, g=127, b=184},  -- #957fb8 oniViolet
        keyword_type      = {r=106, g=149, b=137},  -- #6a9589 waveAqua
        keyword_modifier  = {r=149, g=127, b=184},  -- #957fb8 oniViolet

        -- Strings
        string_escape     = {r=255, g=160, b=102},  -- #ffa066 surimiOrange
        string_regex      = {r=255, g=158, b=59},   -- #ff9e3b roninYellow
        string_special    = {r=152, g=187, b=108},  -- #98bb6c springGreen

        -- Numbers
        number_float      = {r=210, g=126, b=153},  -- #d27e99 sakuraPink

        -- Literals
        boolean           = {r=255, g=160, b=102},  -- #ffa066 surimiOrange
        constant          = {r=255, g=160, b=102},  -- #ffa066 surimiOrange
        constant_builtin  = {r=255, g=160, b=102},  -- #ffa066 surimiOrange

        -- Comments
        comment_doc       = {r=102, g=102, b=102},  -- #666666 fuji gray

        -- Types
        type              = {r=106, g=149, b=137},  -- #6a9589 waveAqua
        type_builtin      = {r=106, g=149, b=137},  -- #6a9589 waveAqua
        type_parameter    = {r=106, g=149, b=137},  -- #6a9589 waveAqua
        type_qualifier    = {r=149, g=127, b=184},  -- #957fb8 oniViolet

        -- Operators and punctuation
        operator          = {r=255, g=93,  b=98},   -- #ff5d62 peachRed
        punctuation       = {r=220, g=215, b=186},  -- #dcd7ba fujiWhite
        punctuation_bracket   = {r=102, g=102, b=102},  -- #666666 gray
        punctuation_delimiter = {r=220, g=215, b=186},  -- #dcd7ba fujiWhite

        -- Special
        constructor       = {r=230, g=195, b=132},  -- #e6c384 carpYellow
        namespace         = {r=34,  g=50,  b=73},   -- #223249 waveBlue (darker)
        module            = {r=34,  g=50,  b=73},   -- #223249 waveBlue
        label             = {r=147, g=138, b=169},  -- #938aa9 boatYellow
        tag               = {r=195, g=64,  b=67},   -- #c34043 autumnRed
        tag_attribute     = {r=118, g=148, b=106},  -- #76946a autumnGreen
        preprocessor      = {r=149, g=127, b=184},  -- #957fb8 oniViolet

        -- Errors and warnings
        error             = {r=255, g=93,  b=98},   -- #ff5d62 peachRed
        warning           = {r=255, g=158, b=59},   -- #ff9e3b roninYellow
    })

    if loki.status then
        loki.status("Kanagawa theme loaded")
    end
end
