#include <nan.h>
#include "gc.h"
#include "eventloop.h"
#include "process.h"

namespace ao::metrics {
  using namespace v8;

  NAN_METHOD(start) {
    int status = 0;
    if (!gc::start()) status |= 0x1;
    if (!eventloop::start()) status |= 0x2;
    if (!process::start()) status |= 0x4;

    info.GetReturnValue().Set(status);
  }

  NAN_METHOD(stop) {
    int status = 0;
    if (!gc::stop()) status |= 0x1;
    if (!eventloop::stop()) status |= 0x2;
    if (!process::stop()) status |= 0x4;

    info.GetReturnValue().Set(status);
  }

  NAN_METHOD(getMetrics) {
    Local<Object> metrics = Nan::New<Object>();

    Local<Object> obj = Nan::New<Object>();
    if (gc::getInterval(obj)) {
      Nan::Set(metrics, Nan::New("gc").ToLocalChecked(), obj);
    }

    obj = Nan::New<Object>();
    if (eventloop::getInterval(obj)) {
      Nan::Set(metrics, Nan::New("eventloop").ToLocalChecked(), obj);
    }

    obj = Nan::New<Object>();
    if (process::getInterval(obj)) {
      Nan::Set(metrics, Nan::New("process").ToLocalChecked(), obj);
    }

    info.GetReturnValue().Set(metrics);
  }

  NAN_MODULE_INIT(init) {
    Nan::HandleScope scope;

    Nan::Set(target, Nan::New("start").ToLocalChecked(),
        Nan::GetFunction(Nan::New<FunctionTemplate>(start)).ToLocalChecked());
    Nan::Set(target, Nan::New("stop").ToLocalChecked(),
        Nan::GetFunction(Nan::New<FunctionTemplate>(stop)).ToLocalChecked());
    Nan::Set(target, Nan::New("getMetrics").ToLocalChecked(),
        Nan::GetFunction(Nan::New<FunctionTemplate>(getMetrics)).ToLocalChecked());
  }

  NODE_MODULE(metrics, init)
}
