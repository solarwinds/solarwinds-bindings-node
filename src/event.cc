#include "bindings.h"

Nan::Persistent<v8::Function> Event::constructor;

// Construct a blank event from the context metadata
Event::Event() {
  oboe_event_init(&event, OboeContext::get());
}

// Construct a new event point an edge at another
Event::Event(const oboe_metadata_t *md, bool addEdge) {
  // both methods copy metadata from md -> this
  if (addEdge) {
    // create_event automatically adds edge in event to md
    oboe_metadata_create_event(md, &event);
  } else {
    // initializes new Event with this md's task_id & new random op_id; no edges set
    oboe_event_init(&event, md);
  }
}

// Remember to cleanup the struct when garbage collected
Event::~Event() {
  oboe_event_destroy(&event);
}

v8::Local<v8::Object> Event::NewInstance(v8::Local<v8::Value> context, v8::Local<v8::Value> addEdge) {
  Nan::EscapableHandleScope scope;

  const unsigned argc = 2;
  v8::Local<v8::Value> argv[argc] = { context, addEdge };
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
  v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);

  return scope.Escape(instance);
}

v8::Local<v8::Object> Event::NewInstance(v8::Local<v8::Value> context) {
  Nan::EscapableHandleScope scope;

  const unsigned argc = 1;
  v8::Local<v8::Value> argv[argc] = { context };
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
  v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);

  return scope.Escape(instance);
}

v8::Local<v8::Object> Event::NewInstance() {
  Nan::EscapableHandleScope scope;

  const unsigned argc = 0;
  v8::Local<v8::Value> argv[argc] = {};
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
  v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);

  return scope.Escape(instance);
}

// Add info to the event
NAN_METHOD(Event::addInfo) {
  // Validate arguments
  if (info.Length() != 2) {
    return Nan::ThrowError("Wrong number of arguments");
  }
  if (!info[0]->IsString()) {
    return Nan::ThrowTypeError("Key must be a string");
  }
  if (!info[1]->IsString() && !info[1]->IsNumber() && !info[1]->IsBoolean()) {
    return Nan::ThrowTypeError("Value must be a boolean, string or number");
  }

  // Unwrap event instance from V8
  Event* self = ObjectWrap::Unwrap<Event>(info.This());
  oboe_event_t* event = &self->event;

  // Get key string from arguments and prepare a status variable
  Nan::Utf8String nan_key(info[0]);
  const char* key = *nan_key;
  int status;

  // Handle integer values
  if (info[1]->IsBoolean()) {
    bool val = info[1]->BooleanValue();
    status = oboe_event_add_info_bool(event, key, val);

  // Handle double values
  } else if (info[1]->IsInt32()) {
    int64_t val = info[1]->Int32Value();
    status = oboe_event_add_info_int64(event, key, val);

  // Handle double values
  } else if (info[1]->IsNumber()) {
    const double val = info[1]->NumberValue();
    status = oboe_event_add_info_double(event, key, val);

  // Handle string values
  } else {
    // Get value string from arguments
    Nan::Utf8String nan_val(info[1]);
    char* val = *nan_val;
    int len = strlen(val);

    // Detect if we should add as binary or a string
    // TODO: Should probably use buffers for binary data...
    if (memchr(val, '\0', len)) {
      status = oboe_event_add_info_binary(event, key, val, len);
    } else {
      status = oboe_event_add_info(event, key, val);
    }
  }

  if (status < 0) {
    return Nan::ThrowError("Failed to add info");
  }
}

// Add an edge from a metadata instance
NAN_METHOD(Event::addEdge) {
  // Validate arguments
  if (info.Length() != 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }
  if (!info[0]->IsObject() && !info[0]->IsString()) {
    return Nan::ThrowTypeError("Must supply an edge metadata instance or string");
  }

  // Unwrap event instance from V8
  Event* self = Nan::ObjectWrap::Unwrap<Event>(info.This());
  int status;

  if (info[0]->IsObject()) {
    // Unwrap metadata instance from arguments
    Metadata* metadata = Nan::ObjectWrap::Unwrap<Metadata>(info[0]->ToObject());

    // Attempt to add the edge
    status = oboe_event_add_edge(&self->event, &metadata->metadata);
  } else {
    // Get string data from arguments
    Nan::Utf8String edge(info[0]);
    std::string val(*edge);

    // Attempt to add edge
    status = oboe_event_add_edge_fromstr(&self->event, *edge, strlen(*edge));
  }

  if (status < 0) {
    return Nan::ThrowError("Failed to add edge");
  }
}

// Get the metadata of an event
NAN_METHOD(Event::getMetadata) {
  // Unwrap event instance from V8
  Event* self = Nan::ObjectWrap::Unwrap<Event>(info.This());
  oboe_event_t* event = &self->event;

  // Make an empty object template with space for internal field pointers
  v8::Local<v8::ObjectTemplate> t = Nan::New<v8::ObjectTemplate>();
  t->SetInternalFieldCount(2);

  // Construct an object with our internal field pointer
  v8::Local<v8::Object> obj = t->NewInstance();

  // Attach the internal field pointer
  Nan::SetInternalFieldPointer(obj, 1, (void *) &event->metadata);

  // Use the object as an argument in the event constructor
  info.GetReturnValue().Set(Metadata::NewInstance(obj));
}

// Get the metadata of an event as a string
NAN_METHOD(Event::toString) {
  // Unwrap the event instance from V8
  Event* self = Nan::ObjectWrap::Unwrap<Event>(info.This());

  // Get a pointer to the event struct
  oboe_event_t* event = &self->event;

  // Build a character array from the event metadata content
  char buf[OBOE_MAX_METADATA_PACK_LEN];
  int rc = oboe_metadata_tostr(&event->metadata, buf, sizeof(buf) - 1);

  // If we have data, return it as a string
  if (rc == 0) {
    info.GetReturnValue().Set(Nan::New(buf).ToLocalChecked());

  // Otherwise, return an empty string
  } else {
    info.GetReturnValue().Set(Nan::New("").ToLocalChecked());
  }
}

// Start tracing using supplied metadata
NAN_METHOD(Event::startTrace) {
  // Validate arguments
  if (info.Length() != 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }
  if (!info[0]->IsObject()) {
    return Nan::ThrowTypeError("Must supply a metadata instance");
  }

  // Unwrap metadata from arguments
  Metadata* metadata = Nan::ObjectWrap::Unwrap<Metadata>(info[0]->ToObject());

  // Make an empty object template with space for internal field pointers
  v8::Local<v8::ObjectTemplate> t = Nan::New<v8::ObjectTemplate>();
  t->SetInternalFieldCount(2);

  // Construct an object with our internal field pointer
  v8::Local<v8::Object> obj = t->NewInstance();

  // Attach the internal field pointer
  Nan::SetInternalFieldPointer(obj, 1, (void *) &metadata->metadata);

  // Use the object as an argument in the event constructor
  info.GetReturnValue().Set(Event::NewInstance(obj, Nan::New(false)));
}

// Creates a new Javascript instance
NAN_METHOD(Event::New) {
  if (!info.IsConstructCall()) {
    return Nan::ThrowError("Event() must be called as a constructor");
  }

  Event* event;
  if (info.Length() > 0) {
    void* ptr = Nan::GetInternalFieldPointer(info[0].As<v8::Object>(), 1);
    oboe_metadata_t* context = static_cast<oboe_metadata_t*>(ptr);

    bool addEdge = true;
    if (info.Length() == 2 && info[1]->IsBoolean()) {
      addEdge = info[1]->BooleanValue();
    }

    event = new Event(context, addEdge);
  } else {
    event = new Event();
  }

  event->Wrap(info.This());
  Nan::SetInternalFieldPointer(info.This(), 1, &event->event);
  info.GetReturnValue().Set(info.This());
}

// Wrap the C++ object so V8 can understand it
void Event::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> ctor = Nan::New<v8::FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(2);
  ctor->SetClassName(Nan::New("Event").ToLocalChecked());

  // Statics
  Nan::SetMethod(ctor, "startTrace", Event::startTrace);

  // Prototype
  Nan::SetPrototypeMethod(ctor, "addInfo", Event::addInfo);
  Nan::SetPrototypeMethod(ctor, "addEdge", Event::addEdge);
  Nan::SetPrototypeMethod(ctor, "getMetadata", Event::getMetadata);
  Nan::SetPrototypeMethod(ctor, "toString", Event::toString);

  constructor.Reset(ctor->GetFunction());
  Nan::Set(exports, Nan::New("Event").ToLocalChecked(), ctor->GetFunction());
}
