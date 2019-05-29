#include "bindings.h"
#include <cmath>

//
// Set the tracing mode.
//
// @param newMode One of
// - OBOE_TRACE_NEVER(0) to disable tracing,
// - OBOE_TRACE_ALWAYS(1) to start a new trace if needed
//
Napi::Value setTracingMode(const Napi::CallbackInfo& info) {
  const Napi::Env env = info.Env();

  // Validate arguments
  if (info.Length() != 1 || !info[0].IsNumber()) {
    Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
    return env.Null();
  }

  int mode = info[0].As<Napi::Number>().Uint32Value();
  if (mode != OBOE_TRACE_NEVER && mode != OBOE_TRACE_ALWAYS) {
    Napi::RangeError::New(env, "Invalid tracing mode").ThrowAsJavaScriptException();
    return env.Null();
  }

  oboe_settings_mode_set(mode);

  return env.Null();
}

//
// Set the default sample rate.
//
// This rate is used until overridden by the AppOptics servers.  If not set then
// oboe supplies a default value (300,000, i.e., 30%) at time of this writing.
//
// The rate is interpreted as a ratio out of OBOE_SAMPLE_RESOLUTION (1,000,000).
//
// @param newRate A number between 0 (none) and OBOE_SAMPLE_RESOLUTION (a million)
//
Napi::Value setDefaultSampleRate(const Napi::CallbackInfo& info) {
    // presume failure
    int rateUsed = -1;

    // Validate arguments, if not valid or if the argument is nan,
    // return -1 (impossible rate).
    if (info.Length() == 1 && info[0].IsNumber()) {
        double check = info[0].As<Napi::Number>().DoubleValue();
        // don't convert check to an int because NaN becomes 0
        int rate = check;
        if (!std::isnan(check)) {
            // it's a valid number but maybe not in range.
            if (rate < 0) {
                rate = 0;
            } else if (rate > OBOE_SAMPLE_RESOLUTION) {
                rate = OBOE_SAMPLE_RESOLUTION;
            }
            rateUsed = rate;
            oboe_settings_rate_set(rate);
        }
    }

    return Napi::Number::New(info.Env(), rateUsed);
}

//
// Stringify the current context's metadata structure.
//
Napi::Value toString(const Napi::CallbackInfo& info) {
  char buf[Metadata::fmtBufferSize];
  oboe_metadata_t* md = oboe_context_get();

  int rc;

  if (info.Length() == 1 && info[0].IsNumber()) {
    int fmt = info[0].As<Napi::Number>().Int64Value();
    // default 1 to a human readable form for historical reasons.
    if (fmt == 1) {
      fmt = Metadata::fmtHuman;
    }
    rc = Metadata::format(md, sizeof(buf), buf, fmt);
  } else {
    rc = oboe_metadata_tostr(md, buf, sizeof(buf) - 1);
  }

  return Napi::String::New(info.Env(), rc == 0 ? buf : "");
}

//
// Set oboe's context to the argument supplied. The argument can
// be in any form that Metadata::getMetadata() can decode.
//
Napi::Value set(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  // Validate arguments
  if (info.Length() != 1) {
    Napi::Error::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
    return env.Null();
  }

  //bool Metadata::getMetadata(Napi::Value v, oboe_metadata_t * omd)
  oboe_metadata_t omd;

  int status = Metadata::getMetadata(info[0], &omd);
  if (!status) {
    Napi::TypeError::New(env, "invalid metadata").ThrowAsJavaScriptException();
    return env.Null();
  }

  // Set the context
  oboe_context_set(&omd);

  return env.Null();
}

//
// clear is used for testing.
//
Napi::Value clear(const Napi::CallbackInfo& info) {
  oboe_context_clear();
  return info.Env().Null();
}

//
// return a boolean indicating whether oboe's current context is
// valid.
//
Napi::Value isValid(const Napi::CallbackInfo& info) {
  return Napi::Boolean::New(info.Env(), oboe_context_is_valid());
}

//
// Extended method to create events. Replaced createEvent but allows
// an argument of the metadata to use to create the event. With no
// argument it uses oboe's current context as the metadata.
//
Napi::Value createEventX(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    oboe_metadata_t omd;
    bool add_edge = true;

    //
    // If no metadata is supplied use oboe's.
    //
    if (info.Length() == 0) {
      oboe_metadata_t* context = oboe_context_get();
      omd = *context;
    } else if (!Metadata::getMetadata(info[0], &omd)) {
      Napi::TypeError::New(env, "Invalid argument").ThrowAsJavaScriptException();
      return env.Null();
    }

    if (info.Length() >= 2) {
        add_edge = info[1].ToBoolean().Value();
    }

    // create the event
    Napi::Object event = Event::NewInstance(env, &omd, add_edge);

    // check for error.
    Event* e = Napi::ObjectWrap<Event>::Unwrap(event);
    if (e->oboe_status != 0) {
      Napi::Error::New(env, "Oboe failed to create event").ThrowAsJavaScriptException();
      return env.Null();
    }

    return event;
}

// 1: OBOE_SAMPLE_RATE_SOURCE_FILE - local agent config
// 2: OBOE_SAMPLE_RATE_SOURCE_DEFAULT - compiled default
// 3: OBOE_SAMPLE_RATE_SOURCE_OBOE - remote config (layer-specific)
// 4: OBOE_SAMPLE_RATE_SOURCE_LAST_OBOE - previous oboe setting
// 5: OBOE_SAMPLE_RATE_SOURCE_DEFAULT_MISCONFIGURED - unknown source
// 6: OBOE_SAMPLE_RATE_SOURCE_OBOE_DEFAULT - remote setting(default)
// 7: OBOE_SAMPLE_RATE_SOURCE_CUSTOM - custom setting passed as parameter via
//                                     the oboe_tracing_decisions(). supports
//                                     request level customized mode/rates.

//
// New function to start a trace. It returns all information
// necessary in a single call.
//
// getTraceSettings(object)
//
// object.xtrace - Metadata instance or undefined
// object.mode - a route-specific trace mode, 0 or 1 for 'never'
// or 'always' object.rate - a route-specific sampling rate
// object.edge - override the default edge setting.
//
Napi::Value getTraceSettings(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  oboe_tracing_decisions_in_t in;
  oboe_tracing_decisions_out_t out;
  oboe_metadata_t omd;

  // in defaults
  bool have_metadata = false;
  std::string xtrace("");
  int rate = -1;
  int mode = -1;
  // edge back to supplied metadata unless there is none.
  bool edge = true;

  // caller specified values. errors are ignored and default values are used.
  if (info[0].IsObject()) {
    Napi::Object o = info[0].ToObject();

    // is an xtrace supplied?
    Napi::Value v = o.Get("xtrace");
    if (v.IsString()) {
      xtrace = v.As<Napi::String>();

      // make sure it's the right length before calling oboe.
      if (xtrace.length() == 60) {
        // try to convert it to metadata. if it fails act as if no xtrace was
        // supplied.
        int status = oboe_metadata_fromstr(&omd, xtrace.c_str(), xtrace.length());
        // status can be zero with a version other than 2, so check that too.
        if (status < 0 || omd.version != 2) {
          xtrace = "";
        } else {
          have_metadata = true;
        }
      } else {
        // if it's the wrong length don't pass it to oboe
        xtrace = "";
      }
    }

    // now get the much simpler integer values
    v = o.Get("rate");
    if (v.IsNumber()) {
      rate = v.As<Napi::Number>().Int64Value();
    }

    v = o.Get("mode");
    if (v.IsNumber()) {
      mode = v.As<Napi::Number>().Int64Value();
    }

    // allow overriding the edge setting. it's not clear why
    // this might need to be done but it does add some control
    // for testing or unforseen cases.
    if (o.Has("edge")) {
      edge = o.Get("edge").ToBoolean().Value();
    }
  }

  // if no xtrace or the xtrace was bad then construct new metadata.
  // specifying the edge as true makes no sense in this case because
  // there is no previous metadata.
  if (!have_metadata) {
    edge = false;
    oboe_metadata_init(&omd);
    oboe_metadata_random(&omd);
  }

  // apply default or user specified values.
  in.version = 1;
  in.service_name = "";
  in.in_xtrace = xtrace.c_str();
  in.custom_sample_rate = rate;
  in.custom_tracing_mode = mode;

  // set the version in case oboe adds fields in the future.
  out.version = 1;

  // ask for oboe's decisions on life, the universe, and everything.
  int status = oboe_tracing_decisions(&in, &out);

                                                // tracing disabled -2
                                                // xtrace not sampled -1
  const char* messages[] = {
    (const char *) ("ok"),                      // 0
    (const char *) ("no output structure"),     // 1
    (const char *) ("no config"),               // 2
    (const char *) ("reporter not ready"),      // 3
    (const char *) ("no valid settings"),       // 4
    (const char *) ("queue full"),              // 5
  };

  if (status > 0) {
    const char* m;
    if (status > 5) {
      m = "failed to get trace settings";
    } else {
      m = messages[status];
    }

    // assemble an error return
    Napi::Object o = Napi::Object::New(env);
    o.Set("error", status);
    o.Set("message", m);
    return o;
  }

  // now we have oboe_metadata_t either from a supplied xtrace id or from
  // a Metadata object created for this span. set the sample bit to match
  // the sample decision and create a JavaScript Metadata instance.
  if (out.do_sample) {
    omd.flags |= XTR_FLAGS_SAMPLED;
  } else {
    omd.flags &= ~XTR_FLAGS_SAMPLED;
  }
  Napi::Value v = Napi::External<oboe_metadata_t>::New(env, &omd);
  Napi::Object md = Metadata::NewInstance(env, v);

  // assemble the return object
  Napi::Object o = Napi::Object::New(env);
  o.Set("metadata", md);
  o.Set("metadataFromXtrace", Napi::Boolean::New(env, have_metadata));
  o.Set("status", Napi::Number::New(env, status));
  o.Set("edge", Napi::Boolean::New(env, edge));
  o.Set("doSample", Napi::Boolean::New(env, out.do_sample));
  o.Set("doMetrics", Napi::Boolean::New(env, out.do_metrics));
  o.Set("source", Napi::Number::New(env, out.sample_source));
  o.Set("rate", Napi::Number::New(env, out.sample_rate));

  return o;
}

//
// This is not a class, just a group of functions in a JavaScript namespace.
//
namespace OboeContext {

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Object module = Napi::Object::New(env);

  module.Set("setTracingMode", Napi::Function::New(env, setTracingMode));
  module.Set("setDefaultSampleRate", Napi::Function::New(env, setDefaultSampleRate));
  module.Set("toString", Napi::Function::New(env, toString));
  module.Set("set", Napi::Function::New(env, set));
  module.Set("clear", Napi::Function::New(env, clear));
  module.Set("isValid", Napi::Function::New(env, isValid));
  module.Set("createEventX", Napi::Function::New(env, createEventX));
  module.Set("getTraceSettings", Napi::Function::New(env, getTraceSettings));

  exports.Set("Context", module);

  return exports;
}

} // end namespace OboeContext
