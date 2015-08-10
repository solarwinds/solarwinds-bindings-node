#include "../bindings.h"

Nan::Persistent<v8::Function> UdpReporter::constructor;

// Construct with an address and port to report to
UdpReporter::UdpReporter() {
  connected = false;
  host = "localhost";
  port = "7831";
}

// Remember to cleanup the udp reporter struct when garbage collected
UdpReporter::~UdpReporter() {
  if (&reporter) {
    oboe_reporter_destroy(&reporter);
  }
}

int UdpReporter::send(oboe_metadata_t* meta, oboe_event_t* event) {
  if ( ! connected) {
    int status = oboe_reporter_udp_init(&reporter, host.c_str(), port.c_str());
    if (status != 0) {
      return status;
    }

    connected = true;
  }

  return oboe_reporter_send(&reporter, meta, event);
}

NAN_SETTER(UdpReporter::setAddress) {
  if ( ! value->IsString()) {
    return Nan::ThrowTypeError("Address must be a string");
  }

  UdpReporter* self = Nan::ObjectWrap::Unwrap<UdpReporter>(info.This());

  std::string s = *Nan::Utf8String(value);
  char* host = strdup(s.substr(0, s.find(":")).c_str());
  char* port = strdup(s.substr(s.find(":") + 1).c_str());
  if (host == NULL || port == NULL) {
    return Nan::ThrowError("Invalid address string");
  }

  self->connected = false;
  self->host = host;
  self->port = port;
}
NAN_GETTER(UdpReporter::getAddress) {
  UdpReporter* self = Nan::ObjectWrap::Unwrap<UdpReporter>(info.This());
  std::string host = self->host;
  std::string port = self->port;
  std::string address = host + ":" + port;
  info.GetReturnValue().Set(Nan::New(address).ToLocalChecked());
}

NAN_SETTER(UdpReporter::setHost) {
  if ( ! value->IsString()) {
    return Nan::ThrowTypeError("host must be a string");
  }

  UdpReporter* self = Nan::ObjectWrap::Unwrap<UdpReporter>(info.This());
  Nan::Utf8String val(value->ToString());

  self->connected = false;
  self->host = *val;
}
NAN_GETTER(UdpReporter::getHost) {
  UdpReporter* self = Nan::ObjectWrap::Unwrap<UdpReporter>(info.This());
  info.GetReturnValue().Set(Nan::New(self->host).ToLocalChecked());
}

NAN_SETTER(UdpReporter::setPort) {
  if ( ! value->IsString() && ! value->IsNumber()) {
    return Nan::ThrowTypeError("port must be a string");
  }

  UdpReporter* self = Nan::ObjectWrap::Unwrap<UdpReporter>(info.This());
  Nan::Utf8String val(value->ToString());

  self->connected = false;
  self->port = *val;
}
NAN_GETTER(UdpReporter::getPort) {
  UdpReporter* self = Nan::ObjectWrap::Unwrap<UdpReporter>(info.This());
  info.GetReturnValue().Set(Nan::New(self->port).ToLocalChecked());
}

// Transform a string back into a metadata instance
NAN_METHOD(UdpReporter::sendReport) {
  if (info.Length() < 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }
  if (!info[0]->IsObject()) {
    return Nan::ThrowTypeError("Must supply an event instance");
  }

  UdpReporter* self = Nan::ObjectWrap::Unwrap<UdpReporter>(info.This());
  Event* event = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());

  oboe_metadata_t *md;
  if (info.Length() == 2 && info[1]->IsObject()) {
    Metadata* metadata = Nan::ObjectWrap::Unwrap<Metadata>(info[1]->ToObject());
    md = &metadata->metadata;
  } else {
    md = OboeContext::get();
  }

  int status = self->send(md, &event->event);
  info.GetReturnValue().Set(Nan::New(status >= 0));
}

// Creates a new Javascript instance
NAN_METHOD(UdpReporter::New) {
  if (!info.IsConstructCall()) {
    return Nan::ThrowError("UdpReporter() must be called as a constructor");
  }
  UdpReporter* reporter = new UdpReporter();
  reporter->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

// Wrap the C++ object so V8 can understand it
void UdpReporter::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> ctor = Nan::New<v8::FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(Nan::New("UdpReporter").ToLocalChecked());

  // Assign host/port to change reporter target
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetAccessor(proto, Nan::New("address").ToLocalChecked(), getAddress, setAddress);
  Nan::SetAccessor(proto, Nan::New("host").ToLocalChecked(), getHost, setHost);
  Nan::SetAccessor(proto, Nan::New("port").ToLocalChecked(), getPort, setPort);

  // Prototype
  Nan::SetPrototypeMethod(ctor, "sendReport", UdpReporter::sendReport);

  constructor.Reset(ctor->GetFunction());
  Nan::Set(exports, Nan::New("UdpReporter").ToLocalChecked(), ctor->GetFunction());
}
