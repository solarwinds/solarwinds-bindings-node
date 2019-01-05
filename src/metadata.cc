#include "bindings.h"

Napi::FunctionReference Metadata::constructor;

// currently active
static int metadata_count = 0;
// at destruction - size and average size
static int metadata_deleted_size = 0;
static int metadata_deleted_count = 0;

bool Metadata::sampleFlagIsOn() {
  return metadata.flags & XTR_FLAGS_SAMPLED;
}

/**
 * JavaScript & C++ callable method to create a new Javascript instance
 */
Metadata::Metadata(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Metadata>(info) {
  Napi::Env env = info.Env();

//Napi::Value Metadata::New(const Napi::CallbackInfo& info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(env, "Metadata() must be called as a constructor").ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 1) {
    Napi::TypeError::New(env, "Invalid signature").ThrowAsJavaScriptException();
  }

  // no argument gets empty metadata
  if (info.Length() == 0) {
    return;
  }

  // use a native function to figure out and extract the metadata from one
  // of: Metadata, Event, or String.
  bool ok = Metadata::getMetadata(info[0], &metadata);

  // if it's not any of those it's an error
  if (!ok) {
    Napi::TypeError::New(env, "argument must be Metadata, Event, or a valid X-Trace string");
    return;
  }

}

Metadata::~Metadata() {

}

//
// c++ callable function to
//
Napi::Object Metadata::NewInstance(Napi::Env env, Napi::Value arg) {
    Napi::EscapableHandleScope scope(env);

    Napi::Object o = constructor.New({arg});
    return scope.Escape(napi_value(o)).ToObject();
}

/**
 * JavaScript callable static method to return metadata constructed from the current
 * oboe context.
 */
Napi::Value Metadata::fromContext(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    oboe_metadata_t* omd = oboe_context_get();

    Napi::External<oboe_metadata_t> v = Napi::External<oboe_metadata_t>::New(env, omd);
    Napi::Object md = Metadata::NewInstance(env, v);
    delete omd;
    return md;
}

//
// Transform a string back into a metadata instance
//
Napi::Value Metadata::fromString(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string str = info[0].As<Napi::String>();

  oboe_metadata_t omd;
  int status = oboe_metadata_fromstr(&omd, str.c_str(), str.length());
  // status can be zero with a version other than 2, so check that too.
  if (status < 0 || omd.version != 2) {
    return env.Undefined();
  }

  Napi::External<oboe_metadata_t> v = Napi::External<oboe_metadata_t>::New(env, &omd);
  return Metadata::NewInstance(env, v);
}

//
// Metadata factory for randomized metadata
//
Napi::Value Metadata::makeRandom(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    oboe_metadata_t omd;

    oboe_metadata_init(&omd);
    oboe_metadata_random(&omd);

    // set or clear the sample flag appropriately if an argument specified.
    if (info.Length() == 1) {
        if (info[0].ToBoolean().Value()) {
            omd.flags |= XTR_FLAGS_SAMPLED;
        } else {
            omd.flags &= ~XTR_FLAGS_SAMPLED;
        }
    }

    Napi::External<oboe_metadata_t> v = Napi::External<oboe_metadata_t>::New(env, &omd);

    return Metadata::NewInstance(env, v);
}

//
// Verify that the state of the metadata instance is valid
//
Napi::Value Metadata::isValid(const Napi::CallbackInfo& info) {
    bool status = oboe_metadata_is_valid(&this->metadata);
    return Napi::Boolean::New(info.Env(), status);
}

Napi::Value Metadata::getSampleFlag(const Napi::CallbackInfo& info) {
    return Napi::Boolean::New(info.Env(), this->sampleFlagIsOn());
}

Napi::Value Metadata::setSampleFlagTo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() != 1) {
        Napi::TypeError::New(env, "requires one argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool previous = this->metadata.flags & XTR_FLAGS_SAMPLED;

    // truthy to set it, falsey to clear it
    if (info[0].ToBoolean().Value()) {
        this->metadata.flags |= XTR_FLAGS_SAMPLED;
    } else {
        this->metadata.flags &= ~XTR_FLAGS_SAMPLED;
    }
    return Napi::Boolean::New(env, previous);
}

//
// Serialize a metadata object to a string
//
Napi::Value Metadata::toString(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  char buf[OBOE_MAX_METADATA_PACK_LEN];

  int rc;
  // for now any argument means use our delimited, lowercase format.
  // TODO BAM consider making the args control upper/lower, other?
  if (info.Length() >= 1) {
    rc = format(&this->metadata, sizeof(buf), buf) ? 0 : -1;
  } else {
    rc = oboe_metadata_tostr(&this->metadata, buf, sizeof(buf) - 1);
  }

  return Napi::String::New(env, rc == 0 ? buf : "");
}

//
// C++ internal method to determine if an object is an instance of
// JavaScript Metadata
//
bool Metadata::isMetadata(Napi::Object o) {
  return o.IsObject() && o.InstanceOf(constructor.Value());
}

//
// C++ internal method to copy metadata from Object o. Returns
// represents metadata.
//
bool Metadata::getMetadata(Napi::Value v, oboe_metadata_t* omd) {

    Napi::Object o = v.ToObject();

    if (Metadata::isMetadata(o)) {
        // it's a metadata instanc
        Metadata* md = Napi::ObjectWrap<Metadata>::Unwrap(o);
        *omd = md->metadata;
        //metadata = new OboeMetadata(&md->metadata);
    } else if (Event::isEvent(o)) {
        // it's an event instance
        //Event* e = o->ToObject().Unwrap<Event>();
        Event* e = Napi::ObjectWrap<Event>::Unwrap(o);
        *omd = e->event.metadata;
        //metadata = new OboeMetadata(&e->event.metadata);
    } else if (v.IsString()) {
        // it's a string, this can fail and return false.
        std::string str = v.As<Napi::String>();

        oboe_metadata_t md;
        int status = oboe_metadata_fromstr(&md, str.c_str(), str.length());
        if (status < 0) {
            return false;
        }
        *omd = md;
    } else if (v.IsExternal()) {
      *omd = *v.As<Napi::External<oboe_metadata_t>>().Data();
    } else {
        // it's not something we know how to convert
        return false;
    }

    return true;
}

//
// JavaScript callable static method to determine if the argument
// (event, metadata, or string) has the sample flag turned on.
//
// returns a boolean indicating the state of the sample flag
//
// if a string argument cannot be converted to metadata it
// returns undefined
//
Napi::Value Metadata::sampleFlagIsSet(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool sampleFlag = false;

    // handle each argument type appropriately
    if (info.Length() >= 1) {
        Napi::Object o = info[0].As<Napi::Object>();
        if (Metadata::isMetadata(o)) {
          Metadata* md = Napi::ObjectWrap<Metadata>::Unwrap(o);
          sampleFlag = md->metadata.flags & XTR_FLAGS_SAMPLED;
        } else if (Event::isEvent(o)) {
          Event* e = Napi::ObjectWrap<Event>::Unwrap(o);
          sampleFlag = e->event.metadata.flags & XTR_FLAGS_SAMPLED;
        } else if (info[0].IsString()) {
            std::string str = info[0].As<Napi::String>();

            oboe_metadata_t md;
            int status = oboe_metadata_fromstr(&md, str.c_str(), str.length());
            if (status < 0) {
                return env.Undefined();
            }
            sampleFlag = md.flags & XTR_FLAGS_SAMPLED;
        }
    }

    return Napi::Boolean::New(env, sampleFlag);
}

// initialize the module
Napi::Object Metadata::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function ctor = DefineClass(env, "Metadata", {
    // Statics
    StaticMethod("fromString", &Metadata::fromString),
    StaticMethod("fromContext", &Metadata::fromContext),
    StaticMethod("makeRandom", &Metadata::makeRandom),
    StaticMethod("sampleFlagIsSet", &Metadata::sampleFlagIsSet),
    // Instance methods (prototype)
    InstanceMethod("isValid", &Metadata::isValid),
    InstanceMethod("getSampleFlag", &Metadata::getSampleFlag),
    InstanceMethod("setSampleFlagTo", &Metadata::setSampleFlagTo),
    InstanceMethod("toString", &Metadata::toString),
  });

  constructor = Napi::Persistent(ctor);
  constructor.SuppressDestruct();
  exports.Set("Metadata", ctor);

  return exports;
}

//
// function to format an x-trace with components split by ':'
//
bool Metadata::format(oboe_metadata_t* md, size_t len, char* buffer) {
    char* b = buffer;

    // length counts prefix chars, flag chars, 3 colons and null termination
    if (2 + md->task_len + md->op_len + 2 + 4 > len) return false;

    // fix up the first byte (because it's not 2B in memory)
    int task_bits = (md->task_len >> 2) - 1;
    if (task_bits == 4) task_bits = 3;
    uint8_t packed = md->version << 4 | ((md->op_len >> 2) - 1) << 3 | task_bits;
    b = PutHex(packed, b);
    *b++ = ':';

    // put the task ID
    uint8_t* mdp = (uint8_t *) &md->ids.task_id;
    for(unsigned int i = 0; i < md->task_len; i++) {
        b = PutHex(*mdp++, b);
    }
    *b++ = ':';

    // put the op ID
    mdp = (uint8_t *) &md->ids.op_id;
    for (unsigned int i = 0; i < md->op_len; i++) {
        b = PutHex(*mdp++, b);
    }
    *b++ = ':';

    // put the flags byte and null terminate the string.
    b = PutHex(md->flags, b);
    *b = '\0';

    return true;
}

char* Metadata::PutHex(uint8_t byte, char *p, char base) {
    base -= 10;
    int digit = (byte >> 4);
    digit += digit <= 9 ? '0' : base;
    *p++ = (char) digit;
    digit = byte & 0xF;
    digit += digit <= 9 ? '0' : base;
    *p++ = (char) digit;

    return p;
}
