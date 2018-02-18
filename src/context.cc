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
  if (rate < 0) {
      rate = 0;
  } else if (rate > OBOE_SAMPLE_RESOLUTION) {
      rate = OBOE_SAMPLE_RESOLUTION;
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
 * This is designed to be called once per request at the entry layer.
 *
 * @param layer Name of the layer being considered for tracing
 * @param in_xtrace Incoming X-Trace ID (NULL or empty string if not present)
 * @return {Object} {sample, source, rate}
 * TODO BAM update comments when done
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

  v8::Local<v8::String> sample = Nan::New<v8::String>("sample").ToLocalChecked();
  v8::Local<v8::String> source = Nan::New<v8::String>("source").ToLocalChecked();
  v8::Local<v8::String> rate = Nan::New<v8::String>("rate").ToLocalChecked();

  v8::Local<v8::Object> o = Nan::New<v8::Object>();
  Nan::Set(o, sample, Nan::New<v8::Boolean>(rc));
  Nan::Set(o, source, Nan::New<v8::Number>(sample_source));
  Nan::Set(o, rate, Nan::New<v8::Number>(sample_rate));

  info.GetReturnValue().Set(o);
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
  // TODO BAM this exact sequence is also used in createEventX. Refactor.
  Metadata* metadata;

  if (Metadata::isMetadata(info[0])) {
    // it's a metadata instance
    Metadata* md = Nan::ObjectWrap::Unwrap<Metadata>(info[0]->ToObject());
    metadata = new Metadata(&md->metadata);
  } else if (Event::isEvent(info[0])) {
    // it's an event instance
    Event* e = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());
    metadata = new Metadata(&e->event.metadata);
  } else if (info[0]->IsString()) {
    // it's a string, this can fail and return undefined.
    // TODO BAM duplicates code in metadata.cc - refactor to C++ callable method?
    Nan::Utf8String str(info[0]);

    oboe_metadata_t md;
    int status = oboe_metadata_fromstr(&md, *str, str.length());
    if (status < 0) {
      info.GetReturnValue().Set(Nan::Undefined());
      return;
    }
    metadata = new Metadata(&md);
  } else {
    return Nan::ThrowTypeError("Invalid argument for Context::set");
  }

  // Set the context
  oboe_context_set(&metadata->metadata);

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

//
// Extended method to create events. Replaces startTrace and
// createEvent. Accepts an argument
//
NAN_METHOD(OboeContext::createEventX) {
  //
  // This signature gets oboe's thread-specific context and uses that to
  // create a new event. It will create blank metadata (i.e., the IDs
  // will be 0, not random data).
  //
  if (info.Length() == 0) {
    Metadata* md = new Metadata(oboe_context_get());
    info.GetReturnValue().Set(Event::NewInstance(md));
    delete md;
    return;
  }

  //
  // there is at least one argument. it must be metadata in some form.
  //
  Metadata* metadata;

  if (Metadata::isMetadata(info[0])) {
    // it's a metadata instance
    Metadata* md = Nan::ObjectWrap::Unwrap<Metadata>(info[0]->ToObject());
    metadata = new Metadata(&md->metadata);
  } else if (Event::isEvent(info[0])) {
    // it's an event instance
    Event* e = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());
    metadata = new Metadata(&e->event.metadata);
  } else if (info[0]->IsString()) {
    // it's a string, this can fail and return undefined.
    // TODO BAM duplicates code in metadata.cc - refactor to C++ callable method.
    Nan::Utf8String str(info[0]);

    oboe_metadata_t md;
    int status = oboe_metadata_fromstr(&md, *str, str.length());
    if (status < 0) {
      info.GetReturnValue().Set(Nan::Undefined());
      return;
    }
    metadata = new Metadata(&md);
  } else {
    return Nan::ThrowError("Invalid argument for createEventX()");
  }

  info.GetReturnValue().Set(Event::NewInstance(metadata));
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
  Nan::SetMethod(exports, "createEventX", OboeContext::createEventX);
  Nan::SetMethod(exports, "startTrace", OboeContext::startTrace);

  Nan::Set(module, Nan::New("Context").ToLocalChecked(), exports);
}
