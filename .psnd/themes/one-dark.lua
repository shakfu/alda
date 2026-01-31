-- One Dark Theme for Loki Editor
-- Based on Atom's One Dark theme
--
-- Palette:
--   bg: #282c34           fg: #abb2bf
--   red: #e06c75          green: #98c379
--   yellow: #e5c07b       blue: #61afef
--   purple: #c678dd       cyan: #56b6c2
--   orange: #d19a66       comment: #5c6370

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=171, g=178, b=191},  -- #abb2bf fg
        nonprint  = {r=92,  g=99,  b=112},  -- #5c6370 comment
        comment   = {r=92,  g=99,  b=112},  -- #5c6370 comment
        mlcomment = {r=92,  g=99,  b=112},  -- #5c6370 comment
        keyword1  = {r=198, g=120, b=221},  -- #c678dd purple
        keyword2  = {r=86,  g=182, b=194},  -- #56b6c2 cyan
        string    = {r=152, g=195, b=121},  -- #98c379 green
        number    = {r=209, g=154, b=102},  -- #d19a66 orange
        match     = {r=229, g=192, b=123},  -- #e5c07b yellow

        -- Functions
        ["function"]      = {r=97,  g=175, b=239},  -- #61afef blue
        function_builtin  = {r=86,  g=182, b=194},  -- #56b6c2 cyan
        function_call     = {r=97,  g=175, b=239},  -- #61afef blue
        function_method   = {r=97,  g=175, b=239},  -- #61afef blue
        function_macro    = {r=198, g=120, b=221},  -- #c678dd purple

        -- Variables
        variable          = {r=224, g=108, b=117},  -- #e06c75 red
        variable_builtin  = {r=224, g=108, b=117},  -- #e06c75 red
        variable_parameter= {r=171, g=178, b=191},  -- #abb2bf fg
        variable_field    = {r=224, g=108, b=117},  -- #e06c75 red
        variable_property = {r=224, g=108, b=117},  -- #e06c75 red

        -- Keywords
        keyword_control   = {r=198, g=120, b=221},  -- #c678dd purple
        keyword_function  = {r=198, g=120, b=221},  -- #c678dd purple
        keyword_return    = {r=198, g=120, b=221},  -- #c678dd purple
        keyword_operator  = {r=198, g=120, b=221},  -- #c678dd purple
        keyword_import    = {r=198, g=120, b=221},  -- #c678dd purple
        keyword_type      = {r=86,  g=182, b=194},  -- #56b6c2 cyan
        keyword_modifier  = {r=198, g=120, b=221},  -- #c678dd purple

        -- Strings
        string_escape     = {r=86,  g=182, b=194},  -- #56b6c2 cyan
        string_regex      = {r=152, g=195, b=121},  -- #98c379 green
        string_special    = {r=152, g=195, b=121},  -- #98c379 green

        -- Numbers
        number_float      = {r=209, g=154, b=102},  -- #d19a66 orange

        -- Literals
        boolean           = {r=209, g=154, b=102},  -- #d19a66 orange
        constant          = {r=209, g=154, b=102},  -- #d19a66 orange
        constant_builtin  = {r=209, g=154, b=102},  -- #d19a66 orange

        -- Comments
        comment_doc       = {r=92,  g=99,  b=112},  -- #5c6370 comment

        -- Types
        type              = {r=229, g=192, b=123},  -- #e5c07b yellow
        type_builtin      = {r=229, g=192, b=123},  -- #e5c07b yellow
        type_parameter    = {r=229, g=192, b=123},  -- #e5c07b yellow
        type_qualifier    = {r=198, g=120, b=221},  -- #c678dd purple

        -- Operators and punctuation
        operator          = {r=171, g=178, b=191},  -- #abb2bf fg
        punctuation       = {r=171, g=178, b=191},  -- #abb2bf fg
        punctuation_bracket   = {r=171, g=178, b=191},  -- #abb2bf fg
        punctuation_delimiter = {r=171, g=178, b=191},  -- #abb2bf fg

        -- Special
        constructor       = {r=229, g=192, b=123},  -- #e5c07b yellow
        namespace         = {r=229, g=192, b=123},  -- #e5c07b yellow
        module            = {r=229, g=192, b=123},  -- #e5c07b yellow
        label             = {r=224, g=108, b=117},  -- #e06c75 red
        tag               = {r=224, g=108, b=117},  -- #e06c75 red
        tag_attribute     = {r=209, g=154, b=102},  -- #d19a66 orange
        preprocessor      = {r=198, g=120, b=221},  -- #c678dd purple

        -- Errors and warnings
        error             = {r=224, g=108, b=117},  -- #e06c75 red
        warning           = {r=229, g=192, b=123},  -- #e5c07b yellow
    })

    if loki.status then
        loki.status("One Dark theme loaded")
    end
end
