#include "bindings.h"

/*
NAN_METHOD(Utility::isReady) {
  int status = oboe_is_ready(0);
  info.GetReturnValue().Set(Nan::New(status != 0));
}

void Utility::Init(v8::Local<v8::Object> module) {
  Nan::HandleScope scope;

  v8::Local<v8::Object> exports = Nan::New<v8::Object>();
  //Nan::SetMethod(exports, "isReady", Utility::isReady);

  Nan::Set(module, Nan::New("Utility").ToLocalChecked(), exports);
}
// */
