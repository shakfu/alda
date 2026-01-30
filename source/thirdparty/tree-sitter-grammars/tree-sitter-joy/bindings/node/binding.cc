#include <napi.h>

typedef struct TSLanguage TSLanguage;

extern "C" TSLanguage *tree_sitter_joy();

// "tree-sitter", "currentLanguage" hance the new [Symbol.for("tree-sitter.currentLanguage")]
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports["name"] = Napi::String::New(env, "joy");
  auto language = Napi::External<TSLanguage>::New(env, tree_sitter_joy());
  exports["language"] = language;
  return exports;
}

NODE_API_MODULE(tree_sitter_joy_binding, Init)
