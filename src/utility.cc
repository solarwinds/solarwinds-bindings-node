#include "bindings.h"

/*
Napi::Value Utility::isReady(const Napi::CallbackInfo& info) {
  int status = oboe_is_ready(0);
  return Napi::New(env, status != 0);
}

void Utility::Init(Napi::Object module) {
  Napi::HandleScope scope(env);

  Napi::Object exports = Napi::Object::New(env);
  //Napi::SetMethod(exports, "isReady", Utility::isReady);

  (module).Set(Napi::String::New(env, "Utility"), exports);
}
// */
