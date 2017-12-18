#include "bindings.h"

Event::Event() {
  oboe_status = oboe_event_init(&event, oboe_context_get());
}

Event::Event(const oboe_metadata_t* md, bool addEdge) {
  if (addEdge) {
    oboe_status = oboe_metadata_create_event(md, &event);
  } else {
    oboe_status = oboe_event_init(&event, md);
  }
  std::cout << "oboe_status is " << oboe_status;
}

Event::~Event() {
  oboe_event_destroy(&event);
}

Nan::Persistent<v8::FunctionTemplate> Event::constructor;

NAN_MODULE_INIT(Event::Init) {
  v8::Local<v8::FunctionTemplate> ctor = Nan::New<v8::FunctionTemplate>(Event::New);
  constructor.Reset(ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(Nan::New("Event").ToLocalChecked());

  Nan::SetPrototypeMethod(ctor, "addInfo", Event::addInfo);
  Nan::SetPrototypeMethod(ctor, "addEdge", Event::addEdge);
  Nan::SetPrototypeMethod(ctor, "getMetadata", Event::getMetadata);
  Nan::SetPrototypeMethod(ctor, "toString", Event::toString);

  Nan::SetMethod(ctor, "startTrace", Event::startTrace);
  Nan::SetMethod(ctor, "x", Event::X);


  /* From Vector saved for reference.
  // link our getters and setter to the object property
  Nan::SetAccessor(ctor->InstanceTemplate(), Nan::New("x").ToLocalChecked(), Event::HandleGetters, Event::HandleSetters);
  Nan::SetAccessor(ctor->InstanceTemplate(), Nan::New("y").ToLocalChecked(), Event::HandleGetters, Event::HandleSetters);
  Nan::SetAccessor(ctor->InstanceTemplate(), Nan::New("z").ToLocalChecked(), Event::HandleGetters, Event::HandleSetters);

  Nan::SetPrototypeMethod(ctor, "add", Add);
  // */

  target->Set(Nan::New("Event").ToLocalChecked(), ctor->GetFunction());
}

NAN_METHOD(Event::New) {
  // throw an error if constructor is called without new keyword
  if(!info.IsConstructCall()) {
    return Nan::ThrowError(Nan::New("Event::New - called without new keyword").ToLocalChecked());
  }

  Event* event;

  if (info.Length() == 0) {
    event = new Event();
  } else {
    // TODO BAM make the multiple argument version work because it doesn't.
    event = new Event();
  }

  // TODO BAM check event->oboe_status.

  event->Wrap(info.Holder());

  info.GetReturnValue().Set(info.Holder());
}

NAN_METHOD(Event::toString) {
  Event* self = Nan::ObjectWrap::Unwrap<Event>(info.This());

  oboe_event_t* event = &self->event;

  char buf[OBOE_MAX_METADATA_PACK_LEN];
  int result = oboe_metadata_tostr(&event->metadata, buf, sizeof(buf) - 1);

  // if there was an error make the string null
  if (result != 0) {
    buf[0] = '\0';
  }

  info.GetReturnValue().Set(Nan::New(buf).ToLocalChecked());
}

NAN_METHOD(Event::getMetadata) {
  //Event* self = Nan::ObjectWrap::Unwrap<Event>(info.This());

  info.GetReturnValue().Set(Nan::New("TODO BAM").ToLocalChecked());
}

NAN_METHOD(Event::startTrace) {
  info.GetReturnValue().Set(Nan::New("TODO BAM").ToLocalChecked());
}

NAN_METHOD(Event::addEdge) {
  info.GetReturnValue().Set(Nan::New("TODO BAM").ToLocalChecked());
}

NAN_METHOD(Event::addInfo) {
  info.GetReturnValue().Set(Nan::New("TODO BAM").ToLocalChecked());
}

bool Event::isEvent(v8::Local<v8::Value> object) {
  bool is = Nan::New(Event::constructor)->HasInstance(object);
  return is;
}

NAN_METHOD(Event::X) {
  char const* message = "no conditions met";

  if (info.Length() <= 0) {
      message = "No arguments";
  } else if (info[0]->IsExternal()) {
      message = "It is external";
  } else if (info[0]->IsObject()) {
      v8::Local<v8::Value> arg = info[0];
      if (Event::isEvent(arg)) {
      //if (Nan::New(Event::constructor)->HasInstance(arg)) {
      //if (Nan::New(Event::constructor)->HasInstance(info[0])) {

          message = "it is an Event instance";
      } else {
          message = "it is not an Event instance";
          /*
          // presume it's metadata
          md = Nan::ObjectWrap::Unwrap<Metadata>(info[0]->ToObject());
          message = "The argument is an object";
          if (md) message = "The argument is unwrapped and not null";
          oboe_metadata_t* omd = &md->metadata;
          if (omd) {
              int n = sprintf(buffer, "omd version: 0x%02x", (int) omd->version);
              char * b = new char[n + 1];
              std::string s(buffer);
              std::cout << "XXXX" << s.c_str() << std::endl;
              delete b;

              b = new char[100];
              oboe_metadata_tostr(omd, b, 100);
              message = b;
          }
          // */
      }
  }

  info.GetReturnValue().Set(Nan::New(message).ToLocalChecked());

  //delete message;

}

/*
NAN_METHOD(Event::Add) {
  // unwrap this Event
  Event * self = Nan::ObjectWrap::Unwrap<Event>(info.This());

  if (!Nan::New(Event::constructor)->HasInstance(info[0])) {
    return Nan::ThrowError(Nan::New("Event::Add - expected argument to be instance of Event").ToLocalChecked());
  }
  // unwrap the Event passed as argument
  Event * otherVec = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());

  // specify argument counts and constructor arguments
  const int argc = 3;
  v8::Local<v8::Value> argv[argc] = {
    Nan::New(self->x + otherVec->x),
    Nan::New(self->y + otherVec->y),
    Nan::New(self->z + otherVec->z)
  };

  // get a local handle to our constructor function
  v8::Local<v8::Function> constructorFunc = Nan::New(Event::constructor)->GetFunction();
  // create a new JS instance from arguments
  v8::Local<v8::Object> jsSumVec = Nan::NewInstance(constructorFunc, argc, argv).ToLocalChecked();

  info.GetReturnValue().Set(jsSumVec);
}

NAN_GETTER(Event::HandleGetters) {
  Event* self = Nan::ObjectWrap::Unwrap<Event>(info.This());

  std::string propertyName = std::string(*Nan::Utf8String(property));
  if (propertyName == "x") {
    info.GetReturnValue().Set(self->x);
  } else if (propertyName == "y") {
    info.GetReturnValue().Set(self->y);
  } else if (propertyName == "z") {
    info.GetReturnValue().Set(self->z);
  } else {
    info.GetReturnValue().Set(Nan::Undefined());
  }
}

NAN_SETTER(Event::HandleSetters) {
  Event* self = Nan::ObjectWrap::Unwrap<Event>(info.This());

  if(!value->IsNumber()) {
    return Nan::ThrowError(Nan::New("expected value to be a number").ToLocalChecked());
  }

  std::string propertyName = std::string(*Nan::Utf8String(property));
  if (propertyName == "x") {
    self->x = value->NumberValue();
  } else if (propertyName == "y") {
    self->y = value->NumberValue();
  } else if (propertyName == "z") {
    self->z = value->NumberValue();
  }
}
// */
