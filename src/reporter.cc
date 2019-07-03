#include "bindings.h"
#include <vector>

int64_t get_integer(Napi::Object, const char*, int64_t = 0);
std::string* get_string(Napi::Object, const char*, const char* = "");
bool get_boolean(Napi::Object obj, const char*, bool = false);

int send_event_x(const Napi::CallbackInfo&, int);
Napi::Value send_span(const Napi::CallbackInfo&, send_generic_span_t send_function);

//
// Check to see if oboe is ready to issue sampling decisions.
//
// returns coded status as below
//
Napi::Value isReadyToSample(const Napi::CallbackInfo& info) {
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

//
// Send an event to the reporter
//
Napi::Value sendReport(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !Event::isEvent(info[0].As<Napi::Object>())) {
    Napi::TypeError::New(env, "missing event").ThrowAsJavaScriptException();
    return env.Null();
  }
  int status = send_event_x(info, OBOE_SEND_EVENT);

  return Napi::Number::New(env, status);
}

//
// send status. only used for init message.
//
Napi::Value sendStatus(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !Event::isEvent(info[0].As<Napi::Object>())) {
    Napi::TypeError::New(env, "invalid arguments").ThrowAsJavaScriptException();
    return env.Null();
  }

  int status = send_event_x(info, OBOE_SEND_STATUS);

  return Napi::Number::New(env, status);
}

//
// Common code for sendReport and sendStatus.
//
int send_event_x(const Napi::CallbackInfo& info, int channel) {
  // info has been passed from a C++ function using that function's info. As
  // this is called only from C++ there is no type checking done.
  Event* event = Napi::ObjectWrap<Event>::Unwrap(info[0].ToObject());

  // fake up metadata so oboe can check it. change the op_id so it doesn't
  // match the event's in oboe's check.
  oboe_metadata_t omd = event->event.metadata;
  omd.ids.op_id[0] += 1;

  // send the event. 0 is success.
  int status = oboe_event_send(channel, &event->event, &omd);

  return status;
}

//
// send a metrics span using oboe_http_span
//
Napi::Value sendHttpSpan(const Napi::CallbackInfo& info) {
    return send_span(info, oboe_http_span);
}

//
// send a metrics span using oboe_span
//
Napi::Value sendNonHttpSpan(const Napi::CallbackInfo& info) {
    return send_span(info, oboe_span);
}

//
// do all the work to send a span
//
Napi::Value send_span(const Napi::CallbackInfo& info, send_generic_span_t send_function) {
  Napi::Env env = info.Env();

  if (info.Length() != 1 || !info[0].IsObject()) {
      Napi::TypeError::New(env, "sendXSpan() - requires Object parameter").ThrowAsJavaScriptException();
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
// sendMetric(name, object)
//
// only the first argument is required for an increment call.
//
// name - the name of the metric
// object - an object containing optional parameters
// object.count - the number of observations being reported (default: 1)
// object.addHostTag - boolean - {host: hostname} to tags.
// object.tags - an object containing {tag: value} pairs.
// object.value - if present this call is a valued-based call and this contains
//                the value, or sum of values if count is greater than 1, being
//                reported.
//
// there are two types of metrics:
//   1) count-based - the number of times something has occurred (no value associated with this metric)
//   2) value-based - a specific value is being reported (or a sum of values)
//
// simplest forms:
// sendMetric('my.little.count')
// sendMetric('my.little.value', {value: 234.7})
//
// to report two observations:
// sendMetric('my.little.count', {count: 2})
// sendMetric('my.little.value', {count: 2, value: 469.4})
//
// to supply tags that can be used for filtering:
// sendMetric('my.little.count', {tags: {status: error}})
//
// to add a host name tag:
// sendMetric('my.little.count', {addHostTag: true, tags: {status: error}})
//
// returns -1 for success else error code. the only error now is 0.
//
Napi::Value sendMetric (const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  // check args
  if (info.Length() == 0) {
    Napi::TypeError::New(env, "sendMetric() requires 1 or 2 arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  } else if (!info[0].IsString()) {
      Napi::TypeError::New(env, "sendMetric(name) name argument must be a string")
          .ThrowAsJavaScriptException();
      return env.Null();
  }

  // default the options object if not supplied
  Napi::Object o;
  if (info.Length() == 1) {
    o = Napi::Object::New(env);
  } else if (info[1].IsObject() && !info[1].IsArray()) {
    o = info[1].ToObject();
  } else {
    Napi::TypeError::New(env, "sendMetric(name, params) params must be a plain object")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  // name
  std::string name(info[0].As<Napi::String>());

  // count
  int count = 1;
  Napi::Value v = o.Get("count");
  if (v.IsNumber()) {
    count = v.As<Napi::Number>().Int64Value();
  }

  // value
  // the presence of "value" indicates that this is a summary metric (values)
  // and not an increment metric (count of occurrences).
  bool is_summary = false;
  double value = 0;
  if (o.Has("value")) {
    v = o.Get("value");
    if (!v.IsNumber()) {
      Napi::TypeError::New(env, "sendMetric options.value must be a number")
          .ThrowAsJavaScriptException();
      return env.Null();
    }
    value = v.As<Napi::Number>().DoubleValue();
    is_summary = true;
  }


  // host_tag
  bool host_tag = false;
  if (o.Has("addHostTag")) {
    host_tag = o.Get("addHostTag").ToBoolean().Value();
  }

  // noop - don't call oboe if set.
  v = o.Get("noop");
  bool noop = v.ToBoolean().Value();

  // service_name. unused but required by oboe.
  const char* service_name = "";

  // now look at a tags object if it's present.
  Napi::Object tagsObj;
  size_t tags_count = 0;
  Napi::Array keys;

  // if tags is present and is an object that's not an array then fetch the
  // keys so we know how many there are.
  if (o.Has("tags")) {
    v = o.Get("tags");
    if (!v.IsObject() || v.IsArray()) {
      Napi::TypeError::New(env, "sendMetric() tags must be a plain object").ThrowAsJavaScriptException();
      return env.Null();
    }

    // get the keys of the tags object.
    tagsObj = v.As<Napi::Object>();
    keys = tagsObj.GetPropertyNames();
    tags_count = keys.Length();
  }

  // use the tag count, possibly zero, to allocate the structures needed
  // to store each tag's key and value.

  // oboe's key-value pair structure
  oboe_metric_tag_t otags[tags_count];

  // a place to save the strings so they won't go out of scope at
  // the end of each iteration of the loop.
  std::vector<std::string> holdKeys(tags_count);
  std::vector<std::string> holdValues(tags_count);

  if (tags_count) {
    // coerce the keys and values to strings. they might
    // fail oboe's checks but this makes sure they are strings.
    for (size_t i = 0; i < tags_count; i++) {
      Napi::Value key = keys[i];
      holdKeys[i] = key.ToString();

      Napi::Value value = tagsObj.Get(keys[i]);
      holdValues[i] = value.ToString();

      otags[i].key = (char*) holdKeys[i].c_str();
      otags[i].value = (char*) holdValues[i].c_str();
    }
  }

  // noop allows checking for memory leaks in this code, verifying
  // internal argument handling, etc.
  if (noop) {
    return info.Env().Undefined();
  }

  // having two separate calls is less optimal than just passing the is_summary
  // flag but that's the way it is.
  int status;
  if (is_summary) {
    status = oboe_custom_metric_summary(
      name.c_str(), value, count, host_tag, service_name, otags, tags_count
    );
  } else {
    status = oboe_custom_metric_increment(
      name.c_str(), count, host_tag, service_name, otags, tags_count
    );
  }

  // returns negative number on success else error (currently oboe returns
  // only 0 or 1).
  return Napi::Number::New(env, -status);
}

//
// Initialize the module.
//
namespace Reporter {

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Object module = Napi::Object::New(env);

  module.Set("isReadyToSample", Napi::Function::New(env, isReadyToSample));
  module.Set("sendReport", Napi::Function::New(env, sendReport));
  module.Set("sendStatus", Napi::Function::New(env, sendStatus));

  module.Set("sendHttpSpan", Napi::Function::New(env, sendHttpSpan));
  module.Set("sendNonHttpSpan", Napi::Function::New(env, sendNonHttpSpan));

  module.Set("sendMetric", Napi::Function::New(env, sendMetric));

  exports.Set("Reporter", module);

  return exports;
}

}
//
// Helpers
//
// given an object and a key, if object.key matches the expected type, return the
// appropriate value. if object.key doesn't match the expected type return a default
// value.
//

//
// return an integer
//
int64_t get_integer(Napi::Object obj, const char* key, int64_t default_value) {
  Napi::Value v = obj.Get(key);
  if (v.IsNumber()) {
    return v.As<Napi::Number>().Int64Value();
  }
  return default_value;
}

//
// returns a new std::string that MUST BE DELETED.
//
std::string* get_string(Napi::Object obj, const char* key, const char* default_value) {
  Napi::Value v = obj.Get(key);
  if (v.IsString()) {
    return new std::string(v.As<Napi::String>());
  }
  return new std::string(default_value);
}

//
//  return a boolean
//
bool get_boolean(Napi::Object obj, const char* key, bool default_value) {
  Napi::Value v = obj.Get(key);
  if (v.IsBoolean()) {
    return v.As<Napi::Boolean>().Value();
  }
  return default_value;
}
