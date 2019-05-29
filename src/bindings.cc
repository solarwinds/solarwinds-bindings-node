#include "bindings.h"

// Components
#include "sanitizer.cc"
#include "metadata.cc"
#include "context.cc"
#include "config.cc"
#include "event.cc"
#include "reporter.cc"

//
// Initialize oboe
//
Napi::Value oboeInit(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() != 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "invalid calling signature").ThrowAsJavaScriptException();
        return env.Null();
    }

    // get the options object
    Napi::Object o = info[0].ToObject();

    // setup oboe's options structure
    oboe_init_options_t options;
    options.version = 5;
    oboe_init_options_set_defaults(&options);

    // a place to save the strings so they won't go out of scope. using the
    // number of properties is more than will ever be needed because neither
    // booleans nor integers are stored.
    Napi::Array keys = o.GetPropertyNames();
    std::string fill;
    std::vector<std::string> holdKeys(keys.Length(), fill);
    int kix = -1;

    bool debug = o.Get("debug").ToBoolean().Value();
    // make an output object. it's only filled in when debugging.
    Napi::Object oo = Napi::Object::New(env);

    //
    // for each possible field see if a value was provided.
    //
    if (o.Has("hostnameAlias")) {
      Napi::Value hostnameAlias = o.Get("hostnameAlias");
      if (hostnameAlias.IsString()) {
        if (debug)
          oo.Set("hostnameAlias", hostnameAlias);
        holdKeys[++kix] = hostnameAlias.ToString();
        options.hostname_alias = holdKeys[kix].c_str();
      }
    }
    if (o.Has("logLevel")) {
      Napi::Value logLevel = o.Get("logLevel");
      if (logLevel.IsNumber()) {
        if (debug)
          oo.Set("logLevel", logLevel);
        options.log_level = logLevel.ToNumber().Int64Value();
      }
    }
    if (o.Has("logFilePath")) {
      Napi::Value logFilePath = o.Get("logFilePath");
      if (logFilePath.IsString()) {
        if (debug)
          oo.Set("logFilePath", logFilePath);
        holdKeys[++kix] = logFilePath.ToString();
        options.log_file_path = holdKeys[kix].c_str();
      }
    }
    if (o.Has("maxTransactions")) {
      Napi::Value maxTransactions = o.Get("maxTransactions");
      if (maxTransactions.IsNumber()) {
        if (debug)
          oo.Set("maxTransactions", maxTransactions);
        options.max_transactions = maxTransactions.ToNumber().Int64Value();
      }
    }
    if (o.Has("maxFlushWaitTime")) {
      Napi::Value maxFlushWaitTime = o.Get("maxFlushWaitTime");
      if (maxFlushWaitTime.IsNumber()) {
        if (debug)
          oo.Set("maxFlushWaitTime", maxFlushWaitTime);
        options.max_flush_wait_time = maxFlushWaitTime.ToNumber().Int64Value();
      }
    }
    if (o.Has("eventsFlushInterval")) {
      Napi::Value eventsFlushInterval = o.Get("eventsFlushInterval");
      if (eventsFlushInterval.IsNumber()) {
        if (debug)
          oo.Set("eventsFlushInterval", eventsFlushInterval);
        options.events_flush_interval = eventsFlushInterval.ToNumber().Int64Value();
      }
    }
    if (o.Has("eventsFlushBatchSize")) {
      Napi::Value eventsFlushBatchSize = o.Get("eventsFlushBatchSize");
      if (eventsFlushBatchSize.IsNumber()) {
        if (debug)
          oo.Set("eventsFlushBatchSize", eventsFlushBatchSize);
        options.events_flush_batch_size = eventsFlushBatchSize.ToNumber().Int64Value();
      }
    }
    if (o.Has("reporter")) {
      Napi::Value reporter = o.Get("reporter");
      if (reporter.IsString()) {
        if (debug)
          oo.Set("reporter", reporter);
        holdKeys[++kix] = reporter.ToString();
        options.reporter = holdKeys[kix].c_str();
      }
    }
    // endpoint maps to "host" field.
    if (o.Has("endpoint")) {
      Napi::Value endpoint = o.Get("endpoint");
      if (endpoint.IsString()) {
        if (debug)
          oo.Set("endpoint", endpoint);
        holdKeys[++kix] = endpoint.ToString();
        options.host = holdKeys[kix].c_str();
      }
    }
    // is the service key
    if (o.Has("serviceKey")) {
      Napi::Value serviceKey = o.Get("serviceKey");
      if (serviceKey.IsString()) {
        if (debug)
          oo.Set("serviceKey", serviceKey);
        holdKeys[++kix] = serviceKey.ToString();
        options.service_key = holdKeys[kix].c_str();
      }
    }
    if (o.Has("trustedPath")) {
      Napi::Value trustedPath = o.Get("trustedPath");
      if (trustedPath.IsString()) {
        if (debug)
          oo.Set("trustedPath", trustedPath);
        holdKeys[++kix] = trustedPath.ToString();
        options.trusted_path = holdKeys[kix].c_str();
      }
    }
    if (o.Has("bufferSize")) {
      Napi::Value bufferSize = o.Get("bufferSize");
      if (bufferSize.IsNumber()) {
        if (debug)
          oo.Set("bufferSize", bufferSize);
        options.buffer_size = bufferSize.ToNumber().Int64Value();
      }
    }
    if (o.Has("traceMetrics")) {
      Napi::Value traceMetrics = o.Get("traceMetrics");
      options.trace_metrics = traceMetrics.ToBoolean().Value();
      if (debug)
        oo.Set("traceMetrics", traceMetrics.ToBoolean().Value());
    }
    if (o.Has("histogramPrecision")) {
      Napi::Value histogramPrecision = o.Get("histogramPrecision");
      if (histogramPrecision.IsNumber()) {
        if (debug)
          oo.Set("histogramPrecision", histogramPrecision);
        options.histogram_precision = histogramPrecision.ToNumber().Int64Value();
      }
    }
    if (o.Has("tokenBucketCapacity")) {
      Napi::Value tokenBucketCapacity = o.Get("tokenBucketCapacity");
      if (tokenBucketCapacity.IsNumber()) {
        if (debug)
          oo.Set("tokenBucketCapacity", tokenBucketCapacity);
        options.token_bucket_capacity = tokenBucketCapacity.ToNumber().Int64Value();
      }
    }
    if (o.Has("tokenBucketRate")) {
      Napi::Value tokenBucketRate = o.Get("tokenBucketRate");
      if (tokenBucketRate.IsNumber()) {
        if (debug)
          oo.Set("tokenBucketRate", tokenBucketRate);
        options.token_bucket_rate = tokenBucketRate.ToNumber().Int64Value();
      }
    }
    // oneFilePerEvent maps to "file_single" field.
    if (o.Has("oneFilePerEvent")) {
      Napi::Value oneFilePerEvent = o.Get("oneFilePerEvent");
      options.file_single = oneFilePerEvent.ToBoolean().Value();
      if (debug)
        oo.Set("oneFilePerEvent", oneFilePerEvent.ToBoolean().Value());;
    }

    if (debug) {
      return oo;
    }

    // initialize oboe
    int result = oboe_init(&options);
    return Napi::Number::New(env, result);
}

//
// simple utility to output using C++. Sometimes node doesn't
// manage to output console.log() calls before there is a problem.
//
Napi::Value o (const Napi::CallbackInfo& info) {
  if (info.Length() < 1) {
    return info.Env().Undefined();
  }
  for (uint i = 0; i < info.Length(); i++) {
    std::string s = info[i].ToString();
    std::cout << s;
  }
  // return the number of items output
  return Napi::Number::New(info.Env(), info.Length());
}

//
// Expose public parts of this module and invoke the other classes
// and namespace objects own Init() functions.
//
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  // constants
  exports.Set("MAX_SAMPLE_RATE", Napi::Number::New(env, OBOE_SAMPLE_RESOLUTION));
  exports.Set("MAX_METADATA_PACK_LEN", Napi::Number::New(env, OBOE_MAX_METADATA_PACK_LEN));
  exports.Set("MAX_TASK_ID_LEN", Napi::Number::New(env, OBOE_MAX_TASK_ID_LEN));
  exports.Set("MAX_OP_ID_LEN", Napi::Number::New(env, OBOE_MAX_OP_ID_LEN));
  exports.Set("TRACE_NEVER", Napi::Number::New(env, OBOE_TRACE_NEVER));
  exports.Set("TRACE_ALWAYS", Napi::Number::New(env, OBOE_TRACE_ALWAYS));

  // functions directly on the bindings object
  exports.Set("oboeInit", Napi::Function::New(env, oboeInit));
  exports.Set("o", Napi::Function::New(env, o));

  // classes and objects supplying different namespaces
  exports = Reporter::Init(env, exports);
  exports = OboeContext::Init(env, exports);
  exports = Sanitizer::Init(env, exports);
  exports = Metadata::Init(env, exports);
  exports = Event::Init(env, exports);
  exports = Config::Init(env, exports);

  return exports;

}

NODE_API_MODULE(appoptics_bindings, Init)
