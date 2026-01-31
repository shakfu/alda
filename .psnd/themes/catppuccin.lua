-- Catppuccin Mocha Theme for Loki Editor
-- Based on https://github.com/catppuccin/catppuccin
--
-- Palette (Mocha):
--   rosewater: #f5e0dc    flamingo: #f2cdcd     pink: #f5c2e7
--   mauve: #cba6f7        red: #f38ba8          maroon: #eba0ac
--   peach: #fab387        yellow: #f9e2af       green: #a6e3a1
--   teal: #94e2d5         sky: #89dceb          sapphire: #74c7ec
--   blue: #89b4fa         lavender: #b4befe     text: #cdd6f4
--   subtext1: #bac2de     overlay0: #6c7086

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=205, g=214, b=244},  -- #cdd6f4 text
        nonprint  = {r=108, g=112, b=134},  -- #6c7086 overlay0
        comment   = {r=108, g=112, b=134},  -- #6c7086 overlay0
        mlcomment = {r=108, g=112, b=134},  -- #6c7086 overlay0
        keyword1  = {r=203, g=166, b=247},  -- #cba6f7 mauve
        keyword2  = {r=137, g=180, b=250},  -- #89b4fa blue
        string    = {r=166, g=227, b=161},  -- #a6e3a1 green
        number    = {r=250, g=179, b=135},  -- #fab387 peach
        match     = {r=249, g=226, b=175},  -- #f9e2af yellow

        -- Functions
        ["function"]      = {r=137, g=180, b=250},  -- #89b4fa blue
        function_builtin  = {r=250, g=179, b=135},  -- #fab387 peach
        function_call     = {r=137, g=180, b=250},  -- #89b4fa blue
        function_method   = {r=137, g=180, b=250},  -- #89b4fa blue
        function_macro    = {r=203, g=166, b=247},  -- #cba6f7 mauve

        -- Variables
        variable          = {r=205, g=214, b=244},  -- #cdd6f4 text
        variable_builtin  = {r=243, g=139, b=168},  -- #f38ba8 red
        variable_parameter= {r=235, g=160, b=172},  -- #eba0ac maroon
        variable_field    = {r=180, g=190, b=254},  -- #b4befe lavender
        variable_property = {r=180, g=190, b=254},  -- #b4befe lavender

        -- Keywords
        keyword_control   = {r=203, g=166, b=247},  -- #cba6f7 mauve
        keyword_function  = {r=203, g=166, b=247},  -- #cba6f7 mauve
        keyword_return    = {r=203, g=166, b=247},  -- #cba6f7 mauve
        keyword_operator  = {r=137, g=220, b=235},  -- #89dceb sky
        keyword_import    = {r=203, g=166, b=247},  -- #cba6f7 mauve
        keyword_type      = {r=249, g=226, b=175},  -- #f9e2af yellow
        keyword_modifier  = {r=203, g=166, b=247},  -- #cba6f7 mauve

        -- Strings
        string_escape     = {r=245, g=194, b=231},  -- #f5c2e7 pink
        string_regex      = {r=250, g=179, b=135},  -- #fab387 peach
        string_special    = {r=166, g=227, b=161},  -- #a6e3a1 green

        -- Numbers
        number_float      = {r=250, g=179, b=135},  -- #fab387 peach

        -- Literals
        boolean           = {r=250, g=179, b=135},  -- #fab387 peach
        constant          = {r=250, g=179, b=135},  -- #fab387 peach
        constant_builtin  = {r=250, g=179, b=135},  -- #fab387 peach

        -- Comments
        comment_doc       = {r=147, g=153, b=178},  -- #9399b2 overlay1

        -- Types
        type              = {r=249, g=226, b=175},  -- #f9e2af yellow
        type_builtin      = {r=249, g=226, b=175},  -- #f9e2af yellow
        type_parameter    = {r=245, g=194, b=231},  -- #f5c2e7 pink
        type_qualifier    = {r=203, g=166, b=247},  -- #cba6f7 mauve

        -- Operators and punctuation
        operator          = {r=137, g=220, b=235},  -- #89dceb sky
        punctuation       = {r=186, g=194, b=222},  -- #bac2de subtext1
        punctuation_bracket   = {r=186, g=194, b=222},  -- #bac2de subtext1
        punctuation_delimiter = {r=186, g=194, b=222},  -- #bac2de subtext1

        -- Special
        constructor       = {r=116, g=199, b=236},  -- #74c7ec sapphire
        namespace         = {r=137, g=180, b=250},  -- #89b4fa blue
        module            = {r=137, g=180, b=250},  -- #89b4fa blue
        label             = {r=116, g=199, b=236},  -- #74c7ec sapphire
        tag               = {r=203, g=166, b=247},  -- #cba6f7 mauve
        tag_attribute     = {r=249, g=226, b=175},  -- #f9e2af yellow
        preprocessor      = {r=245, g=194, b=231},  -- #f5c2e7 pink

        -- Errors and warnings
        error             = {r=243, g=139, b=168},  -- #f38ba8 red
        warning           = {r=249, g=226, b=175},  -- #f9e2af yellow
    })

    if loki.status then
        loki.status("Catppuccin Mocha theme loaded")
    end
end
