#include "bindings.h"
#include <cmath>

/**
 * Set the tracing mode.
 *
 * @param newMode One of
 * - OBOE_TRACE_NEVER(0) to disable tracing,
 * - OBOE_TRACE_ALWAYS(1) to start a new trace if needed
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
 * This rate is used until overridden by the AppOptics servers.  If not set then
 * oboe supplies a default value (300,000, i.e., 30%) at time of this writing.
 *
 * The rate is interpreted as a ratio out of OBOE_SAMPLE_RESOLUTION (1,000,000).
 *
 * @param newRate A number between 0 (none) and OBOE_SAMPLE_RESOLUTION (a million)
 */
NAN_METHOD(OboeContext::setDefaultSampleRate) {
    // presume failure
    int rateUsed = -1;

    // Validate arguments, if bad return -1 (impossible rate)
    if (info.Length() == 1 && info[0]->IsNumber()) {
        double check = info[0]->NumberValue();
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

    info.GetReturnValue().Set(Nan::New(rateUsed));
}

/**
 * Check if the current request should be traced based on the current settings.
 *
 * If xtrace is empty, or if it is not valid then it will be considered a
 * new trace. Otherwise sampling will add to the existing trace.
 * Different layers may have special rules.
 *
 * This is designed to be called once per request at the entry layer.
 *
 * @param layer Name of the layer being considered for tracing
 * @param in_xtrace Incoming X-Trace ID (NULL or empty string if not present)
 * @return {Object} {sample, source, rate}
 */
NAN_METHOD(OboeContext::sampleTrace) {
  // Validate arguments
  if (info.Length() < 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }
  if (!info[0]->IsString()) {
    return Nan::ThrowTypeError("Layer name must be a string");
  }

  std::string layer_name;
  std::string in_xtrace;

  layer_name = *Nan::Utf8String(info[0]);

  // If the second argument is present, it must be a string
  // TODO even though oboe requires a string this could accept
  // any form of metadata (i.e., string, metadata, or event).
  if (info.Length() >= 2) {
    if (!info[1]->IsString()) {
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

  Metadata* metadata;

  metadata = Metadata::getMetadata(info[0]);

  if (metadata == NULL) {
      return Nan::ThrowTypeError("Invalid argument to Context::set()");
  }

  // Set the context
  oboe_context_set(&metadata->metadata);

  delete metadata;
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
// Extended method to create events. Replaces createEvent but allows
// an argument of the metadata to use to create the event. With no
// argument it uses oboe's context as the metadata.
//
NAN_METHOD(OboeContext::createEventX) {
    Metadata* md;
    bool add_edge = true;

    //
    // One signature gets oboe's thread-specific context and uses that to
    // create a new event. The other signature supplies the metadata.
    //
    if (info.Length() == 0) {
        md = new Metadata(oboe_context_get());
    } else {
        md = Metadata::getMetadata(info[0]);
    }

    if (md == NULL) {
        return Nan::ThrowError("Invalid argument for createEventX()");
    }

    if (info.Length() >= 2) {
        add_edge = info[1]->BooleanValue();
    }

    v8::Local<v8::Object> event = Event::NewInstance(md, add_edge);
    delete md;

    Event* e = Nan::ObjectWrap::Unwrap<Event>(event);
    if (e->oboe_status != 0) {
        return Nan::ThrowError("Oboe failed to create event");
    }

    info.GetReturnValue().Set(event);
}

/**
 * JavaScript callable method to create an event with the sample bit
 * set as specified by the optional argument.
 */
NAN_METHOD(OboeContext::startTrace) {
    bool sample = false;
    if (info.Length() == 1) {
        sample = info[0]->BooleanValue();
    }

    oboe_metadata_t *md = oboe_context_get();
    oboe_metadata_random(md);

    if (sample) {
        md->flags |= XTR_FLAGS_SAMPLED;
    } else {
        md->flags &= ~XTR_FLAGS_SAMPLED;
    }

    v8::Local<v8::Object> event = Event::NewInstance();

    Event *e = Nan::ObjectWrap::Unwrap<Event>(event);
    if (e->oboe_status != 0) {
        return Nan::ThrowError("Oboe failed to create event");
    }

    info.GetReturnValue().Set(event);
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
