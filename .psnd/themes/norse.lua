-- Norse Theme for Loki Editor
-- Based on the C 'nord' theme with Nord's actual RGB palette
-- https://www.nordtheme.com/
--
-- Palette:
--   Polar Night: #2e3440, #3b4252, #434c5e, #4c566a
--   Snow Storm:  #d8dee9, #e5e9f0, #eceff4
--   Frost:       #8fbcbb (teal), #88c0d0 (cyan), #81a1c1 (light blue), #5e81ac (blue)
--   Aurora:      #bf616a (red), #d08770 (orange), #ebcb8b (yellow),
--                #a3be8c (green), #b48ead (purple)

return function()
    loki.set_theme({
        -- Base types (0-8)
        normal    = {r=216, g=222, b=233},  -- #d8dee9 snow storm
        nonprint  = {r=76,  g=86,  b=106},  -- #4c566a polar night 3
        comment   = {r=76,  g=86,  b=106},  -- #4c566a polar night 3
        mlcomment = {r=76,  g=86,  b=106},  -- #4c566a polar night 3
        keyword1  = {r=180, g=142, b=173},  -- #b48ead purple
        keyword2  = {r=136, g=192, b=208},  -- #88c0d0 cyan
        string    = {r=163, g=190, b=140},  -- #a3be8c green
        number    = {r=180, g=142, b=173},  -- #b48ead purple
        match     = {r=235, g=203, b=139},  -- #ebcb8b yellow

        -- Functions
        ["function"]      = {r=129, g=161, b=193},  -- #81a1c1 light blue
        function_builtin  = {r=136, g=192, b=208},  -- #88c0d0 cyan
        function_call     = {r=129, g=161, b=193},  -- #81a1c1 light blue
        function_method   = {r=129, g=161, b=193},  -- #81a1c1 light blue
        function_macro    = {r=143, g=188, b=187},  -- #8fbcbb teal

        -- Variables
        variable          = {r=216, g=222, b=233},  -- #d8dee9 snow storm
        variable_builtin  = {r=180, g=142, b=173},  -- #b48ead purple
        variable_parameter= {r=216, g=222, b=233},  -- #d8dee9 snow storm
        variable_field    = {r=216, g=222, b=233},  -- #d8dee9 snow storm
        variable_property = {r=216, g=222, b=233},  -- #d8dee9 snow storm

        -- Keywords
        keyword_control   = {r=180, g=142, b=173},  -- #b48ead purple
        keyword_function  = {r=180, g=142, b=173},  -- #b48ead purple
        keyword_return    = {r=180, g=142, b=173},  -- #b48ead purple
        keyword_operator  = {r=180, g=142, b=173},  -- #b48ead purple
        keyword_import    = {r=180, g=142, b=173},  -- #b48ead purple
        keyword_type      = {r=94,  g=129, b=172},  -- #5e81ac blue
        keyword_modifier  = {r=180, g=142, b=173},  -- #b48ead purple

        -- Strings
        string_escape     = {r=235, g=203, b=139},  -- #ebcb8b yellow
        string_regex      = {r=208, g=135, b=112},  -- #d08770 orange
        string_special    = {r=163, g=190, b=140},  -- #a3be8c green

        -- Numbers
        number_float      = {r=180, g=142, b=173},  -- #b48ead purple

        -- Literals
        boolean           = {r=180, g=142, b=173},  -- #b48ead purple
        constant          = {r=180, g=142, b=173},  -- #b48ead purple
        constant_builtin  = {r=180, g=142, b=173},  -- #b48ead purple

        -- Comments
        comment_doc       = {r=76,  g=86,  b=106},  -- #4c566a polar night 3

        -- Types
        type              = {r=94,  g=129, b=172},  -- #5e81ac blue
        type_builtin      = {r=94,  g=129, b=172},  -- #5e81ac blue
        type_parameter    = {r=94,  g=129, b=172},  -- #5e81ac blue
        type_qualifier    = {r=180, g=142, b=173},  -- #b48ead purple

        -- Operators and punctuation
        operator          = {r=136, g=192, b=208},  -- #88c0d0 cyan
        punctuation       = {r=216, g=222, b=233},  -- #d8dee9 snow storm
        punctuation_bracket   = {r=216, g=222, b=233},  -- #d8dee9 snow storm
        punctuation_delimiter = {r=216, g=222, b=233},  -- #d8dee9 snow storm

        -- Special
        constructor       = {r=94,  g=129, b=172},  -- #5e81ac blue
        namespace         = {r=94,  g=129, b=172},  -- #5e81ac blue
        module            = {r=94,  g=129, b=172},  -- #5e81ac blue
        label             = {r=235, g=203, b=139},  -- #ebcb8b yellow
        tag               = {r=180, g=142, b=173},  -- #b48ead purple
        tag_attribute     = {r=129, g=161, b=193},  -- #81a1c1 light blue
        preprocessor      = {r=143, g=188, b=187},  -- #8fbcbb teal

        -- Errors and warnings
        error             = {r=191, g=97,  b=106},  -- #bf616a red
        warning           = {r=208, g=135, b=112},  -- #d08770 orange
    })

    if loki.status then
        loki.status("Norse theme loaded")
    end
end
