; Csound syntax highlighting queries

; Comments
(comment) @comment
(block_comment) @comment

; XML tags
(xml_tag) @keyword.directive

; Header variables
(header_var) @variable.builtin

; Keywords
(instrument_keyword) @keyword.function
(opcode_keyword) @keyword.function
(block_keyword) @keyword
(control_keyword) @keyword.control
(type_keyword) @keyword

; Variables
(variable) @variable
(pfield) @variable.parameter

; Numbers
(number) @number

; Strings
(string) @string

; Operators
(operator) @operator

; Identifiers (opcodes, labels)
(identifier) @function
