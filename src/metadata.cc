#include "bindings.h"

Nan::Persistent<v8::FunctionTemplate> Metadata::constructor;

Metadata::Metadata() {}

// Allow construction of clones
Metadata::Metadata(oboe_metadata_t* md) {
  oboe_metadata_copy(&metadata, md);
}

// Remember to cleanup the metadata struct when garbage collected
Metadata::~Metadata() {
  oboe_metadata_destroy(&metadata);
}

v8::Local<v8::Object> Metadata::NewInstance(Metadata* md) {
  Nan::EscapableHandleScope scope;

  const unsigned argc = 1;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(md) };
  v8::Local<v8::Function> cons = Nan::New<v8::FunctionTemplate>(constructor)->GetFunction();
  v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);

  return scope.Escape(instance);
}

v8::Local<v8::Object> Metadata::NewInstance() {
  Nan::EscapableHandleScope scope;

  const unsigned argc = 0;
  v8::Local<v8::Value> argv[argc] = {};
  v8::Local<v8::Function> cons = Nan::New<v8::FunctionTemplate>(constructor)->GetFunction();
  v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);
  //v8::Local<v8::Object> instance = Nan::NewInstance(cons, argc, argv);

  return scope.Escape(instance);
}

// Transform a string back into a metadata instance
NAN_METHOD(Metadata::fromString) {
  Nan::Utf8String str(info[0]);

  oboe_metadata_t md;
  int status = oboe_metadata_fromstr(&md, *str, str.length());
  if (status < 0) {
    return Nan::ThrowError("Failed to convert Metadata from string");
  }

  Metadata* metadata = new Metadata(&md);
  info.GetReturnValue().Set(Metadata::NewInstance(metadata));
  delete metadata;
}

// Make a new metadata instance with randomized data
/*
NAN_METHOD(Metadata::makeRandom) {
  oboe_metadata_t md;
  oboe_metadata_init(&md);
  oboe_metadata_random(&md);

  // Use the struct as an argument to the constructor
  Metadata* metadata = new Metadata(&md);
  // then use that object to construct another instance? Huh?
  info.GetReturnValue().Set(Metadata::NewInstance(metadata));
  delete metadata;
}
// */

//
// Metadata factory for randomized metadata
//
//
NAN_METHOD(Metadata::makeRandom) {
  oboe_metadata_t md;
  oboe_metadata_init(&md);
  oboe_metadata_random(&md);

  // specify argument counts and constructor arguments
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {Nan::New(&md)};

  // get a local handle to our constructor function
  v8::Local<v8::Function> constructorFunc = Nan::New(Metadata::constructor)->GetFunction();
  // create a new JS instance from arguments
  v8::Local<v8::Object> metadata = Nan::NewInstance(constructorFunc, argc, argv).ToLocalChecked();

  info.GetReturnValue().Set(metadata);
}


// Copy the contents of the metadata instance to a new instance
NAN_METHOD(Metadata::copy) {
  Metadata* self = Nan::ObjectWrap::Unwrap<Metadata>(info.This());
  info.GetReturnValue().Set(Metadata::NewInstance(self));
}

// Verify that the state of the metadata instance is valid
NAN_METHOD(Metadata::isValid) {
  Metadata* self = Nan::ObjectWrap::Unwrap<Metadata>(info.This());
  bool status = oboe_metadata_is_valid(&self->metadata);
  info.GetReturnValue().Set(Nan::New(status));
}

// Set the sample flag and return the previous value
NAN_METHOD(Metadata::setSampleFlag) {
    Metadata* self = Nan::ObjectWrap::Unwrap<Metadata>(info.This());
    bool previous = self->metadata.flags & XTR_FLAGS_SAMPLED;
    self->metadata.flags |= XTR_FLAGS_SAMPLED;
    info.GetReturnValue().Set(Nan::New(previous));
}

// Clear the sample flag and return the previous value
NAN_METHOD(Metadata::clearSampleFlag) {
    Metadata* self = Nan::ObjectWrap::Unwrap<Metadata>(info.This());
    bool previous = self->metadata.flags & XTR_FLAGS_SAMPLED;
    self->metadata.flags &= ~XTR_FLAGS_SAMPLED;
    info.GetReturnValue().Set(Nan::New(previous));
}

// Serialize a metadata object to a string
NAN_METHOD(Metadata::toString) {
  // Unwrap the Metadata instance from V8
  Metadata* self = Nan::ObjectWrap::Unwrap<Metadata>(info.This());

  // Convert the contents to a character array
  char buf[OBOE_MAX_METADATA_PACK_LEN];
  int rc = oboe_metadata_tostr(&self->metadata, buf, sizeof(buf) - 1);

  // If it worked, return it
  if (rc == 0) {
    info.GetReturnValue().Set(Nan::New(buf).ToLocalChecked());

  // Otherwise, return an empty string
  } else {
    info.GetReturnValue().Set(Nan::New("").ToLocalChecked());
  }
}

// Create an event from this metadata instance
NAN_METHOD(Metadata::createEvent) {
  Metadata* self = Nan::ObjectWrap::Unwrap<Metadata>(info.This());
  info.GetReturnValue().Set(Event::NewInstance(self));
}

// Creates a new Javascript instance
NAN_METHOD(Metadata::New) {
  if (!info.IsConstructCall()) {
    return Nan::ThrowError("Metadata() must be called as a constructor");
  }

  Metadata* metadata;
  if (info.Length() == 1 && info[0]->IsExternal()) {
    Metadata* md = static_cast<Metadata*>(info[0].As<v8::External>()->Value());
    oboe_metadata_t* context = &md->metadata;
    metadata = new Metadata(context);
  } else {
    metadata = new Metadata();
  }

  metadata->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

bool Metadata::isMetadata(v8::Local<v8::Value> object) {
  return Nan::New(Metadata::constructor)->HasInstance(object);
}

NAN_METHOD(Metadata::isInstance) {
    bool is = false;
    if (info.Length() == 1) {
        v8::Local<v8::Value> arg = info[0];
        is = Metadata::isMetadata(arg);
    }
    info.GetReturnValue().Set(is);
}

// Wrap the C++ object so V8 can understand it
void Metadata::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> ctor = Nan::New<v8::FunctionTemplate>(Event::New);
  constructor.Reset(ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(2);
  ctor->SetClassName(Nan::New("Metadata").ToLocalChecked());

  // Statics
  Nan::SetMethod(ctor, "fromString", Metadata::fromString);
  Nan::SetMethod(ctor, "makeRandom", Metadata::makeRandom);
  Nan::SetMethod(ctor, "isInstance", Metadata::isInstance);

  // Prototype
  Nan::SetPrototypeMethod(ctor, "copy", Metadata::copy);
  Nan::SetPrototypeMethod(ctor, "isValid", Metadata::isValid);
  Nan::SetPrototypeMethod(ctor, "setSampleFlag", Metadata::setSampleFlag);
  Nan::SetPrototypeMethod(ctor, "clearSampleFlag", Metadata::clearSampleFlag);
  Nan::SetPrototypeMethod(ctor, "toString", Metadata::toString);
  Nan::SetPrototypeMethod(ctor, "createEvent", Metadata::createEvent);

  Nan::Set(exports, Nan::New("Metadata").ToLocalChecked(), ctor->GetFunction());
}
