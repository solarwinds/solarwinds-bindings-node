#include "bindings.h"

int64_t get_integer(Napi::Object, const char*, int64_t = 0);
std::string* get_string(Napi::Object, const char*, const char* = "");
bool get_boolean(Napi::Object obj, const char*, bool = false);

Napi::FunctionReference Reporter::constructor;

// Construct with an address and port to report to
Reporter::Reporter(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Reporter>(info) {
  // this is a shell for now that oboe implements the reporter
  // TODO BAM make this a set of static functions. No need for reporter class.
}

Reporter::~Reporter() {
}

// Check to see if oboe is ready to issue sampling decisions.
// Returns true if oboe is ready else numeric error code.
Napi::Value Reporter::isReadyToSample(const Napi::CallbackInfo& info) {
  int ms = 0;          // milliseconds to wait
  if (info[0].IsNumber()) {
    ms = info[0].As<Napi::Number>().Int64Value();
  }

  int status;
  status = oboe_is_ready(ms);

  // UNKNOWN 0
  // OK 1
  // TRY_LATER 2
  // LIMIT_EXCEEDED 3
  // INVALID_API_KEY 4
  // CONNECT_ERROR 5
  return Napi::Number::New(info.Env(), status);
}

// Send an event to the reporter
Napi::Value Reporter::sendReport(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !Event::isEvent(info[0].As<Napi::Object>())) {
    Napi::TypeError::New(env, "missing event").ThrowAsJavaScriptException();
    return env.Null();
  }
  int status = this->send_event_x(info, OBOE_SEND_EVENT);

  return Napi::Number::New(env, status);
}

// send status. only used for init message.
Napi::Value Reporter::sendStatus(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !Event::isEvent(info[0].As<Napi::Object>())) {
    Napi::TypeError::New(env, "invalid arguments").ThrowAsJavaScriptException();
    return env.Null();
  }

  int status = this->send_event_x(info, OBOE_SEND_STATUS);

  return Napi::Number::New(env, status);
}

int Reporter::send_event_x(const Napi::CallbackInfo& info, int channel) {
  // info has been passed from a C++ function using that function's info. As
  // this is called only from C++ there is no type checking done.
  Event* event = Napi::ObjectWrap<Event>::Unwrap(info[0].ToObject());

  // if metadata was passed in use it otherwise grab it from oboe.
  oboe_metadata_t* md;
  if (info.Length() >= 2 && Metadata::isMetadata(info[1].As<Napi::Object>())) {
    md = &Napi::ObjectWrap<Metadata>::Unwrap(info[1].ToObject())->metadata;
  } else {
    md = oboe_context_get();
  }

  int status = oboe_event_send(channel, &event->event, md);

  return status;
}

//
// send a metrics span using oboe_http_span
//
Napi::Value Reporter::sendHttpSpan(const Napi::CallbackInfo& info) {
    return send_span(info, oboe_http_span);
}

//
// send a metrics span using oboe_span
//
Napi::Value Reporter::sendNonHttpSpan(const Napi::CallbackInfo& info) {
    return send_span(info, oboe_span);
}

//
// do all the work to send a span
//
Napi::Value Reporter::send_span(const Napi::CallbackInfo& info, send_generic_span_t send_function) {
  Napi::Env env = info.Env();

  if (info.Length() != 1 || !info[0].IsObject()) {
      Napi::TypeError::New(env, "Reporter::sendXSpan() - requires Object parameter").ThrowAsJavaScriptException();
      return env.Null();
  }
  Napi::Object obj = info[0].ToObject();

  oboe_span_params_t args;

  args.version = 1;
  // Number.MAX_SAFE_INTEGER is big enough for any reasonable transaction time.
  // max_safe_seconds = MAX_SAFE_INTEGER / 1000000 microseconds
  // max_safe_days = max_safe_seconds / 86400 seconds
  // max_safe_days > 100000. Seems long enough to me.
  args.duration = get_integer(obj, "duration");
  args.has_error = get_boolean(obj, "error", false);
  args.status = get_integer(obj, "status");

  // REMEMBER TO FREE ALL RETURNED STD::STRINGS AFTER PASSING
  // THEM TO OBOE.
  std::string* txname = get_string(obj, "txname");
  args.transaction = txname->c_str();

  std::string* url = get_string(obj, "url");
  args.url = url->c_str();

  std::string* domain = get_string(obj, "domain");
  args.domain = domain->c_str();

  std::string* method = get_string(obj, "method");
  args.method = method->c_str();

  std::string* service = get_string(obj, "service");
  args.service = service->c_str();

  char final_txname[OBOE_TRANSACTION_NAME_MAX_LENGTH + 1];

  int length = send_function(final_txname, sizeof(final_txname), &args);

  // don't forget to FREE STRINGS CREATED by get_string().
  delete txname;
  delete url;
  delete domain;
  delete method;
  delete service;

  // if an error code return an empty string
  if (length < 0) {
      final_txname[0] = '\0';
  }

  // return the transaction name used so it can be used by the agent.
  return Napi::String::New(env, final_txname);

}

//
// This is called at initialization of this module.
//
Napi::Object Reporter::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function ctor = DefineClass(env, "Reporter", {
    InstanceMethod("isReadyToSample", &Reporter::isReadyToSample),
    InstanceMethod("sendReport", &Reporter::sendReport),
    InstanceMethod("sendStatus", &Reporter::sendStatus),
    InstanceMethod("sendHttpSpan", &Reporter::sendHttpSpan),
    InstanceMethod("sendNonHttpSpan", &Reporter::sendNonHttpSpan),
  });

  constructor = Napi::Persistent(ctor);
  constructor.SuppressDestruct();

  exports.Set("Reporter", ctor);

  return exports;
}

int64_t get_integer(Napi::Object obj, const char* key, int64_t default_value) {
  Napi::Value v = obj.Get(key);
  if (v.IsNumber()) {
    return v.As<Napi::Number>().Int64Value();
  }
  return default_value;
}

//
// returns a new std::string that must be deleted.
//
std::string* get_string(Napi::Object obj, const char* key, const char* default_value) {
  Napi::Value v = obj.Get(key);
  if (v.IsString()) {
    return new std::string(v.As<Napi::String>());
  }
  return new std::string(default_value);
}

bool get_boolean(Napi::Object obj, const char* key, bool default_value) {
  Napi::Value v = obj.Get(key);
  if (v.IsBoolean()) {
    return v.As<Napi::Boolean>().Value();
  }
  return default_value;
}
