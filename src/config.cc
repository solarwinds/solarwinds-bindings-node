#ifndef OBOE_CONFIG_H
#define OBOE_CONFIG_H

#include "bindings.h"

Napi::Value getVersionString(const Napi::CallbackInfo& info) {
  const char* version = oboe_config_get_version_string();
  return Napi::String::New(info.Env(), version);
}

//
// put Config in a separate JavaScript namespace.
//
namespace Config {

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Object module = Napi::Object::New(env);
  module.Set("getVersionString", Napi::Function::New(env, getVersionString));

  exports.Set("Config", module);

  return exports;
}

} // end namespace Config

#endif
