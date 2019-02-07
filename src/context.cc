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
// Check if the current request should be sampled based on the current settings.
//
// info[0] - layer name
// info[1] - optional xtrace
//
// returns:
//
// object - {sample: boolean, source: coded_integer, rate: sample_rate}
//
// sample is true if the request should be sampled.
//
// If xtrace is empty, or if it is not valid then it will be considered a
// new trace. Otherwise sampling will add to the existing trace.
// Different layers may have special rules.
//
// This is designed to be called once per request at the entry layer.
//
// @param layer Name of the layer being considered for sampling
// @param in_xtrace Incoming X-Trace ID (NULL or empty string if not present)
// @return {Object} {sample, source, rate}
//   sample - boolean true if should sample
//   source - source used for sampling decision
//   rate - rate used for sampling decision
//
Napi::Value sampleTrace(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  // Validate arguments
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "invalid arguments").ThrowAsJavaScriptException();
    return env.Null();
  }

  std::string layer_name;
  std::string in_xtrace;

  layer_name = info[0].As<Napi::String>().Utf8Value();

  // If the second argument is present, it must be a string
  // TODO even though oboe requires a string this could accept
  // any form of metadata (i.e., string, metadata, or event).
  if (info.Length() >= 2) {
    if (!info[1].IsString()) {
      Napi::TypeError::New(env, "X-Trace ID must be a string").ThrowAsJavaScriptException();
      return env.Null();
    }
    in_xtrace = info[1].As<Napi::String>().Utf8Value();
  }

  int sample_rate = 0;
  int sample_source = 0;
  int sample = oboe_sample_layer(
    layer_name.c_str(),
    in_xtrace.c_str(),
    &sample_rate,
    &sample_source
  );

  // create an object to return multiple values
  Napi::Object o = Napi::Object::New(env);
  o.Set("sample", Napi::Boolean::New(env, sample));
  o.Set("source", Napi::Number::New(env, sample_source));
  o.Set("rate", Napi::Number::New(env, sample_rate));

  return o;
}

//
// Stringify the current context's metadata structure.
//
Napi::Value toString(const Napi::CallbackInfo& info) {
  char buf[OBOE_MAX_METADATA_PACK_LEN];
  oboe_metadata_t* md = oboe_context_get();

  int rc;

  // for now any argument means use our format. maybe accept options/flags?
  if (info.Length() == 1) {
    rc = Metadata::format(md, sizeof(buf), buf) ? 0 : -1;
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

//
// New function to start a trace. It returns all information necessary in a single call.
//
// getTraceSettings(object)
//
// object.xtrace - Metadata instance or undefined
// object.mode - a route-specific trace mode, 0 or 1 for 'never' or 'always'
// object.rate - a route-specific sampling rate
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

      // try to convert it to metadata. if it fails act as if no xtrace was
      // supplied.
      int status = oboe_metadata_fromstr(&omd, xtrace.c_str(), xtrace.length());
      // status can be zero with a version other than 2, so check that too.
      if (status < 0 || omd.version != 2) {
        xtrace = "";
      } else {
        have_metadata = true;
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

  if (status != 0) {
    Napi::Error::New(env, "Failed to get trace settings").ThrowAsJavaScriptException();
    return env.Undefined();
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
  module.Set("sampleTrace", Napi::Function::New(env, sampleTrace));
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
