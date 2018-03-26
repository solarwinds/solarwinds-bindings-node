#include "bindings.h"

Nan::Persistent<v8::FunctionTemplate> Reporter::constructor;

// Construct with an address and port to report to
Reporter::Reporter() {
  // this is a shell for until it can setup new reporters
  connected = true;
  host = "";
  port = "443";
  const char *p = getenv("APPOPTICS_REPORTER");
  if (p == 0 || p[0] == '\0') {
      p = "";
  }
  protocol = p;
}

Reporter::~Reporter() {
}


v8::Local<v8::Object> Reporter::NewInstance() {
    Nan::EscapableHandleScope scope;

    const unsigned argc = 0;
    v8::Local<v8::Value> argv[argc] = {};
    v8::Local<v8::Function> cons = Nan::New<v8::FunctionTemplate>(constructor)->GetFunction();
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

int Reporter::send_event_x(Nan::NAN_METHOD_ARGS_TYPE info, int channel) {
    //Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
    // TODO BAM this requires that the event argument is correct.
    Event* event = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());

    // either get the metadata passed in or grab it from oboe.
    oboe_metadata_t *md;
    if (info.Length() >= 2 && Metadata::isMetadata(info[1])) {
        md = &Nan::ObjectWrap::Unwrap<Metadata>(info[1]->ToObject())->metadata;
    } else {
        md = oboe_context_get();
    }

    return oboe_event_send(channel, &event->event, md);
}


// Send an event to the reporter
NAN_METHOD(Reporter::sendReport) {
    if (info.Length() < 1 || !Event::isEvent(info[0])) {
        return Nan::ThrowTypeError("Reporter::sendReport() - Must supply an event instance");
    }
    Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
    int status = self->send_event_x(info, OBOE_SEND_EVENT);

    /*
    Event* event = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());

    oboe_metadata_t *md;
    if (info.Length() >= 2 && Metadata::isMetadata(info[1])) {
        md = &Nan::ObjectWrap::Unwrap<Metadata>(info[1]->ToObject())->metadata;
    } else {
        md = oboe_context_get();
    }

    int status = self->send_event(md, &event->event);
    // */
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
    int status = self->send_event_x(info, OBOE_SEND_STATUS);

    /*
    Event* event = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());

    // fetch the (possibly new) context
    oboe_metadata_t* md;
    if (info.Length() >= 2 && Metadata::isMetadata(info[1])) {
        md = &Nan::ObjectWrap::Unwrap<Metadata>(info[1]->ToObject())->metadata;
    } else {
        md = oboe_context_get();
    }

    int status = self->send_status(md, &event->event);
    // */
    info.GetReturnValue().Set(Nan::New(status));
}

bool http_span_args_are_good(Nan::NAN_METHOD_ARGS_TYPE info) {
    // anything in JavaScript has a boolean value so no need to check.
    return info.Length() == 5 && info[0]->IsString() && info[1]->IsNumber() &&
        info[2]->IsInt32() && info[3]->IsString(); // && info[4]->IsBoolean();
}

NAN_METHOD(Reporter::sendHttpSpanName) {
    // transaction name (or req.base_url) {char *} "name"
    // req.base_url (or transaction name) {char *} "url"
    // duration - microseconds {int64}             "duration"
    // status - HTTP status code {int}             "status"
    // req.request_method - HTTP method {char *}   "method"
    // error - 1 if transaction contains error else 0 {int}  "error"
    if (!http_span_args_are_good(info)) {
        info.GetReturnValue().Set(Nan::New(false));
        return;
    }

    std::string name = *Nan::Utf8String(info[0]);
    char *url = NULL;
    // Number.MAX_SAFE_INTEGER is big enough for any reasonable transaction time.
    // max_safe_seconds = MAX_SAFE_INTEGER / 1000000 microseconds
    // max_safe_days = MAX_SAFE_SECONDS / 86400 seconds
    // max_safe_days > 100000. Seems long enough to me.
    int64_t duration = info[1]->IntegerValue();
    int status = info[2]->IntegerValue();
    std::string method = *Nan::Utf8String(info[3]);
    int error = info[4]->BooleanValue();

    /*
    std::cout << "name: " << name
        << " duration: " << duration
        << " status: " << status
        << " method: " << method
        << " error: " << error << std::endl;
    // */

    oboe_http_span(name.c_str(), url, duration, status, method.c_str(), error);

    info.GetReturnValue().Set(Nan::New(true));
}

NAN_METHOD(Reporter::sendHttpSpanUrl) {
    if (!http_span_args_are_good(info)) {
        info.GetReturnValue().Set(Nan::New(false));
        return;
    }

    char* name = NULL;
    std::string url = *Nan::Utf8String(info[0]);
    // see comment in sendHttpSpanName()
    int64_t duration = info[1]->IntegerValue();
    int status = info[2]->IntegerValue();
    std::string method = *Nan::Utf8String(info[3]);
    int error = info[4]->BooleanValue();

    oboe_http_span(name, url.c_str(), duration, status, method.c_str(), error);

    info.GetReturnValue().Set(Nan::New(true));
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
    v8::Local<v8::FunctionTemplate> ctor = Nan::New<v8::FunctionTemplate>(Reporter::New);
    constructor.Reset(ctor);
    ctor->SetClassName(Nan::New("Reporter").ToLocalChecked());
    // internal field count and accessors must be set on the instance template.
    v8::Local<v8::ObjectTemplate> instance = ctor->InstanceTemplate();
    instance->SetInternalFieldCount(1);

    // Assign host/port to change reporter target
    Nan::SetAccessor(instance, Nan::New("address").ToLocalChecked(), getAddress, setAddress);
    Nan::SetAccessor(instance, Nan::New("host").ToLocalChecked(), getHost, setHost);
    Nan::SetAccessor(instance, Nan::New("port").ToLocalChecked(), getPort, setPort);

    // Prototype
    Nan::SetPrototypeMethod(ctor, "sendReport", Reporter::sendReport);
    Nan::SetPrototypeMethod(ctor, "sendStatus", Reporter::sendStatus);
    Nan::SetPrototypeMethod(ctor, "sendHttpSpanName", Reporter::sendHttpSpanName);
    Nan::SetPrototypeMethod(ctor, "sendHttpSpanUrl", Reporter::sendHttpSpanUrl);

    Nan::Set(exports, Nan::New("Reporter").ToLocalChecked(), ctor->GetFunction());
}

