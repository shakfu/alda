-- Everforest Theme for Loki Editor
-- Based on https://github.com/sainnhe/everforest
--
-- Palette (Dark Medium):
--   bg: #2d353b           fg: #d3c6aa
--   red: #e67e80          orange: #e69875
--   yellow: #dbbc7f       green: #a7c080
--   aqua: #83c092         blue: #7fbbb3
--   purple: #d699b6       gray: #859289

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=211, g=198, b=170},  -- #d3c6aa fg
        nonprint  = {r=133, g=146, b=137},  -- #859289 gray
        comment   = {r=133, g=146, b=137},  -- #859289 gray
        mlcomment = {r=133, g=146, b=137},  -- #859289 gray
        keyword1  = {r=230, g=126, b=128},  -- #e67e80 red
        keyword2  = {r=127, g=187, b=179},  -- #7fbbb3 blue
        string    = {r=167, g=192, b=128},  -- #a7c080 green
        number    = {r=214, g=153, b=182},  -- #d699b6 purple
        match     = {r=219, g=188, b=127},  -- #dbbc7f yellow

        -- Functions
        ["function"]      = {r=167, g=192, b=128},  -- #a7c080 green
        function_builtin  = {r=131, g=192, b=146},  -- #83c092 aqua
        function_call     = {r=167, g=192, b=128},  -- #a7c080 green
        function_method   = {r=167, g=192, b=128},  -- #a7c080 green
        function_macro    = {r=131, g=192, b=146},  -- #83c092 aqua

        -- Variables
        variable          = {r=211, g=198, b=170},  -- #d3c6aa fg
        variable_builtin  = {r=230, g=152, b=117},  -- #e69875 orange
        variable_parameter= {r=214, g=153, b=182},  -- #d699b6 purple
        variable_field    = {r=211, g=198, b=170},  -- #d3c6aa fg
        variable_property = {r=211, g=198, b=170},  -- #d3c6aa fg

        -- Keywords
        keyword_control   = {r=230, g=126, b=128},  -- #e67e80 red
        keyword_function  = {r=131, g=192, b=146},  -- #83c092 aqua
        keyword_return    = {r=230, g=126, b=128},  -- #e67e80 red
        keyword_operator  = {r=230, g=152, b=117},  -- #e69875 orange
        keyword_import    = {r=214, g=153, b=182},  -- #d699b6 purple
        keyword_type      = {r=219, g=188, b=127},  -- #dbbc7f yellow
        keyword_modifier  = {r=230, g=126, b=128},  -- #e67e80 red

        -- Strings
        string_escape     = {r=230, g=152, b=117},  -- #e69875 orange
        string_regex      = {r=167, g=192, b=128},  -- #a7c080 green
        string_special    = {r=167, g=192, b=128},  -- #a7c080 green

        -- Numbers
        number_float      = {r=214, g=153, b=182},  -- #d699b6 purple

        -- Literals
        boolean           = {r=214, g=153, b=182},  -- #d699b6 purple
        constant          = {r=214, g=153, b=182},  -- #d699b6 purple
        constant_builtin  = {r=230, g=152, b=117},  -- #e69875 orange

        -- Comments
        comment_doc       = {r=133, g=146, b=137},  -- #859289 gray

        -- Types
        type              = {r=219, g=188, b=127},  -- #dbbc7f yellow
        type_builtin      = {r=219, g=188, b=127},  -- #dbbc7f yellow
        type_parameter    = {r=127, g=187, b=179},  -- #7fbbb3 blue
        type_qualifier    = {r=230, g=126, b=128},  -- #e67e80 red

        -- Operators and punctuation
        operator          = {r=230, g=152, b=117},  -- #e69875 orange
        punctuation       = {r=211, g=198, b=170},  -- #d3c6aa fg
        punctuation_bracket   = {r=211, g=198, b=170},  -- #d3c6aa fg
        punctuation_delimiter = {r=133, g=146, b=137},  -- #859289 gray

        -- Special
        constructor       = {r=219, g=188, b=127},  -- #dbbc7f yellow
        namespace         = {r=127, g=187, b=179},  -- #7fbbb3 blue
        module            = {r=127, g=187, b=179},  -- #7fbbb3 blue
        label             = {r=230, g=152, b=117},  -- #e69875 orange
        tag               = {r=230, g=126, b=128},  -- #e67e80 red
        tag_attribute     = {r=167, g=192, b=128},  -- #a7c080 green
        preprocessor      = {r=214, g=153, b=182},  -- #d699b6 purple

        -- Errors and warnings
        error             = {r=230, g=126, b=128},  -- #e67e80 red
        warning           = {r=219, g=188, b=127},  -- #dbbc7f yellow
    })

    if loki.status then
        loki.status("Everforest theme loaded")
    end
end
