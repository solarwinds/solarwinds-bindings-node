#include "bindings.h"
#include <vector>

int64_t get_integer(Napi::Object, const char*, int64_t = 0);
std::string* get_string(Napi::Object, const char*, const char* = "");
bool get_boolean(Napi::Object obj, const char*, bool = false);

int send_event_x(const Napi::CallbackInfo&, int);
Napi::Value send_span(const Napi::CallbackInfo&, send_generic_span_t send_function);

//
// send a span using oboe_http_span
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

  // if an error return the code.
  if (length < 0) {
    return Napi::Number::New(env, length);
  }

  // return the transaction name used so it can be used by the agent.
  return Napi::String::New(env, final_txname);

}

enum SMFlags {
  kSMFlagsTesting = 1 << 0,
  kSMFlagsNoop = 1 << 1
};
//
// internal function used by sendMetric() (deprecated) and sendMetrics().
//
Napi::Value send_metrics_core (Napi::Env env, Napi::Array metrics, uint64_t flags) {
  int64_t goodCount = 0;
  const char* service_name = "";
  bool testing = flags & kSMFlagsTesting;
  bool noop = flags & kSMFlagsNoop;

  Napi::Array errors = Napi::Array::New(env);
  Napi::Array echo;
  Napi::Object echoTags;
  if (testing) {
    echo = Napi::Array::New(env);
  }

  //
  // loop through each metric and send if valid else
  // report back in errors array.
  //
  for (size_t i = 0; i < metrics.Length(); i++) {
    bool is_summary = false;
    std::string name;
    int64_t count;
    double value = 0;
    bool add_host_tag = false;

    Napi::Value element = metrics[i];

    // little lambda for errors.
    auto set_error = [&](const char* code) {
      Napi::Object err = Napi::Object::New(env);
      err.Set("code", code);
      err.Set("metric", element);
      errors[errors.Length()] = err;
    };

    if (!element.IsObject() || element.IsArray()) {
      set_error("metric must be plain object");
      continue;
    }
    Napi::Object metric = element.As<Napi::Object>();

    // make sure there is a string name. valid characters if
    // choosing to add in the future: ‘A-Za-z0-9.:-_’.
    if (!metric.Has("name") || !metric.Get("name").IsString()) {
      set_error("must have string name");
      continue;
    }
    name = metric.Get("name").As<Napi::String>();

    // and a numeric count
    if (!metric.Has("count")) {
      count = 1;
    } else {
      Napi::Value c = metric.Get("count");
      if (!c.IsNumber()) {
        set_error("count must be a number");
        continue;
      }
      count = c.As<Napi::Number>().DoubleValue();
      if (count <= 0) {
        set_error("count must be greater than 0");
        continue;
      }
    }

    // if there is a value it's a summary metric
    if (metric.Has("value")) {
      Napi::Value v = metric.Get("value");
      if (!v.IsNumber()) {
        set_error("summary value must be numeric");
        continue;
      }
      is_summary = true;
      value = v.As<Napi::Number>().DoubleValue();
    }

    if (metric.Has("addHostTag")) {
      add_host_tag = metric.Get("addHostTag").ToBoolean();
    }

    //
    // handle tags
    //
    size_t tag_count = 0;
    Napi::Object tags;
    Napi::Array keys;

    if (metric.Has("tags")) {
      Napi::Value v = metric.Get("tags");
      if (!v.IsObject() || v.IsArray()) {
        set_error("tags must be plain object");
        continue;
      }
      tags = v.As<Napi::Object>();
      keys = tags.GetPropertyNames();
      tag_count = keys.Length();

      if (testing) {
        echoTags = Napi::Object::New(env);
      }
    }

    // allocate oboe's key-value pair structure
    oboe_metric_tag_t otags[tag_count];

    // save all the tag strings so they won't go out of scope
    std::vector<std::string> holdKeys(tag_count);
    std::vector<std::string> holdValues(tag_count);

    bool had_error = false;
    size_t n = 0;
    while (n < tag_count) {
      Napi::Value key = keys[n];
      holdKeys[n] = key.ToString();

      Napi::Value value = tags.Get(keys[n]);
      holdValues[n] = value.ToString();
      // i don't know how ToString() can fail but the doc says
      // it can so let's try to handle it.
      if (env.IsExceptionPending()) {
        Napi::Error error = env.GetAndClearPendingException();
        had_error = true;
        break;
      }

      otags[n].key = (char*)holdKeys[n].c_str();
      otags[n].value = (char*)holdValues[n].c_str();

      if (testing) {
        echoTags.Set(holdKeys[n], holdValues[n]);
      }
      n += 1;
    }

    if (had_error) {
      set_error("string conversion of value failed");
      continue;
    }

    int status;
    if (noop) {
      status = 0;
    } else {
      if (is_summary) {
        status = oboe_custom_metric_summary(name.c_str(), value, count,
                                            add_host_tag, service_name,
                                            otags, tag_count);
      } else {
        status = oboe_custom_metric_increment(name.c_str(), count,
                                              add_host_tag, service_name,
                                              otags, tag_count);
      }
    }

    // oboe returns 0 for success else a status code;
    if (status != 0) {
      std::string error_msg = "metric send failed: " + std::to_string(status);
      set_error(error_msg.c_str());
      continue;
    }

    // everything is set at this point
    goodCount += 1;

    // if testing also return metrics that were sent.
    if (testing) {
      Napi::Object m = Napi::Object::New(env);
      m.Set("name", Napi::String::New(env, name));
      m.Set("count", Napi::Number::New(env, count));
      if (is_summary) {
        m.Set("value", Napi::Number::New(env, value));
      }
      m.Set("addHostTag", Napi::Boolean::New(env, add_host_tag));
      if (tag_count > 0) {
        m.Set("tags", echoTags);
      }
      echo[echo.Length()] = m;
    }

  }

  Napi::Object result = Napi::Object::New(env);
  result.Set("errors", errors);
  if (testing) {
    result.Set("correct", echo);
  }

  return result;

}

//
// javascript
//
// sendMetrics(globalOptions, metrics) returns {status, errorMetrics[]}
//
// globalOptions - object
// globalOptions.addHostTag - boolean - add {host: hostname} to tags
// globalOptions.tags - tags which are sent with each metric, individual tags
// override
//                like:
//                  tags = Object.assign({}, globalTags, individualMetricTags);
//
// metrics - array of metrics to send
// metric - each element of metrics is an object:
// metric.name - name of the metric
// metric.count - number of observations being reported
// metric.value - if present this is a "summary" metric. if not it is an
//                "increment" metric. contains the value, or sum of the
//                values if count is greater than 1.
// metric.addHostTag - boolean (overrides options.addHostTag if present)
// metric.tags - object of {tag: value} pairs.
//
//
// c++ - process an array of metrics each with a fully specified set of tags
//
// aob.reporter.sendMetrics(increments, summaries)
//
Napi::Value sendMetrics(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  // check args
  if (info.Length() < 1 || !info[0].IsArray()) {
    Napi::TypeError::New(env, "invalid signature for sendMetrics()")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // allow some testing
  uint64_t flags = 0;

  if (info.Length() == 2 && info[1].IsObject()) {
    Napi::Object options = info[1].As<Napi::Object>();
    if (options.Get("testing").ToBoolean()) flags |= kSMFlagsTesting;
    if (options.Get("noop").ToBoolean()) flags |= kSMFlagsNoop;
  }

  Napi::Array metrics = info[0].As<Napi::Array>();

  return send_metrics_core(env, metrics, flags);
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

  Napi::Object metric = Napi::Object::New(env);
  // name
  metric.Set("name", info[0]);

  // count
  int count = 1;
  Napi::Value v = o.Get("count");
  if (v.IsNumber()) {
    count = v.As<Napi::Number>().Int64Value();
  }
  metric.Set("count", count);

  // value
  double value = 0;
  if (o.Has("value")) {
    v = o.Get("value");
    if (!v.IsNumber()) {
      Napi::TypeError::New(env, "sendMetric options.value must be a number")
          .ThrowAsJavaScriptException();
      return env.Null();
    }
    value = v.As<Napi::Number>().DoubleValue();
    metric.Set("value", value);
  }

  // host_tag
  if (o.Has("addHostTag")) {
    metric.Set("addHostTag", o.Get("addHostTag"));
  }

  // tags
  if (o.Has("tags")) {
    v = o.Get("tags");
    if (!v.IsObject() || v.IsArray()) {
      Napi::TypeError::New(env, "sendMetric() tags must be a plain object")
          .ThrowAsJavaScriptException();
      return env.Null();
    }
    metric.Set("tags", v);
  }

  int64_t flags = 0;
  // noop - don't call oboe if set.
  if (o.Get("noop").ToBoolean().Value()) flags |= kSMFlagsNoop;

  Napi::Array metrics_array = Napi::Array::New(env, 1);
  metrics_array[(uint32_t)0] = metric;

  Napi::Value result = send_metrics_core(env, metrics_array, flags);

  // if unexpected results don't know what to do.
  if (!result.IsObject()) {
    return Napi::Number::New(env, 0);
  }

  Napi::Object robj = result.As<Napi::Object>();

  int error = 0;
  if (robj.Has("errors")) {
    Napi::Value errors = robj.Get("errors");
    if (errors.IsArray()) {
      Napi::Array error_array = errors.As<Napi::Array>();
      if (error_array.Length()) {
        error = -1;
      }
    }
  }

  return Napi::Number::New(env, -error);
}

//
// lambda additions
//
Napi::Value flush (const Napi::CallbackInfo& info) {
  int status = oboe_reporter_flush();
  // {OK: 0, TOO_BIG: 1, BAD_UTF8: 2, NO_REPORTER: 3, NOT_READY: 4}
  return Napi::Number::New(info.Env(), status);
}

Napi::Value getType (const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  const char* type = oboe_get_reporter_type();
  if (!type) {
    return env.Undefined();
  }
  return Napi::String::New(env, type);
}

//
// Initialize the module.
//
namespace Reporter {

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Object module = Napi::Object::New(env);

  module.Set("sendHttpSpan", Napi::Function::New(env, sendHttpSpan));
  module.Set("sendNonHttpSpan", Napi::Function::New(env, sendNonHttpSpan));

  module.Set("sendMetric", Napi::Function::New(env, sendMetric));
  module.Set("sendMetrics", Napi::Function::New(env, sendMetrics));

  module.Set("flush", Napi::Function::New(env, flush));
  module.Set("getType", Napi::Function::New(env, getType));

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
