-- Gruvbox Dark Theme for Loki Editor
-- Based on https://github.com/morhetz/gruvbox
--
-- Palette:
--   bg: #282828           fg: #ebdbb2
--   red: #cc241d          green: #98971a
--   yellow: #d79921       blue: #458588
--   purple: #b16286       aqua: #689d6a
--   orange: #d65d0e       gray: #928374

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=235, g=219, b=178},  -- #ebdbb2 fg
        nonprint  = {r=146, g=131, b=116},  -- #928374 gray
        comment   = {r=146, g=131, b=116},  -- #928374 gray
        mlcomment = {r=146, g=131, b=116},  -- #928374 gray
        keyword1  = {r=251, g=73,  b=52},   -- #fb4934 bright red
        keyword2  = {r=131, g=165, b=152},  -- #83a598 bright blue
        string    = {r=184, g=187, b=38},   -- #b8bb26 bright green
        number    = {r=211, g=134, b=155},  -- #d3869b bright purple
        match     = {r=250, g=189, b=47},   -- #fabd2f bright yellow

        -- Functions
        ["function"]      = {r=184, g=187, b=38},   -- #b8bb26 bright green
        function_builtin  = {r=142, g=192, b=124},  -- #8ec07c bright aqua
        function_call     = {r=184, g=187, b=38},   -- #b8bb26 bright green
        function_method   = {r=184, g=187, b=38},   -- #b8bb26 bright green
        function_macro    = {r=142, g=192, b=124},  -- #8ec07c bright aqua

        -- Variables
        variable          = {r=235, g=219, b=178},  -- #ebdbb2 fg
        variable_builtin  = {r=254, g=128, b=25},   -- #fe8019 bright orange
        variable_parameter= {r=131, g=165, b=152},  -- #83a598 bright blue
        variable_field    = {r=235, g=219, b=178},  -- #ebdbb2 fg
        variable_property = {r=235, g=219, b=178},  -- #ebdbb2 fg

        -- Keywords
        keyword_control   = {r=251, g=73,  b=52},   -- #fb4934 bright red
        keyword_function  = {r=142, g=192, b=124},  -- #8ec07c bright aqua
        keyword_return    = {r=251, g=73,  b=52},   -- #fb4934 bright red
        keyword_operator  = {r=251, g=73,  b=52},   -- #fb4934 bright red
        keyword_import    = {r=142, g=192, b=124},  -- #8ec07c bright aqua
        keyword_type      = {r=250, g=189, b=47},   -- #fabd2f bright yellow
        keyword_modifier  = {r=251, g=73,  b=52},   -- #fb4934 bright red

        -- Strings
        string_escape     = {r=254, g=128, b=25},   -- #fe8019 bright orange
        string_regex      = {r=184, g=187, b=38},   -- #b8bb26 bright green
        string_special    = {r=184, g=187, b=38},   -- #b8bb26 bright green

        -- Numbers
        number_float      = {r=211, g=134, b=155},  -- #d3869b bright purple

        -- Literals
        boolean           = {r=211, g=134, b=155},  -- #d3869b bright purple
        constant          = {r=211, g=134, b=155},  -- #d3869b bright purple
        constant_builtin  = {r=254, g=128, b=25},   -- #fe8019 bright orange

        -- Comments
        comment_doc       = {r=146, g=131, b=116},  -- #928374 gray

        -- Types
        type              = {r=250, g=189, b=47},   -- #fabd2f bright yellow
        type_builtin      = {r=250, g=189, b=47},   -- #fabd2f bright yellow
        type_parameter    = {r=131, g=165, b=152},  -- #83a598 bright blue
        type_qualifier    = {r=251, g=73,  b=52},   -- #fb4934 bright red

        -- Operators and punctuation
        operator          = {r=235, g=219, b=178},  -- #ebdbb2 fg
        punctuation       = {r=235, g=219, b=178},  -- #ebdbb2 fg
        punctuation_bracket   = {r=235, g=219, b=178},  -- #ebdbb2 fg
        punctuation_delimiter = {r=168, g=153, b=132},  -- #a89984 gray

        -- Special
        constructor       = {r=250, g=189, b=47},   -- #fabd2f bright yellow
        namespace         = {r=142, g=192, b=124},  -- #8ec07c bright aqua
        module            = {r=142, g=192, b=124},  -- #8ec07c bright aqua
        label             = {r=142, g=192, b=124},  -- #8ec07c bright aqua
        tag               = {r=251, g=73,  b=52},   -- #fb4934 bright red
        tag_attribute     = {r=250, g=189, b=47},   -- #fabd2f bright yellow
        preprocessor      = {r=142, g=192, b=124},  -- #8ec07c bright aqua

        -- Errors and warnings
        error             = {r=251, g=73,  b=52},   -- #fb4934 bright red
        warning           = {r=250, g=189, b=47},   -- #fabd2f bright yellow
    })

    if loki.status then
        loki.status("Gruvbox Dark theme loaded")
    end
end
