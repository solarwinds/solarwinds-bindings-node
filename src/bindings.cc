#include "bindings.h"

//
// Initialize oboe
//
Napi::Value oboeInit(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "invalid calling signature").ThrowAsJavaScriptException();
        return env.Null();
    }
    // get the options settings object
    Napi::Object o = info[0].ToObject();

    // if there is a second argument and it's an object store information about what was
    // processed in it. it can also cause the actual oboe_init() call to be skipped.
    bool skipInit = false;
    Napi::Object oo;
    Napi::Object processed = Napi::Object::New(env);
    Napi::Object valid = Napi::Object::New(env);
    if (info.Length() == 2 && info[1].IsObject()) {
      oo = info[1].ToObject();
      oo.Set("processed", processed);
      oo.Set("valid", valid);
      if (oo.Has("skipInit")) {
        skipInit = oo.Get("skipInit").ToBoolean().Value();
      }
    }

    // setup oboe's options structure
    oboe_init_options_t options;
    options.version = 13;

    int setDefaultsStatus = oboe_init_options_set_defaults(&options);
    if (setDefaultsStatus > 0) {
      Napi::RangeError::New(env, "setting init option defaults failed").ThrowAsJavaScriptException();
      return env.Null();
    }

    // a place to save the strings so they won't go out of scope. using the
    // number of properties is more than will ever be needed because neither
    // booleans nor integers are stored.
    Napi::Array keys = o.GetPropertyNames();
    std::string fill;
    std::vector<std::string> holdKeys(keys.Length(), fill);
    int kix = -1;

    //
    // for each valid field see if a value was provided.
    //
    if (o.Has("hostnameAlias")) {
      Napi::Value hostnameAlias = o.Get("hostnameAlias");
      processed.Set("hostnameAlias", hostnameAlias);
      if (hostnameAlias.IsString()) {
        valid.Set("hostnameAlias", hostnameAlias);
        holdKeys[++kix] = hostnameAlias.ToString();
        options.hostname_alias = holdKeys[kix].c_str();
      }
    }
    if (o.Has("logLevel")) {
      Napi::Value logLevel = o.Get("logLevel");
      processed.Set("logLevel", logLevel);
      if (logLevel.IsNumber()) {
        valid.Set("logLevel", logLevel);
        options.log_level = logLevel.ToNumber().Int64Value();
      }
    }
    //if (o.Has("logFilePath")) {
    //  Napi::Value logFilePath = o.Get("logFilePath");
    //  processed.Set("logFilePath", logFilePath);
    //  if (logFilePath.IsString()) {
    //    valid.Set("logFilePath", logFilePath);
    //    holdKeys[++kix] = logFilePath.ToString();
    //    options.log_file_path = holdKeys[kix].c_str();
    //  }
    //}
    if (o.Has("maxTransactions")) {
      Napi::Value maxTransactions = o.Get("maxTransactions");
      processed.Set("maxTransactions", maxTransactions);
      if (maxTransactions.IsNumber()) {
        valid.Set("maxTransactions", maxTransactions);
        options.max_transactions = maxTransactions.ToNumber().Int64Value();
      }
    }
    if (o.Has("maxFlushWaitTime")) {
      Napi::Value maxFlushWaitTime = o.Get("maxFlushWaitTime");
      processed.Set("maxFlushWaitTime", maxFlushWaitTime);
      if (maxFlushWaitTime.IsNumber()) {
        valid.Set("maxFlushWaitTime", maxFlushWaitTime);
        options.max_flush_wait_time = maxFlushWaitTime.ToNumber().Int64Value();
      }
    }
    if (o.Has("eventsFlushInterval")) {
      Napi::Value eventsFlushInterval = o.Get("eventsFlushInterval");
      processed.Set("eventsFlushInterval", eventsFlushInterval);
      if (eventsFlushInterval.IsNumber()) {
        valid.Set("eventsFlushInterval", eventsFlushInterval);
        options.events_flush_interval = eventsFlushInterval.ToNumber().Int64Value();
      }
    }
    if (o.Has("eventsFlushBatchSize")) {
      Napi::Value eventsFlushBatchSize = o.Get("eventsFlushBatchSize");
      processed.Set("eventsFlushBatchSize", eventsFlushBatchSize);
      if (eventsFlushBatchSize.IsNumber()) {
        valid.Set("eventsFlushBatchSize", eventsFlushBatchSize);
        options.max_request_size_bytes = eventsFlushBatchSize.ToNumber().Int64Value();
      }
    }
    if (o.Has("reporter")) {
      Napi::Value reporter = o.Get("reporter");
      processed.Set("reporter", reporter);
      if (reporter.IsString()) {
        valid.Set("reporter", reporter);
        holdKeys[++kix] = reporter.ToString();
        options.reporter = holdKeys[kix].c_str();
      }
    }
    // endpoint maps to "host" field.
    if (o.Has("endpoint")) {
      Napi::Value endpoint = o.Get("endpoint");
      processed.Set("endpoint", endpoint);
      if (endpoint.IsString()) {
        valid.Set("endpoint", endpoint);
        holdKeys[++kix] = endpoint.ToString();
        options.host = holdKeys[kix].c_str();
      }
    }
    // is the service key
    if (o.Has("serviceKey")) {
      Napi::Value serviceKey = o.Get("serviceKey");
      processed.Set("serviceKey", serviceKey);
      if (serviceKey.IsString()) {
        valid.Set("serviceKey", serviceKey);
        holdKeys[++kix] = serviceKey.ToString();
        options.service_key = holdKeys[kix].c_str();
      }
    }
    if (o.Has("trustedPath")) {
      Napi::Value trustedPath = o.Get("trustedPath");
      processed.Set("trustedPath", trustedPath);
      if (trustedPath.IsString()) {
        valid.Set("trustedPath", trustedPath);
        holdKeys[++kix] = trustedPath.ToString();
        options.trusted_path = holdKeys[kix].c_str();
      }
    }
    if (o.Has("bufferSize")) {
      Napi::Value bufferSize = o.Get("bufferSize");
      processed.Set("bufferSize", bufferSize);
      if (bufferSize.IsNumber()) {
        valid.Set("bufferSize", bufferSize);
        options.buffer_size = bufferSize.ToNumber().Int64Value();
      }
    }
    if (o.Has("traceMetrics")) {
      Napi::Value traceMetrics = o.Get("traceMetrics");
      processed.Set("traceMetrics", traceMetrics);
      valid.Set("traceMetrics", traceMetrics);
      options.trace_metrics = traceMetrics.ToBoolean().Value();
    }
    if (o.Has("histogramPrecision")) {
      Napi::Value histogramPrecision = o.Get("histogramPrecision");
      processed.Set("histogramPrecision", histogramPrecision);
      if (histogramPrecision.IsNumber()) {
        valid.Set("histogramPrecision", histogramPrecision);
        options.histogram_precision = histogramPrecision.ToNumber().Int64Value();
      }
    }
    if (o.Has("tokenBucketCapacity")) {
      Napi::Value tokenBucketCapacity = o.Get("tokenBucketCapacity");
      processed.Set("tokenBucketCapacity", tokenBucketCapacity);
      if (tokenBucketCapacity.IsNumber()) {
        valid.Set("tokenBucketCapacity", tokenBucketCapacity);
        options.token_bucket_capacity = tokenBucketCapacity.ToNumber().DoubleValue();
      }
    }
    if (o.Has("tokenBucketRate")) {
      Napi::Value tokenBucketRate = o.Get("tokenBucketRate");
      processed.Set("tokenBucketRate", tokenBucketRate);
      if (tokenBucketRate.IsNumber()) {
        valid.Set("tokenBucketRate", tokenBucketRate);
        options.token_bucket_rate = tokenBucketRate.ToNumber().DoubleValue();
      }
    }
    // oneFilePerEvent maps to "file_single" field.
    //if (o.Has("oneFilePerEvent")) {
    //  Napi::Value oneFilePerEvent = o.Get("oneFilePerEvent");
    //  processed.Set("oneFilePerEvent", oneFilePerEvent);
    //  valid.Set("oneFilePerEvent", oneFilePerEvent);
    //  options.file_single = oneFilePerEvent.ToBoolean().Value();
    //}
    if (o.Has("ec2MetadataTimeout")) {
      Napi::Value ec2MetadataTimeout = o.Get("ec2MetadataTimeout");
      processed.Set("ec2MetadataTimeout", ec2MetadataTimeout);
      if (ec2MetadataTimeout.IsNumber()) {
        valid.Set("ec2MetadataTimeout", ec2MetadataTimeout);
        options.ec2_metadata_timeout =  ec2MetadataTimeout.ToNumber().Int32Value();
      }
    }
    if (o.Has("proxy")) {
      Napi::Value proxy = o.Get("proxy");
      processed.Set("proxy", proxy);
      if (proxy.IsString()) {
        valid.Set("proxy", proxy);
        holdKeys[++kix] = proxy.ToString();
        options.proxy = holdKeys[kix].c_str();
      }
    }
    if (o.Has("stdoutClearNonblocking")) {
      Napi::Value stdoutClearNonblocking = o.Get("stdoutClearNonblocking");
      processed.Set("stdoutClearNonblocking", stdoutClearNonblocking);
      if (stdoutClearNonblocking.IsNumber()) {
        valid.Set("stdoutClearNonblocking", stdoutClearNonblocking);
        options.stdout_clear_nonblocking = stdoutClearNonblocking.ToNumber().Int32Value();
      }
    }
    if (o.Has("mode")) {
      Napi::Value mode = o.Get("mode");
      processed.Set("mode", mode);
      if (mode.IsNumber()) {
        valid.Set("mode", mode);
        options.mode = mode.ToNumber().Int32Value();
      }
    }

    if (skipInit) {
      return env.Null();
    }

    // initialize oboe
    int result = oboe_init(&options);
    return Napi::Number::New(env, result);
}

//
// Check to see if oboe is ready to issue sampling decisions.
//
// returns coded status as below
//
Napi::Value isReadyToSample(const Napi::CallbackInfo& info) {
  int ms = 0;  // milliseconds to wait
  if (info[0].IsNumber()) {
    ms = info[0].As<Napi::Number>().Int64Value();
  }

  int status;
  status = oboe_is_ready(ms);

  // UNKNOWN 0
  // OK 1
  // TRY_LATER 2
  // LIMIT_EXCEEDED 3
  // unused 4 (was INVALID_API_KEY)
  // CONNECT_ERROR 5
  return Napi::Number::New(info.Env(), status);
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
  exports.Set("isReadyToSample", Napi::Function::New(env, isReadyToSample));
  exports.Set("o", Napi::Function::New(env, o));

  // classes and objects supplying different namespaces
  exports = Reporter::Init(env, exports);
  exports = Settings::Init(env, exports);
  exports = Sanitizer::Init(env, exports);
  exports = Event::Init(env, exports);
  exports = Config::Init(env, exports);

  return exports;

}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
