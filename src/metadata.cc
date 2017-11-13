#include "bindings.h"
#include <iostream>

Nan::Persistent<v8::Function> Metadata::constructor;

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
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
  v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);

  return scope.Escape(instance);
}

v8::Local<v8::Object> Metadata::NewInstance() {
  Nan::EscapableHandleScope scope;

  const unsigned argc = 0;
  v8::Local<v8::Value> argv[argc] = {};
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
  v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);

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
NAN_METHOD(Metadata::makeRandom) {
  oboe_metadata_t md;
  oboe_metadata_init(&md);
  oboe_metadata_random(&md);

  // Use the object as an argument in the event constructor
  Metadata* metadata = new Metadata(&md);
  info.GetReturnValue().Set(Metadata::NewInstance(metadata));
  delete metadata;
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

NAN_METHOD(Metadata::setSampleFlag) {
    Metadata* self = Nan::ObjectWrap::Unwrap<Metadata>(info.This());
    bool previous = self->metadata.flags & XTR_FLAGS_SAMPLED;
    self->metadata.flags |= XTR_FLAGS_SAMPLED;
    info.GetReturnValue().Set(Nan::New(previous));
}

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

// Wrap the C++ object so V8 can understand it
void Metadata::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> ctor = Nan::New<v8::FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(2);
  ctor->SetClassName(Nan::New("Metadata").ToLocalChecked());

  // Statics
  Nan::SetMethod(ctor, "fromString", Metadata::fromString);
  Nan::SetMethod(ctor, "makeRandom", Metadata::makeRandom);

  // Prototype
  Nan::SetPrototypeMethod(ctor, "copy", Metadata::copy);
  Nan::SetPrototypeMethod(ctor, "isValid", Metadata::isValid);
  Nan::SetPrototypeMethod(ctor, "setSampleFlag", Metadata::setSampleFlag);
  Nan::SetPrototypeMethod(ctor, "clearSampleFlag", Metadata::clearSampleFlag);
  Nan::SetPrototypeMethod(ctor, "toString", Metadata::toString);
  Nan::SetPrototypeMethod(ctor, "createEvent", Metadata::createEvent);

  constructor.Reset(ctor->GetFunction());
  Nan::Set(exports, Nan::New("Metadata").ToLocalChecked(), ctor->GetFunction());
}
