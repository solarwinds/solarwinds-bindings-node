#ifndef OBOE_CONFIG_H
#define OBOE_CONFIG_H

#include "bindings.h"

Napi::Value getRevision(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), oboe_config_get_revision());
}

Napi::Value getVersion(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), oboe_config_get_version());
}

Napi::Value checkVersion(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() != 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
    Napi::TypeError::New(env, "invalid arguments").ThrowAsJavaScriptException();
    return env.Null();
  }

  int version = info[0].As<Napi::Number>().Int64Value();
  int revision = info[1].As<Napi::Number>().Int64Value();

  bool status = oboe_config_check_version(version, revision) != 0;

  return Napi::Boolean::New(env, status);
}

Napi::Value getVersionString(const Napi::CallbackInfo& info) {
  const char* version = oboe_config_get_version_string();
  return Napi::String::New(info.Env(), version);
}

//
// put Config in a separate namespace and module.
//
namespace Config {

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Object module = Napi::Object::New(env);
  module.Set("getVersion", Napi::Function::New(env, getVersion));
  module.Set("getRevision", Napi::Function::New(env, getRevision));
  module.Set("checkVersion", Napi::Function::New(env, checkVersion));
  module.Set("getVersionString", Napi::Function::New(env, getVersionString));

  exports.Set("Config", module);

  return exports;
}

} // end namespace Config

/*
Napi::Value Config::getRevision(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), oboe_config_get_revision());
}

Napi::Value Config::getVersion(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), oboe_config_get_version());
}

Napi::Value Config::checkVersion(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() != 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
    Napi::TypeError::New(env, "invalid arguments").ThrowAsJavaScriptException();
    return env.Null();
  }

  int version = info[0].As<Napi::Number>().Int64Value();
  int revision = info[1].As<Napi::Number>().Int64Value();

  bool status = oboe_config_check_version(version, revision) != 0;

  return Napi::Boolean::New(env, status);
}

Napi::Value Config::getVersionString(const Napi::CallbackInfo& info) {
    const char* version = oboe_config_get_version_string();
    return Napi::String::New(info.Env(), version);
}

Napi::Object Config::Init(Napi::Env env, Napi::Object module) {
  Napi::HandleScope scope(env);

  Napi::Object exports = Napi::Object::New(env);
  exports.Set(Napi::String::New(env, "getVersion"), &Config::getVersion);
  exports.Set(Napi::String::New(env, "getRevision"), &Config::getRevision);
  exports.Set(Napi::String::New(env, "checkVersion"), &Config::checkVersion);
  exports.Set(Napi::String::New(env, "getVersionString"), &Config::getVersionString);

  module.Set(Napi::String::New(env, "Config"), exports);

  return module;
}
// */

#endif
