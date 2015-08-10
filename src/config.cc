#ifndef OBOE_CONFIG_H
#define OBOE_CONFIG_H

#include "bindings.h"

NAN_METHOD(Config::getRevision) {
  info.GetReturnValue().Set(Nan::New(oboe_config_get_revision()));
}

NAN_METHOD(Config::getVersion) {
  info.GetReturnValue().Set(Nan::New(oboe_config_get_version()));
}

NAN_METHOD(Config::checkVersion) {
  if (info.Length() != 2) {
    return Nan::ThrowError("Wrong number of arguments");
  }

  if (!info[0]->IsNumber() || !info[1]->IsNumber()) {
    return Nan::ThrowTypeError("Values must be numbers");
  }

  int version = info[0]->NumberValue();
  int revision = info[1]->NumberValue();

  bool status = oboe_config_check_version(version, revision) != 0;

  info.GetReturnValue().Set(Nan::New(status));
}

void Config::Init(v8::Local<v8::Object> module) {
  Nan::HandleScope scope;

  v8::Local<v8::Object> exports = Nan::New<v8::Object>();
  Nan::SetMethod(exports, "getVersion", Config::getVersion);
  Nan::SetMethod(exports, "getRevision", Config::getRevision);
  Nan::SetMethod(exports, "checkVersion", Config::checkVersion);

  Nan::Set(module, Nan::New("Config").ToLocalChecked(), exports);
}

#endif
