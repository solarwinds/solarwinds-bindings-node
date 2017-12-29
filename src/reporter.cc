#include "bindings.h"

Nan::Persistent<v8::Function> Reporter::constructor;

// Construct with an address and port to report to
Reporter::Reporter() {
  // this is a shell for until it can setup new reporters
  connected = true;
  host = "";
  port = "443";
  const char *p = getenv("APPOPTICS_REPORTER");
  // handle null, thanks.
  if (p == 0 || p[0] == '\0') {
      p = "";
  }
  protocol = p;
  initDone = false;
  channel = OBOE_SEND_STATUS;
}

Reporter::~Reporter() {
}


v8::Local<v8::Object> Reporter::NewInstance() {
    Nan::EscapableHandleScope scope;

    const unsigned argc = 0;
    v8::Local<v8::Value> argv[argc] = {};
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);

    return scope.Escape(instance);
  }

int Reporter::send_event(oboe_metadata_t* meta, oboe_event_t* event) {
  // some way to check that connection is not available (ssl)?
  if ( ! connected) {
    // return some unique code to distinguish from liboboe codes.
    return -111;
  }

  return oboe_event_send(OBOE_SEND_EVENT, event, meta);
}

int Reporter::send_status(oboe_metadata_t* meta, oboe_event_t* event) {
    if (!connected) {
        return -111;
    }
    return oboe_event_send(OBOE_SEND_STATUS, event, meta);
}

NAN_SETTER(Reporter::setAddress) {
  if ( ! value->IsString()) {
    return Nan::ThrowTypeError("Address must be a string");
  }

  Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());

  std::string s = *Nan::Utf8String(value);
  char* host = strdup(s.substr(0, s.find(":")).c_str());
  char* port = strdup(s.substr(s.find(":") + 1).c_str());
  if (host == NULL || port == NULL) {
    return Nan::ThrowError("Invalid address string");
  }

  //self->connected = false;
  self->host = host;
  self->port = port;
  free(host);
  free(port);
}
NAN_GETTER(Reporter::getAddress) {
  Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
  std::string host = self->host;
  std::string port = self->port;
  std::string address = host + ":" + port;
  info.GetReturnValue().Set(Nan::New(address).ToLocalChecked());
}

NAN_SETTER(Reporter::setHost) {
  if ( ! value->IsString()) {
    return Nan::ThrowTypeError("host must be a string");
  }

  Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
  self->host = *Nan::Utf8String(value->ToString());
  //self->connected = false;
}
NAN_GETTER(Reporter::getHost) {
  Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
  info.GetReturnValue().Set(Nan::New(self->host).ToLocalChecked());
}

NAN_SETTER(Reporter::setPort) {
  if ( ! value->IsString() && ! value->IsNumber()) {
    return Nan::ThrowTypeError("port must be a string");
  }

  Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
  self->port = *Nan::Utf8String(value->ToString());
  //self->connected = false;
}
NAN_GETTER(Reporter::getPort) {
  Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
  info.GetReturnValue().Set(Nan::New(self->port).ToLocalChecked());
}

// TODO remove initDone altogether.
NAN_SETTER(Reporter::setInitDone) {
    /* let it be truthy of falsey, no need to be boolean.
    if (!value->IsBoolean()) {
        return Nan::ThrowTypeError("init value must be boolean");
    }
    */
    Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
    self->initDone = value->BooleanValue();

    self->channel = self->initDone ? OBOE_SEND_EVENT : OBOE_SEND_STATUS;
}
NAN_GETTER(Reporter::getInitDone) {
    Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
    info.GetReturnValue().Set(Nan::New(self->initDone));
}

NAN_GETTER(Reporter::getChannel) {
    Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
    info.GetReturnValue().Set(Nan::New(self->channel));
}


// Send an event to the reporter
NAN_METHOD(Reporter::sendReport) {
  if (info.Length() < 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }
  if (!Event::isEvent(info[0])) {
    return Nan::ThrowTypeError("Must supply an event instance");
  }

  Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
  Event* event = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());

  oboe_metadata_t *md;
  if (info.Length() == 2 && info[1]->IsObject()) {
    Metadata* metadata = Nan::ObjectWrap::Unwrap<Metadata>(info[1]->ToObject());
    md = &metadata->metadata;
  } else {
    md = oboe_context_get();
  }

  int status = self->send_event(md, &event->event);
  info.GetReturnValue().Set(Nan::New(status));
}

NAN_METHOD(Reporter::sendStatus) {
    if (info.Length() < 1) {
        return Nan::ThrowError("Reporter::sendStatus - wrong number of arguments");
    }
    if (!Event::isEvent(info[0])) {
        return Nan::ThrowError("Reporter::sendStatus - requires an event instance");
    }

    Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
    Event* event = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());

    // fetch the (possibly new) context
    oboe_metadata_t* md;
    if (info.Length() >= 2 && Metadata::isMetadata(info[1])) {
        md = &Nan::ObjectWrap::Unwrap<Metadata>(info[1]->ToObject())->metadata;
    } else {
        md = oboe_context_get();
    }

    int status = self->send_status(md, &event->event);
    info.GetReturnValue().Set(Nan::New(status));
}

// Creates a new Javascript instance
NAN_METHOD(Reporter::New) {
  if (!info.IsConstructCall()) {
    return Nan::ThrowError("Reporter() must be called as a constructor");
  }
  Reporter* reporter = new Reporter();
  reporter->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

// Wrap the C++ object so V8 can understand it
void Reporter::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> ctor = Nan::New<v8::FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(Nan::New("Reporter").ToLocalChecked());

  // Assign host/port to change reporter target
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetAccessor(proto, Nan::New("address").ToLocalChecked(), getAddress, setAddress);
  Nan::SetAccessor(proto, Nan::New("host").ToLocalChecked(), getHost, setHost);
  Nan::SetAccessor(proto, Nan::New("port").ToLocalChecked(), getPort, setPort);
  Nan::SetAccessor(proto, Nan::New("initDone").ToLocalChecked(), getInitDone, setInitDone);
  Nan::SetAccessor(proto, Nan::New("channel").ToLocalChecked(), getChannel);

  // Prototype
  Nan::SetPrototypeMethod(ctor, "sendReport", Reporter::sendReport);
  Nan::SetPrototypeMethod(ctor, "sendStatus", Reporter::sendStatus);

  constructor.Reset(ctor->GetFunction());
  Nan::Set(exports, Nan::New("Reporter").ToLocalChecked(), ctor->GetFunction());
}
