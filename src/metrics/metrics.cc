#include <napi.h>
#include "gc.h"
#include "eventloop.h"
#include "process.h"

namespace ao { namespace metrics {

  Napi::Value start(const Napi::CallbackInfo& info) {
    int status = 0;
    if (!gc::start()) status |= 0x1;
    if (!eventloop::start()) status |= 0x2;
    if (!process::start()) status |= 0x4;

    return Napi::Number::New(info.Env(), status);
  }

  Napi::Value stop(const Napi::CallbackInfo& info) {
    int status = 0;
    if (!gc::stop()) status |= 0x1;
    if (!eventloop::stop()) status |= 0x2;
    if (!process::stop()) status |= 0x4;

    return Napi::Number::New(info.Env(), status);
  }

  Napi::Value getMetrics(const Napi::CallbackInfo& info) {
    auto metrics = Napi::Object::New(info.Env());

    auto obj = Napi::Object::New(info.Env());
    if (gc::getInterval(obj)) {
      metrics.Set("gc", obj);
    }

    obj = Napi::Object::New(info.Env());
    if (eventloop::getInterval(obj)) {
      metrics.Set("eventloop", obj);
    }

    obj = Napi::Object::New(info.Env());
    if (process::getInterval(obj)) {
      metrics.Set("process", obj);
    }

    return metrics;
  }

  Napi::Object init(Napi::Env env, Napi::Object exports) {
    exports.Set("start", Napi::Function::New(env, start));
    exports.Set("stop", Napi::Function::New(env, stop));
    exports.Set("getMetrics", Napi::Function::New(env, getMetrics));

    return exports;
  }

  NODE_API_MODULE(metrics, init)
}} // namespace ao::metrics
