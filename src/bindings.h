#ifndef NODE_OBOE_H_
#define NODE_OBOE_H_

#include <iostream>
#include <string>

#include <node.h>
#include <nan.h>
#include <uv.h>
#include <v8.h>

#include <oboe/oboe.h>

class Event;

class Metadata : public Nan::ObjectWrap {
  friend class Reporter;
  friend class OboeContext;
  friend class Event;

  ~Metadata();
  Metadata();
  Metadata(oboe_metadata_t*);
  bool sampleFlagIsOn();

  oboe_metadata_t metadata;
  static Nan::Persistent<v8::FunctionTemplate> constructor;
  static NAN_METHOD(New);
  static NAN_METHOD(getMetadataData);
  static NAN_METHOD(fromString);
  static NAN_METHOD(makeRandom);
  static NAN_METHOD(copy);
  static NAN_METHOD(isValid);
  static NAN_METHOD(getSampleFlag);
  static NAN_METHOD(setSampleFlagTo);
  static NAN_METHOD(toString);
  static NAN_METHOD(createEvent);
  static NAN_METHOD(fromContext);

  static v8::Local<v8::Object> NewInstance(Metadata*);
  static v8::Local<v8::Object> NewInstance();

  static NAN_METHOD(isInstance);
  static NAN_METHOD(sampleFlagIsSet);
  static bool isMetadata(v8::Local<v8::Value>);
  static Metadata* getMetadata(v8::Local<v8::Value>);
  static bool format(oboe_metadata_t*, size_t, char*);

  private:
    static char* PutHex(uint8_t, char*, char = 'a');

  public:
    static void Init(v8::Local<v8::Object>);
};

class OboeContext {
  friend class Reporter;
  friend class Metadata;
  friend class Event;

  // V8 conversion
  static NAN_METHOD(setTracingMode);
  static NAN_METHOD(setDefaultSampleRate);
  static NAN_METHOD(sampleTrace);
  static NAN_METHOD(toString);
  static NAN_METHOD(set);
  static NAN_METHOD(copy);
  static NAN_METHOD(clear);
  static NAN_METHOD(isValid);
  static NAN_METHOD(init);
  static NAN_METHOD(createEvent);
  static NAN_METHOD(createEventX);
  static NAN_METHOD(startTrace);

  public:
    static void Init(v8::Local<v8::Object>);
};

class Event : public Nan::ObjectWrap {
  friend class Reporter;
  friend class OboeContext;
  friend class Metadata;
  friend class Log;

  explicit Event();
  explicit Event(const oboe_metadata_t*, bool);
  ~Event();

  oboe_event_t event;
  int oboe_status;
  static Nan::Persistent<v8::FunctionTemplate> constructor;
  static NAN_METHOD(New);
  static NAN_METHOD(getEventData);
  static NAN_METHOD(addInfo);
  static NAN_METHOD(addEdge);
  static NAN_METHOD(getMetadata);
  static NAN_METHOD(toString);
  static NAN_METHOD(setSampleFlagTo);
  static NAN_METHOD(getSampleFlag);

  static bool isEvent(v8::Local<v8::Value>);

  static v8::Local<v8::Object> NewInstance(Metadata*, bool);
  static v8::Local<v8::Object> NewInstance(Metadata*);
  static v8::Local<v8::Object> NewInstance();

  public:
    static void Init(v8::Local<v8::Object>);
};

class Reporter : public Nan::ObjectWrap {
    Reporter();
    ~Reporter();
    int send_event_x(Nan::NAN_METHOD_ARGS_TYPE, int);

    static Nan::Persistent<v8::FunctionTemplate> constructor;
    static NAN_METHOD(New);
    static NAN_METHOD(isReadyToSample);
    static NAN_METHOD(sendReport);
    static NAN_METHOD(sendStatus);
    static NAN_METHOD(sendHttpSpan);
    static NAN_METHOD(sendHttpSpanName);
    static NAN_METHOD(sendHttpSpanUrl);

    static v8::Local<v8::Object> NewInstance();

    public:
      static void Init(v8::Local<v8::Object>);
};


class Utility {

  public:

    static inline int64_t get_integer(
        v8::Local<v8::Object> obj,
        v8::Local<v8::String> prop,
        int64_t default_value = 0) {

        if (Nan::Has(obj, prop).FromMaybe(false)) {
            Nan::MaybeLocal<v8::Value> v = Nan::Get(obj, prop);
            if (!v.IsEmpty()) {
                v8::Local<v8::Value> val = v.ToLocalChecked();
                if (val->IsInt32() || val->IsNumber()) {
                    return val->IntegerValue();
                }
            }
        }
        return default_value;
    }

    //
    // returns a new std::string that must be deleted.
    //
    static inline std::string* get_string(
        v8::Local<v8::Object> obj,
        v8::Local<v8::String> prop,
        const char* default_value = "") {

        if (Nan::Has(obj, prop).FromMaybe(false)) {
            Nan::MaybeLocal<v8::Value> v = Nan::Get(obj, prop);
            if (!v.IsEmpty()) {
                v8::Local<v8::Value> val = v.ToLocalChecked();
                if (val->IsString()) {
                    //std::string* method = Nan::Utf8String(val); ?
                    std::string* string = new std::string(*v8::String::Utf8Value(val->ToString()));
                    return string;
                }
            }
        }
        return new std::string(default_value);
    }

    static inline bool get_boolean(
        v8::Local<v8::Object> obj,
        v8::Local<v8::String> prop,
        bool default_value = false) {

        if (Nan::Has(obj, prop).FromMaybe(false)) {
            Nan::MaybeLocal<v8::Value> v = Nan::Get(obj, prop);
            if (!v.IsEmpty()) {
                return Nan::Get(obj, prop).ToLocalChecked()->BooleanValue();
            }
        }
        return default_value;
    }
};


class Config {
    static NAN_METHOD(getRevision);
    static NAN_METHOD(getVersion);
    static NAN_METHOD(checkVersion);
    static NAN_METHOD(getVersionString);

public:
    static void Init(v8::Local<v8::Object>);
};

class Sanitizer {
  static NAN_METHOD(sanitize);

  public:
    static void Init(v8::Local<v8::Object>);
};

#endif  // NODE_OBOE_H_
