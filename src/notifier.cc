#include "bindings.h"

//
// initialize a client connection to
//
Napi::Value oboeNotifierInit(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "socket path must be a string")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  std::string path = info[0].ToString();

  const int status = oboe_notifier_init(path.c_str());

  return Napi::Number::New(env, status);
}

Napi::Value oboeNotifierStatus(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), oboe_notifier_status());
}

Napi::Value oboeNotifierStop(const Napi::CallbackInfo& info) {
  bool block = false;
  if (info.Length() > 0) {
    block = info[0].ToBoolean().Value();
  }
  int status = oboe_notifier_stop(block);
  return Napi::Number::New(info.Env(), status);
}

//
// Initialize the module.
//
namespace Notifier {

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Object module = Napi::Object::New(env);

  module.Set("Init", Napi::Function::New(env, oboeNotifierInit));
  module.Set("Status", Napi::Function::New(env, oboeNotifierStatus));
  module.Set("Stop", Napi::Function::New(env, oboeNotifierStop));

  exports.Set("Notifier", module);

  return exports;
}

}  // namespace Notifier
