#include "bindings.h"

/**
 * Set the tracing mode.
 *
 * @param newMode One of
 * - OBOE_TRACE_NEVER(0) to disable tracing,
 * - OBOE_TRACE_ALWAYS(1) to start a new trace if needed, or
 * - OBOE_TRACE_THROUGH(2) to only add to an existing trace.
 */
NAN_METHOD(OboeContext::setTracingMode) {
  // Validate arguments
  if (info.Length() != 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }
  if (!info[0]->IsNumber()) {
    return Nan::ThrowTypeError("Tracing mode must be a number");
  }

  int mode = info[0]->NumberValue();
  if (mode < 0 || mode > 2) {
    return Nan::ThrowRangeError("Invalid tracing mode");
  }

  oboe_settings_cfg_tracing_mode_set(mode);
}

/**
 * Set the default sample rate.
 *
 * This rate is used until overridden by the TraceView servers.  If not set then the
 * value 300,000 will be used (ie. 30%).
 *
 * The rate is interpreted as a ratio out of OBOE_SAMPLE_RESOLUTION (currently 1,000,000).
 *
 * @param newRate A number between 0 (none) and OBOE_SAMPLE_RESOLUTION (a million)
 */
NAN_METHOD(OboeContext::setDefaultSampleRate) {
  // Validate arguments
  if (info.Length() != 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }
  if (!info[0]->IsNumber()) {
    return Nan::ThrowTypeError("Sample rate must be a number");
  }

  int rate = info[0]->NumberValue();
  if (rate < 1 || rate > OBOE_SAMPLE_RESOLUTION) {
    return Nan::ThrowRangeError("Sample rate out of range");
  }

  oboe_settings_cfg_sample_rate_set(rate);
}

/**
 * Check if the current request should be traced based on the current settings.
 *
 * If xtrace is empty, or if it is identified as a foreign (ie. cross customer)
 * trace, then sampling will be considered as a new trace.
 * Otherwise sampling will be considered as adding to the current trace.
 * Different layers may have special rules.  Also special rules for AppView
 * Web synthetic traces apply if in_tv_meta is given a non-empty string.
 *
 * This is designed to be called once per layer per request.
 *
 * @param layer Name of the layer being considered for tracing
 * @param in_xtrace Incoming X-Trace ID (NULL or empty string if not present)
 * @param in_tv_meta AppView Web ID from X-TV-Meta HTTP header or higher layer (NULL or empty string if not present).
 * @return Zero to not trace; otherwise return the sample rate used in the low order
 *         bytes 0 to 2 and the sample source in the higher-order byte 3.
 */
NAN_METHOD(OboeContext::sampleRequest) {
  // Validate arguments
  if (info.Length() < 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }

  char* layer_name;
  char* in_xtrace;
  char* in_tv_meta;

  // The first argument must be a string
  if (!info[0]->IsString()) {
    return Nan::ThrowTypeError("Layer name must be a string");
  }
  Nan::Utf8String layer_name_v8(info[0]);
  layer_name = *layer_name_v8;

  // If the second argument is present, it must be a string
  if (info.Length() >= 2) {
    if ( ! info[1]->IsString()) {
      return Nan::ThrowTypeError("X-Trace ID must be a string");
    }
    Nan::Utf8String in_xtrace_v8(info[1]);
    in_xtrace = *in_xtrace_v8;
  } else {
    in_xtrace = strdup("");
  }

  // If the third argument is present, it must be a string
  if (info.Length() >= 3) {
    if ( ! info[2]->IsString()) {
      return Nan::ThrowTypeError("AppView Web ID must be a string");
    }
    Nan::Utf8String in_tv_meta_v8(info[2]);
    in_tv_meta = *in_tv_meta_v8;
  } else {
    in_tv_meta = strdup("");
  }

  int sample_rate = 0;
  int sample_source = 0;
  int rc = oboe_sample_layer(
    layer_name,
    in_xtrace,
    in_tv_meta,
    &sample_rate,
    &sample_source
  );

  // Store rc, sample_source and sample_rate in an array
  v8::Local<v8::Array> array = Nan::New<v8::Array>(2);
  Nan::Set(array, 0, Nan::New(rc));
  Nan::Set(array, 1, Nan::New(sample_source));
  Nan::Set(array, 2, Nan::New(sample_rate));

  info.GetReturnValue().Set(array);
}

// returns pointer to current context (from thread-local storage)
oboe_metadata_t* OboeContext::get() {
  return oboe_context_get();
}

NAN_METHOD(OboeContext::toString) {
  char buf[OBOE_MAX_METADATA_PACK_LEN];

  oboe_metadata_t *md = OboeContext::get();
  int rc = oboe_metadata_tostr(md, buf, sizeof(buf) - 1);
  if (rc == 0) {
    info.GetReturnValue().Set(Nan::New(buf).ToLocalChecked());
  } else {
    info.GetReturnValue().Set(Nan::New("").ToLocalChecked());
  }
}

NAN_METHOD(OboeContext::set) {
  // Validate arguments
  if (info.Length() != 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }
  if (!info[0]->IsObject() && !info[0]->IsString()) {
    return Nan::ThrowTypeError("You must supply a Metadata instance or string");
  }

  if (info[0]->IsObject()) {
    // Unwrap metadata instance from arguments
    Metadata* metadata = Nan::ObjectWrap::Unwrap<Metadata>(info[0]->ToObject());

    // Set the context data from the metadata instance
    oboe_context_set(&metadata->metadata);
  } else {
    // Get string data from arguments
    Nan::Utf8String v8_val(info[0]);
    std::string val(*v8_val);

    // Set the context data from the converted string
    int status = oboe_context_set_fromstr(val.data(), val.size());
    if (status != 0) {
      return Nan::ThrowError("Could not set context by metadata string id");
    }
  }
}

NAN_METHOD(OboeContext::copy) {
  // Make an empty object template with space for internal field pointers
  v8::Local<v8::ObjectTemplate> t = Nan::New<v8::ObjectTemplate>();
  t->SetInternalFieldCount(2);

  // Construct an object with our internal field pointer
  v8::Local<v8::Object> obj = t->NewInstance();

  // Attach the internal field pointer
  Nan::SetInternalFieldPointer(obj, 1, OboeContext::get());

  // Use the object as an argument in the event constructor
  info.GetReturnValue().Set(Metadata::NewInstance(obj));
}

NAN_METHOD(OboeContext::clear) {
  oboe_context_clear();
}

NAN_METHOD(OboeContext::isValid) {
  info.GetReturnValue().Set(Nan::New<v8::Boolean>(oboe_context_is_valid()));
}

NAN_METHOD(OboeContext::createEvent) {
  // Make an empty object template with space for internal field pointers
  v8::Local<v8::ObjectTemplate> t = Nan::New<v8::ObjectTemplate>();
  t->SetInternalFieldCount(2);

  // Construct an object with our internal field pointer
  v8::Local<v8::Object> obj = t->NewInstance();

  // Attach the internal field pointer
  Nan::SetInternalFieldPointer(obj, 1, OboeContext::get());

  // Use the object as an argument in the event constructor
  info.GetReturnValue().Set(Event::NewInstance(obj));
}

NAN_METHOD(OboeContext::startTrace) {
  oboe_metadata_t* md = OboeContext::get();
  oboe_metadata_random(md);
  info.GetReturnValue().Set(Event::NewInstance());
}

void OboeContext::Init(v8::Local<v8::Object> module) {
  Nan::HandleScope scope;

  v8::Local<v8::Object> exports = Nan::New<v8::Object>();
  Nan::SetMethod(exports, "setTracingMode", OboeContext::setTracingMode);
  Nan::SetMethod(exports, "setDefaultSampleRate", OboeContext::setDefaultSampleRate);
  Nan::SetMethod(exports, "sampleRequest", OboeContext::sampleRequest);
  Nan::SetMethod(exports, "toString", OboeContext::toString);
  Nan::SetMethod(exports, "set", OboeContext::set);
  Nan::SetMethod(exports, "copy", OboeContext::copy);
  Nan::SetMethod(exports, "clear", OboeContext::clear);
  Nan::SetMethod(exports, "isValid", OboeContext::isValid);
  Nan::SetMethod(exports, "createEvent", OboeContext::createEvent);
  Nan::SetMethod(exports, "startTrace", OboeContext::startTrace);

  Nan::Set(module, Nan::New("Context").ToLocalChecked(), exports);
}
