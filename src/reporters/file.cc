#include "../bindings.h"

Nan::Persistent<v8::Function> FileReporter::constructor;

// Construct with a filename.
FileReporter::FileReporter(const char *file) {
  oboe_reporter_file_init(&reporter, file);
}

// Remember to cleanup the reporter struct when garbage collected
FileReporter::~FileReporter() {
  oboe_reporter_destroy(&reporter);
}

// Transform a string back into a metadata instance
NAN_METHOD(FileReporter::sendReport) {
  if (info.Length() < 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }
  if (!info[0]->IsObject()) {
    return Nan::ThrowTypeError("Must supply an event instance");
  }

  FileReporter* self = Nan::ObjectWrap::Unwrap<FileReporter>(info.This());
  Event* event = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());

  oboe_metadata_t *md;
  if (info.Length() == 2 && info[1]->IsObject()) {
    Metadata* metadata = Nan::ObjectWrap::Unwrap<Metadata>(info[1]->ToObject());
    md = &metadata->metadata;
  } else {
    md = oboe_context_get();
  }

  int status = oboe_reporter_send(&self->reporter, md, &event->event);
  info.GetReturnValue().Set(Nan::New(status >= 0));
}

// Creates a new Javascript instance
NAN_METHOD(FileReporter::New) {
  if (!info.IsConstructCall()) {
    return Nan::ThrowError("FileReporter() must be called as a constructor");
  }

  // Validate arguments
  if (info.Length() != 1) {
    return Nan::ThrowError("Wrong number of arguments");
  }
  if (!info[0]->IsString()) {
    return Nan::ThrowTypeError("Address must be a string");
  }

  FileReporter* obj = new FileReporter(*Nan::Utf8String(info[0]));

  obj->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

// Wrap the C++ object so V8 can understand it
void FileReporter::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> ctor = Nan::New<v8::FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(Nan::New("FileReporter").ToLocalChecked());

  // Prototype
  Nan::SetPrototypeMethod(ctor, "sendReport", FileReporter::sendReport);

  constructor.Reset(ctor->GetFunction());
  Nan::Set(exports, Nan::New("FileReporter").ToLocalChecked(), ctor->GetFunction());
}
