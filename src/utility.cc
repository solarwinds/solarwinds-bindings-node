#include "bindings.h"

/*
NAN_METHOD(Utility::isReady) {
  int status = oboe_is_ready();
  info.GetReturnValue().Set(Nan::New(status != 0));
}

NAN_METHOD(Utility::setFlags) {
    if (info.Length() !== 2 || !info[0]->IsObject() || !info[1]->IsNumber()) {
        return Nan::ThrowTypeError("setFlags needs a buffer and the flags");
    }
    char* buffer = (char*) node::Buffer::Data(info[0]->ToObject());
}



NAN_METHOD(Utility::getBuffer) {
    char* initial = new char[30]();
    initial[29] = 0x01;
    //Nan::MaybeLocal<v8::Object>buffer = Nan::NewBuffer(initial, 30);
    //info.GetReturnValue().Set(Nan::New(buffer));
    //info.GetReturnValue().Set(Nan::New(buffer).ToLocalChecked());
    info.GetReturnValue().Set(Nan::NewBuffer(initial, 30).ToLocalChecked());
}

void Utility::Init(v8::Local<v8::Object> module) {
  Nan::HandleScope scope;

  v8::Local<v8::Object> exports = Nan::New<v8::Object>();
  //Nan::SetMethod(exports, "isReady", Utility::isReady);
  Nan::SetMethod(exports, "getBuffer", Utility::getBuffer);

  Nan::Set(module, Nan::New("Utility").ToLocalChecked(), exports);
}
// */
