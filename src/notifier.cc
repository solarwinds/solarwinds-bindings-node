#include "bindings.h"

//
// initialize a client connection to a unix-domain socket.
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

//
// get the status of the connection
//
//
// these codes are returned by oboe_notifier_status()
//
// OBOE_NOTIFIER_SHUTTING_DOWN -3
// OBOE_NOTIFIER_INITIALIZING -2
// OBOE_NOTIFIER_DISABLED -1
// OBOE_NOTIFIER_OK 0
// OBOE_NOTIFIER_SOCKET_PATH_TOO_LONG 1
// OBOE_NOTIFIER_SOCKET_CREATE 2
// OBOE_NOTIFIER_SOCKET_CONNECT 3
// OBOE_NOTIFIER_SOCKET_WRITE_FULL 4
// OBOE_NOTIFIER_SOCKET_WRITE_ERROR 5
// OBOE_NOTIFIER_SHUTDOWN_TIMED_OUT 6

Napi::Value oboeNotifierStatus(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), oboe_notifier_status());
}

//
// stop the client from connecting to the socket path. if block
// is true it will not return until the disconnect is complete.
//
// return the status.
//
// normal status is -3 (shutting down) if non-blocking or -1 (disabled)
// if blocking.
//
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

  module.Set("init", Napi::Function::New(env, oboeNotifierInit));
  module.Set("status", Napi::Function::New(env, oboeNotifierStatus));
  module.Set("stop", Napi::Function::New(env, oboeNotifierStop));

  exports.Set("Notifier", module);

  return exports;
}

}  // namespace Notifier
