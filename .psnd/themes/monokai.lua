-- Monokai Theme for Loki Editor
-- Classic Monokai/Monokai Pro color scheme
--
-- Palette:
--   Background: #272822    Foreground: #f8f8f2
--   Comment: #75715e       Yellow: #e6db74
--   Pink: #f92672          Green: #a6e22e
--   Cyan: #66d9ef          Orange: #fd971f
--   Purple: #ae81ff

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=248, g=248, b=242},  -- #f8f8f2 Foreground
        nonprint  = {r=73,  g=72,  b=62},   -- #49483e Dark gray
        comment   = {r=117, g=113, b=94},   -- #75715e Comment
        mlcomment = {r=117, g=113, b=94},   -- #75715e Comment
        keyword1  = {r=249, g=38,  b=114},  -- #f92672 Pink
        keyword2  = {r=102, g=217, b=239},  -- #66d9ef Cyan
        string    = {r=230, g=219, b=116},  -- #e6db74 Yellow
        number    = {r=174, g=129, b=255},  -- #ae81ff Purple
        match     = {r=166, g=226, b=46},   -- #a6e22e Green

        -- Functions
        ["function"]      = {r=166, g=226, b=46},   -- #a6e22e Green
        function_builtin  = {r=102, g=217, b=239},  -- #66d9ef Cyan
        function_call     = {r=166, g=226, b=46},   -- #a6e22e Green
        function_method   = {r=166, g=226, b=46},   -- #a6e22e Green
        function_macro    = {r=102, g=217, b=239},  -- #66d9ef Cyan

        -- Variables
        variable          = {r=248, g=248, b=242},  -- #f8f8f2 Foreground
        variable_builtin  = {r=253, g=151, b=31},   -- #fd971f Orange (self/this)
        variable_parameter= {r=253, g=151, b=31},   -- #fd971f Orange
        variable_field    = {r=248, g=248, b=242},  -- #f8f8f2 Foreground
        variable_property = {r=248, g=248, b=242},  -- #f8f8f2 Foreground

        -- Keywords
        keyword_control   = {r=249, g=38,  b=114},  -- #f92672 Pink
        keyword_function  = {r=102, g=217, b=239},  -- #66d9ef Cyan
        keyword_return    = {r=249, g=38,  b=114},  -- #f92672 Pink
        keyword_operator  = {r=249, g=38,  b=114},  -- #f92672 Pink
        keyword_import    = {r=249, g=38,  b=114},  -- #f92672 Pink
        keyword_type      = {r=102, g=217, b=239},  -- #66d9ef Cyan
        keyword_modifier  = {r=249, g=38,  b=114},  -- #f92672 Pink

        -- Strings
        string_escape     = {r=174, g=129, b=255},  -- #ae81ff Purple
        string_regex      = {r=230, g=219, b=116},  -- #e6db74 Yellow
        string_special    = {r=230, g=219, b=116},  -- #e6db74 Yellow

        -- Numbers
        number_float      = {r=174, g=129, b=255},  -- #ae81ff Purple

        -- Literals
        boolean           = {r=174, g=129, b=255},  -- #ae81ff Purple
        constant          = {r=174, g=129, b=255},  -- #ae81ff Purple
        constant_builtin  = {r=174, g=129, b=255},  -- #ae81ff Purple

        -- Comments
        comment_doc       = {r=117, g=113, b=94},   -- #75715e Comment

        -- Types
        type              = {r=102, g=217, b=239},  -- #66d9ef Cyan
        type_builtin      = {r=102, g=217, b=239},  -- #66d9ef Cyan
        type_parameter    = {r=253, g=151, b=31},   -- #fd971f Orange
        type_qualifier    = {r=249, g=38,  b=114},  -- #f92672 Pink

        -- Operators and punctuation
        operator          = {r=249, g=38,  b=114},  -- #f92672 Pink
        punctuation       = {r=248, g=248, b=242},  -- #f8f8f2 Foreground
        punctuation_bracket   = {r=248, g=248, b=242},  -- #f8f8f2 Foreground
        punctuation_delimiter = {r=248, g=248, b=242},  -- #f8f8f2 Foreground

        -- Special
        constructor       = {r=166, g=226, b=46},   -- #a6e22e Green
        namespace         = {r=166, g=226, b=46},   -- #a6e22e Green
        module            = {r=166, g=226, b=46},   -- #a6e22e Green
        label             = {r=230, g=219, b=116},  -- #e6db74 Yellow
        tag               = {r=249, g=38,  b=114},  -- #f92672 Pink
        tag_attribute     = {r=166, g=226, b=46},   -- #a6e22e Green
        preprocessor      = {r=249, g=38,  b=114},  -- #f92672 Pink

        -- Errors and warnings
        error             = {r=249, g=38,  b=114},  -- #f92672 Pink/Red
        warning           = {r=253, g=151, b=31},   -- #fd971f Orange
    })

    if loki.status then
        loki.status("Monokai theme loaded")
    end
end
