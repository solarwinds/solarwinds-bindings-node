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

bool http_span_args_are_good(Nan::NAN_METHOD_ARGS_TYPE info) {
    // anything in JavaScript has a boolean value so no need to check.
    return info.Length() == 5 && info[0]->IsString() && info[1]->IsNumber() &&
        info[2]->IsInt32() && info[3]->IsString(); // && info[4]->IsBoolean();
}

typedef struct oboe_span_params {
    int version; // the version of this structure
    const char* transaction; // transaction name (will be NULL or empty if url given)
    const char* url; // the raw url which will be processed and used as transaction name
        // (if transaction is NULL or empty)
    const char* domain; // a domain to be prepended to the transaction name (can be NULL)
    int64_t duration; // the duration of the span in micro seconds (usec)
    int status; // HTTP status code (e.g. 200, 500, ...)
    const char* method; // HTTP method (e.g. GET, POST, ...)
    int has_error; // boolean flag whether this transaction contains an error (1) or not (0)
} oboe_span_params_t;

// These will be the string object keys for the object that
// sendHttpSpan is called with. They will be initialized once
// so each call to sendHttpSpan doesn't create the strings.
static Nan::Persistent<v8::String> kName;
static Nan::Persistent<v8::String> kTxName;
static Nan::Persistent<v8::String> kUrl;
static Nan::Persistent<v8::String> kDomain;
static Nan::Persistent<v8::String> kDuration;
static Nan::Persistent<v8::String> kStatus;
static Nan::Persistent<v8::String> kMethod;
static Nan::Persistent<v8::String> kError;

NAN_METHOD(Reporter::sendHttpSpan) {
    if (info.Length() != 1 || !info[0]->IsObject()) {
        info.GetReturnValue().Set(Nan::New(false));
        return;
    }

    oboe_span_params_t args;

    v8::Local<v8::Object> obj = info[0]->ToObject();

    args.version = 1;
    args.duration = Utility::get_integer(obj, Nan::New(kDuration));
    args.has_error = Utility::get_boolean(obj, Nan::New(kError), false);
    args.status = Utility::get_integer(obj, Nan::New(kStatus));

    // REMEMBER TO FREE ALL RETURNED STD::STRINGS
    std::string* txname = Utility::get_string(obj, Nan::New(kTxName));
    args.transaction = txname->c_str();

    std::string* url = Utility::get_string(obj, Nan::New(kUrl));
    args.url = url->c_str();

    std::string* domain = Utility::get_string(obj, Nan::New(kDomain));
    args.domain = domain->c_str();

    std::string* method = Utility::get_string(obj, Nan::New(kMethod));
    args.method = method->c_str();

    printf("version %d\ntxname %s\nurl %s\ndomain %s\nduration %ld\nstatus %d\nmethod %s\nerror %d\n",
        args.version,
        args.transaction,
        args.url,
        args.domain,
        args.duration,
        args.status,
        args.method,
        args.has_error
    );

    delete txname;
    delete url;
    delete domain;
    delete method;
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

    // initialize object property names in persistent storage
    kName.Reset(Nan::New<v8::String>("name").ToLocalChecked());
    kTxName.Reset(Nan::New<v8::String>("txName").ToLocalChecked());
    kUrl.Reset(Nan::New<v8::String>("url").ToLocalChecked());
    kDomain.Reset(Nan::New<v8::String>("domain").ToLocalChecked());
    kDuration.Reset(Nan::New<v8::String>("duration").ToLocalChecked());
    kStatus.Reset(Nan::New<v8::String>("status").ToLocalChecked());
    kMethod.Reset(Nan::New<v8::String>("method").ToLocalChecked());
    kError.Reset(Nan::New<v8::String>("error").ToLocalChecked());

    // Prototype
    Nan::SetPrototypeMethod(ctor, "sendReport", Reporter::sendReport);
    Nan::SetPrototypeMethod(ctor, "sendStatus", Reporter::sendStatus);
    Nan::SetPrototypeMethod(ctor, "sendHttpSpan", Reporter::sendHttpSpan);
    Nan::SetPrototypeMethod(ctor, "sendHttpSpanName", Reporter::sendHttpSpanName);
    Nan::SetPrototypeMethod(ctor, "sendHttpSpanUrl", Reporter::sendHttpSpanUrl);

    Nan::Set(exports, Nan::New("Reporter").ToLocalChecked(), ctor->GetFunction());
}

