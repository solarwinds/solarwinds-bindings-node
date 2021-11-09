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
  std::string tracestate("");
  int rate = -1;
  int mode = -1;
  // edge back to supplied metadata unless there is none.
  bool edge = true;

  // debugging booleans
  //bool showIn = false;
  //bool showOut = false;

  //
  // trigger trace extensions
  //

  // type_requested 0 = normal, 1 = trigger-trace
  int type_requested = 0;
  std::string xtraceOpts("");
  std::string xtraceOptsSig("");
  int64_t xtraceOptsTimestamp = 0;
  int customTriggerMode = -1;

  // caller specified values. errors are ignored and default values are used.
  if (info[0].IsObject()) {
    Napi::Object o = info[0].ToObject();

    // is an xtrace supplied?
    Napi::Value v = o.Get("xtrace");
    if (v.IsString()) {
      xtrace = v.As<Napi::String>();

      // make sure it's the right length before calling oboe.
      if (xtrace.length() == 60 || xtrace.length() == 55) {
        // try to convert it to metadata. if it fails act as if no xtrace was
        // supplied.
        int status = oboe_metadata_fromstr(&omd, xtrace.c_str(), xtrace.length());
        // status can be zero with a version other than 2, so check that too.
        if (status < 0) {
          xtrace = "";
        } else {
          have_metadata = true;
        }
      } else {
        // if it's the wrong length don't pass it to oboe
        xtrace = "";
      }
    }

    v = o.Get("tracestate");
    if (v.IsString()) {
      tracestate = v.As<Napi::String>();
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

    // now handle x-trace-options and x-trace-options-signature
    v = o.Get("typeRequested");
    if (v.IsNumber()) {
      type_requested = v.As<Napi::Number>().Int64Value();
    }
    v = o.Get("xtraceOpts");
    if (v.IsString()) {
      xtraceOpts = v.As<Napi::String>();
    }
    v = o.Get("xtraceOptsSig");
    if (v.IsString()) {
      xtraceOptsSig = v.As<Napi::String>();
    }
    v = o.Get("xtraceOptsTimestamp");
    if (v.IsNumber()) {
      xtraceOptsTimestamp = v.As<Napi::Number>().Int64Value();
    }
    v = o.Get("customTriggerMode");
    if (v.IsNumber()) {
      customTriggerMode = v.As<Napi::Number>().Int32Value();
    }
  }

  // apply default or user specified values.

  // for traceparent: type is 1 and version is 0
  // for xtrace: type is 0 and version is 2
  if (omd.type == 0) {
    in.version = 2;
  } else {
    in.version = 3;
  }

  in.service_name = "";
  in.in_xtrace = xtrace.c_str();
  in.tracestate = tracestate.c_str();
  in.custom_sample_rate = rate;
  in.custom_tracing_mode = mode;

  // v2 fields (added for trigger-trace support)
  in.custom_trigger_mode = customTriggerMode;
  in.request_type = type_requested;
  in.header_options = xtraceOpts.c_str();
  in.header_signature = xtraceOptsSig.c_str();
  in.header_timestamp = xtraceOptsTimestamp;

  // ask for oboe's decisions on life, the universe, and everything.
  out.version = 3;
  int status = oboe_tracing_decisions(&in, &out);

  // version 2+ of the oboe_tracing_decisions_out structure returns a
  // pointer to the message string for all codes.
  //
  // -2 tracing-mode-disabled
  // -1 xtrace-not-sampled
  // 0 ok


  // set the message and auth info for both error and successful returns
  Napi::Object o = Napi::Object::New(env);
  o.Set("status", Napi::Number::New(env, status));
  o.Set("message", Napi::String::New(env, out.status_message));
  o.Set("authStatus", Napi::Number::New(env, out.auth_status));
  o.Set("authMessage", Napi::String::New(env, out.auth_message));
  o.Set("typeProvisioned", Napi::Number::New(env, out.request_provisioned));

  // status > 0 is an error return; do no additional processing.
  if (status > 0) {
    return o;
  }

  // if an x-trace was not used by oboe to make the decision then
  // create metadata. oboe sets sample_source to -1 when it was a
  // "continue" decision, i.e., the trace was continued using the
  // supplied x-trace (no trace decision was made).
  have_metadata = out.sample_source == OBOE_SAMPLE_RATE_SOURCE_CONTINUED;
  if (!have_metadata) {
    edge = false;
    oboe_metadata_init(&omd);
    oboe_metadata_random(&omd);
  }

  // now we have oboe_metadata_t either from a supplied xtrace id or from
  // a Metadata object created for this span. set the sample bit to match
  // the sample decision and create a JavaScript Metadata instance.
  if (out.do_sample) {
    omd.flags |= XTR_FLAGS_SAMPLED;
  } else {
    omd.flags &= ~XTR_FLAGS_SAMPLED;
  }

  Napi::Object event = Event::makeFromOboeMetadata(env, omd);
  //Napi::Value v = Napi::External<oboe_metadata_t>::New(env, &omd);
  //Napi::Object md = Metadata::NewInstance(env, v);

  // oboe sets these if not continued, for when that change will be made
  //out->sample_rate = -1;
  //out->sample_source = -1;
  //out->token_bucket_rate = -1;
  //out->token_bucket_capacity = -1;

  // augment the return object
  o.Set("metadata", event);
  o.Set("metadataFromXtrace", Napi::Boolean::New(env, have_metadata));
  //o.Set("status", Napi::Number::New(env, status));
  o.Set("edge", Napi::Boolean::New(env, edge));
  o.Set("doSample", Napi::Boolean::New(env, out.do_sample));
  o.Set("doMetrics", Napi::Boolean::New(env, out.do_metrics));
  o.Set("source", Napi::Number::New(env, out.sample_source));
  o.Set("rate", Napi::Number::New(env, out.sample_rate));
  o.Set("tokenBucketRate", Napi::Number::New(env, out.token_bucket_rate));
  o.Set("tokenBucketCapacity", Napi::Number::New(env, out.token_bucket_capacity));

  return o;
}

//
// This is not a class, just a group of functions in a JavaScript namespace.
// (well, in two javascript namespaces for compatability.)
//
namespace Settings {

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Object module = Napi::Object::New(env);

  module.Set("setTracingMode", Napi::Function::New(env, setTracingMode));
  module.Set("setDefaultSampleRate", Napi::Function::New(env, setDefaultSampleRate));

  module.Set("getTraceSettings", Napi::Function::New(env, getTraceSettings));

  exports.Set("Settings", module);

  //
  // for legacy compatibility, keep the Context namespace valid
  //
  exports.Set("Context", module);

  return exports;
}

} // end namespace Settings
