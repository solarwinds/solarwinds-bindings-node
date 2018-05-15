#include "bindings.h"

Nan::Persistent<v8::FunctionTemplate> Reporter::constructor;

// Construct with an address and port to report to
Reporter::Reporter() {
  // this is a shell for now that oboe implements the reporter
}

Reporter::~Reporter() {
}


v8::Local<v8::Object> Reporter::NewInstance() {
    Nan::EscapableHandleScope scope;

    const unsigned argc = 0;
    v8::Local<v8::Value> argv[argc] = {};
    v8::Local<v8::Function> cons = Nan::New<v8::FunctionTemplate>(constructor)->GetFunction();
    //v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);
    v8::Local<v8::Object> instance = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

    return scope.Escape(instance);
  }

int Reporter::send_event_x(Nan::NAN_METHOD_ARGS_TYPE info, int channel) {
    // info.This() is not a Reporter in this case - it's been passed from
    // a C++ function using that function's info. As this is called only
    // from C++ there is no type checking done.
    // TODO verify that info is an event object or consider moving it
    // out of Reporter altogether.
    Event* event = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());

    // either get the metadata passed in or grab it from oboe.
    oboe_metadata_t *md;
    if (info.Length() >= 2 && Metadata::isMetadata(info[1])) {
        md = &Nan::ObjectWrap::Unwrap<Metadata>(info[1]->ToObject())->metadata;
    } else {
        md = oboe_context_get();
    }

    int status = oboe_event_send(channel, &event->event, md);

    return status;
}

// Check to see if oboe is ready to issue sampling decisions.
// Returns true if oboe is ready else numeric error code.
NAN_METHOD(Reporter::isReadyToSample) {
    int ms = 0;
    if (info[0]->IsNumber()) {
        ms = info[0]->IntegerValue();
    }
    int status;
    status = oboe_is_ready(ms);

    // UNKNOWN 0
    // OK 1
    // TRY_LATER 2
    // LIMIT_EXCEEDED 3
    // INVALID_API_KEY 4
    // CONNECT_ERROR 5
    info.GetReturnValue().Set(Nan::New(status));
}


// Send an event to the reporter
NAN_METHOD(Reporter::sendReport) {
    if (info.Length() < 1 || !Event::isEvent(info[0])) {
        return Nan::ThrowTypeError("Reporter::sendReport() - Must supply an event instance");
    }
    Reporter* self = Nan::ObjectWrap::Unwrap<Reporter>(info.This());
    int status = self->send_event_x(info, OBOE_SEND_EVENT);

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

    info.GetReturnValue().Set(Nan::New(status));
}

// These will be the string object keys for the object that
// sendHttpSpan is called with. They will be initialized once
// so each call to sendHttpSpan doesn't create the strings; it
// only creates a local reference to them.
static Nan::Persistent<v8::String> kName;
static Nan::Persistent<v8::String> kTxname;
static Nan::Persistent<v8::String> kUrl;
static Nan::Persistent<v8::String> kDomain;
static Nan::Persistent<v8::String> kDuration;
static Nan::Persistent<v8::String> kStatus;
static Nan::Persistent<v8::String> kMethod;
static Nan::Persistent<v8::String> kError;

NAN_METHOD(Reporter::sendHttpSpan) {
    if (info.Length() != 1 || !info[0]->IsObject()) {
        return Nan::ThrowTypeError("Reporter::sendHttpSpan() - requires Object parameter");
    }
    v8::Local<v8::Object> obj = info[0]->ToObject();

    oboe_span_params_t args;

    args.version = 1;
    // Number.MAX_SAFE_INTEGER is big enough for any reasonable transaction time.
    // max_safe_seconds = MAX_SAFE_INTEGER / 1000000 microseconds
    // max_safe_days = MAX_SAFE_SECONDS / 86400 seconds
    // max_safe_days > 100000. Seems long enough to me.
    args.duration = Utility::get_integer(obj, Nan::New(kDuration));
    args.has_error = Utility::get_boolean(obj, Nan::New(kError), false);
    args.status = Utility::get_integer(obj, Nan::New(kStatus));

    // REMEMBER TO FREE ALL RETURNED STD::STRINGS AFTER PASSING
    // THEM TO OBOE.
    std::string* txname = Utility::get_string(obj, Nan::New(kTxname));
    args.transaction = txname->c_str();

    std::string* url = Utility::get_string(obj, Nan::New(kUrl));
    args.url = url->c_str();

    std::string* domain = Utility::get_string(obj, Nan::New(kDomain));
    args.domain = domain->c_str();

    std::string* method = Utility::get_string(obj, Nan::New(kMethod));
    args.method = method->c_str();

    char final_txname[1024];

    int length = oboe_http_span(final_txname, sizeof(final_txname), &args);

    // don't forget to clean up created strings.
    delete txname;
    delete url;
    delete domain;
    delete method;

    // if an error code return an empty string
    if (length < 0) {
        final_txname[0] = '\0';
    }

    // return the transaction name used so it can be used by the agent.
    info.GetReturnValue().Set(Nan::New(final_txname).ToLocalChecked());
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

    // initialize object property names in persistent storage
    kName.Reset(Nan::New<v8::String>("name").ToLocalChecked());
    kTxname.Reset(Nan::New<v8::String>("txname").ToLocalChecked());
    kUrl.Reset(Nan::New<v8::String>("url").ToLocalChecked());
    kDomain.Reset(Nan::New<v8::String>("domain").ToLocalChecked());
    kDuration.Reset(Nan::New<v8::String>("duration").ToLocalChecked());
    kStatus.Reset(Nan::New<v8::String>("status").ToLocalChecked());
    kMethod.Reset(Nan::New<v8::String>("method").ToLocalChecked());
    kError.Reset(Nan::New<v8::String>("error").ToLocalChecked());

    // Prototype
    Nan::SetPrototypeMethod(ctor, "isReadyToSample", Reporter::isReadyToSample);
    Nan::SetPrototypeMethod(ctor, "sendReport", Reporter::sendReport);
    Nan::SetPrototypeMethod(ctor, "sendStatus", Reporter::sendStatus);
    Nan::SetPrototypeMethod(ctor, "sendHttpSpan", Reporter::sendHttpSpan);
    //Nan::SetPrototypeMethod(ctor, "sendHttpSpanName", Reporter::sendHttpSpanName);
    //Nan::SetPrototypeMethod(ctor, "sendHttpSpanUrl", Reporter::sendHttpSpanUrl);

    Nan::Set(exports, Nan::New("Reporter").ToLocalChecked(), ctor->GetFunction());
}

