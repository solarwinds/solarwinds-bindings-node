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
  //if (mode < 0 || mode > 2) {
  if (mode != OBOE_TRACE_NEVER && mode != OBOE_TRACE_ALWAYS) {
    return Nan::ThrowRangeError("Invalid tracing mode");
  }

  oboe_settings_mode_set(mode);
}

/**
 * Set the default sample rate.
 *
 * This rate is used until overridden by the AppOptics servers.  If not set then the
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

  oboe_settings_rate_set(rate);
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
 * @return {Array} [sample, sampleRateUsed, sampleSource] sample is truthy if it should be sampled.
 */
NAN_METHOD(OboeContext::sampleTrace) {
  // Validate arguments
  if (info.Length() < 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }

  std::string layer_name;
  std::string in_xtrace;

  // The first argument must be a string
  if (!info[0]->IsString()) {
    return Nan::ThrowTypeError("Layer name must be a string");
  }
  layer_name = *Nan::Utf8String(info[0]);

  // If the second argument is present, it must be a string
  // TODO BAM - why can't it be an object, i.e., metadata?
  // BAM - because oboe wants a string here. but still, this
  // should accept either, formatting it if necessary.
  if (info.Length() >= 2) {
    if ( ! info[1]->IsString()) {
      return Nan::ThrowTypeError("X-Trace ID must be a string");
    }
    in_xtrace = *Nan::Utf8String(info[1]);
  }

  int sample_rate = 0;
  int sample_source = 0;
  int rc = oboe_sample_layer(
    layer_name.c_str(),
    in_xtrace.c_str(),
    &sample_rate,
    &sample_source
  );

  // Store rc, sample_source and sample_rate in an array
  v8::Local<v8::Array> array = Nan::New<v8::Array>(3);
  Nan::Set(array, 0, Nan::New(rc));
  Nan::Set(array, 1, Nan::New(sample_source));
  Nan::Set(array, 2, Nan::New(sample_rate));

  info.GetReturnValue().Set(array);
}

// Serialize a metadata object to a string
NAN_METHOD(OboeContext::toString) {
  char buf[OBOE_MAX_METADATA_PACK_LEN];
  oboe_metadata_t* md = oboe_context_get();

  int rc;
  // for now any argument counts. maybe accept 'A' or 'a'?
  if (info.Length() == 1) {
    rc = Metadata::format(md, sizeof(buf), buf) ? 0 : -1;
  } else {
    rc = oboe_metadata_tostr(md, buf, sizeof(buf) - 1);
  }

  info.GetReturnValue().Set(Nan::New(rc == 0 ? buf : "").ToLocalChecked());
}


NAN_METHOD(OboeContext::set) {
  // Validate arguments
  if (info.Length() != 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }
  // TODO BAM validate that it is Metadata,  not just object.
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
    Nan::Utf8String val(info[0]);

    // Set the context data from the converted string
    int status = oboe_context_set_fromstr(*val, val.length());
    if (status != 0) {
      return Nan::ThrowError("Could not set context by metadata string id");
    }
  }
}

NAN_METHOD(OboeContext::copy) {
  Metadata* md = new Metadata(oboe_context_get());
  info.GetReturnValue().Set(Metadata::NewInstance(md));
  delete md;
}

NAN_METHOD(OboeContext::clear) {
  oboe_context_clear();
}

NAN_METHOD(OboeContext::isValid) {
  info.GetReturnValue().Set(Nan::New<v8::Boolean>(oboe_context_is_valid()));
}

NAN_METHOD(OboeContext::createEvent) {
  Metadata* md = new Metadata(oboe_context_get());
  info.GetReturnValue().Set(Event::NewInstance(md));
  delete md;
}

NAN_METHOD(OboeContext::startTrace) {
  oboe_metadata_t* md = oboe_context_get();
  oboe_metadata_random(md);
  info.GetReturnValue().Set(Event::NewInstance());
}

void OboeContext::Init(v8::Local<v8::Object> module) {
  Nan::HandleScope scope;

  v8::Local<v8::Object> exports = Nan::New<v8::Object>();
  Nan::SetMethod(exports, "setTracingMode", OboeContext::setTracingMode);
  Nan::SetMethod(exports, "setDefaultSampleRate", OboeContext::setDefaultSampleRate);
  Nan::SetMethod(exports, "sampleTrace", OboeContext::sampleTrace);
  Nan::SetMethod(exports, "toString", OboeContext::toString);
  Nan::SetMethod(exports, "set", OboeContext::set);
  Nan::SetMethod(exports, "copy", OboeContext::copy);
  Nan::SetMethod(exports, "clear", OboeContext::clear);
  Nan::SetMethod(exports, "isValid", OboeContext::isValid);
  Nan::SetMethod(exports, "createEvent", OboeContext::createEvent);
  Nan::SetMethod(exports, "startTrace", OboeContext::startTrace);

  Nan::Set(module, Nan::New("Context").ToLocalChecked(), exports);
}
