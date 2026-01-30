{
  "targets": [
    {
      "target_name": "tree_sitter_joy_binding",
      "dependencies": [
        "<!(node -p \"require('node-addon-api').targets\"):node_addon_api_except",
      ],
      "include_dirs": [
        "src",
      ],
      "sources": [
        "bindings/node/binding.cc",
        "src/parser.c",
        "src/scanner.c",
      ],
      "cflags_c": [
        "-std=c11",
      ],
      "cflags_cc": [
        "-std=c++14",
      ],
      "conditions": [
        ["OS=='mac'", {
          "xcode_settings": {
            "CLANG_CXX_LANGUAGE_STANDARD": "c++14",
            "MACOSX_DEPLOYMENT_TARGET": "10.15",
          },
        }],
      ],
    }
  ]
}
