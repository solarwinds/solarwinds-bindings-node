#include "bindings.h"
#include <iostream>

Nan::Persistent<v8::Function> Metadata::constructor;

Metadata::Metadata() {}

// Allow construction of clones
Metadata::Metadata(oboe_metadata_t *md) {
  oboe_metadata_copy(&metadata, md);
}

// Remember to cleanup the metadata struct when garbage collected
Metadata::~Metadata() {
  oboe_metadata_destroy(&metadata);
}

v8::Local<v8::Object> Metadata::NewInstance(v8::Local<v8::Value> arg) {
  Nan::EscapableHandleScope scope;

  const unsigned argc = 1;
  v8::Local<v8::Value> argv[argc] = { arg };
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
  std::string s(*Nan::Utf8String(info[0]));

  oboe_metadata_t md;
  int status = oboe_metadata_fromstr(&md, s.data(), s.size());
  if (status < 0) {
    return Nan::ThrowError("Failed to convert Metadata to string");
  }

  // Make an empty object template with space for internal field pointers
  v8::Local<v8::ObjectTemplate> t = Nan::New<v8::ObjectTemplate>();
  t->SetInternalFieldCount(2);

  // Construct an object with our internal field pointer
  v8::Local<v8::Object> obj = t->NewInstance();

  // Attach the internal field pointer
  Nan::SetInternalFieldPointer(obj, 1, (void *) &md);

  // Use the object as an argument in the event constructor
  info.GetReturnValue().Set(Metadata::NewInstance(obj));
}

// Make a new metadata instance with randomized data
NAN_METHOD(Metadata::makeRandom) {
  oboe_metadata_t md;
  oboe_metadata_init(&md);
  oboe_metadata_random(&md);

  // Make an empty object template with space for internal field pointers
  v8::Local<v8::ObjectTemplate> t = Nan::New<v8::ObjectTemplate>();
  t->SetInternalFieldCount(2);

  // Construct an object with our internal field pointer
  v8::Local<v8::Object> obj = t->NewInstance();

  // Attach the internal field pointer
  Nan::SetInternalFieldPointer(obj, 1, (void *) &md);

  // Use the object as an argument in the event constructor
  info.GetReturnValue().Set(Metadata::NewInstance(obj));
}

// Copy the contents of the metadata instance to a new instance
NAN_METHOD(Metadata::copy) {
  Metadata* self = Nan::ObjectWrap::Unwrap<Metadata>(info.This());

  // Make an empty object template with space for internal field pointers
  v8::Local<v8::ObjectTemplate> t = Nan::New<v8::ObjectTemplate>();
  t->SetInternalFieldCount(2);

  // Construct an object with our internal field pointer
  v8::Local<v8::Object> obj = t->NewInstance();

  // Attach the internal field pointer
  Nan::SetInternalFieldPointer(obj, 1, (void *) &self->metadata);

  // Use the object as an argument in the event constructor
  info.GetReturnValue().Set(Metadata::NewInstance(obj));
}

// Verify that the state of the metadata instance is valid
NAN_METHOD(Metadata::isValid) {
  Metadata* self = Nan::ObjectWrap::Unwrap<Metadata>(info.This());
  bool status = oboe_metadata_is_valid(&self->metadata);
  info.GetReturnValue().Set(Nan::New(status));
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
  // Unwrap the Metadata instance from V8
  Metadata* self = Nan::ObjectWrap::Unwrap<Metadata>(info.This());

  // Make an empty object template with space for internal field pointers
  v8::Local<v8::ObjectTemplate> t = Nan::New<v8::ObjectTemplate>();
  t->SetInternalFieldCount(2);

  // Construct an object with our internal field pointer
  v8::Local<v8::Object> obj = t->NewInstance();

  // Attach the internal field pointer
  Nan::SetInternalFieldPointer(obj, 1, (void *) &self->metadata);

  // Use the object as an argument in the event constructor
  info.GetReturnValue().Set(Event::NewInstance(obj));
}

// Creates a new Javascript instance
NAN_METHOD(Metadata::New) {
  if (!info.IsConstructCall()) {
    return Nan::ThrowError("Metadata() must be called as a constructor");
  }

  Metadata* metadata;
  if (info.Length() == 1) {
    void* ptr = Nan::GetInternalFieldPointer(info[0].As<v8::Object>(), 1);
    oboe_metadata_t* context = static_cast<oboe_metadata_t*>(ptr);
    metadata = new Metadata(context);
  } else {
    metadata = new Metadata();
  }

  metadata->Wrap(info.This());
  Nan::SetInternalFieldPointer(info.This(), 1, (void *) &metadata->metadata);
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
  Nan::SetPrototypeMethod(ctor, "toString", Metadata::toString);
  Nan::SetPrototypeMethod(ctor, "createEvent", Metadata::createEvent);

  constructor.Reset(ctor->GetFunction());
  Nan::Set(exports, Nan::New("Metadata").ToLocalChecked(), ctor->GetFunction());
}
