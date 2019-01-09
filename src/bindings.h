#ifndef NODE_OBOE_H_
#define NODE_OBOE_H_

#include <iostream>
#include <string>

#include <napi.h>
#include <oboe/oboe.h>

typedef int (*send_generic_span_t) (char*, uint16_t, oboe_span_params_t*);

class Event;

class Metadata : public Napi::ObjectWrap<Metadata> {
  friend class Reporter;
  friend class Event;

public:
  Metadata();
  Metadata(const Napi::CallbackInfo& info);
  ~Metadata();

  bool sampleFlagIsOn();
  Napi::Value isValid(const Napi::CallbackInfo& info);
  Napi::Value getSampleFlag(const Napi::CallbackInfo& info);
  Napi::Value setSampleFlagTo(const Napi::CallbackInfo& info);
  Napi::Value toString(const Napi::CallbackInfo& info);

  oboe_metadata_t metadata;
  //static Napi::FunctionReference constructor;
  //static Napi::Value New(const Napi::CallbackInfo& info);
  //static bool getMetadataData(const Napi::Value, oboe_metadata_t*);
  //static Napi::Value copy(const Napi::CallbackInfo& info);
  //static Napi::Value createEvent(const Napi::CallbackInfo& info);

  // return a Metadata object
  static Napi::Value fromContext(const Napi::CallbackInfo& info);
  static Napi::Value fromString(const Napi::CallbackInfo& info);
  static Napi::Value makeRandom(const Napi::CallbackInfo& info);

  // instanceof equivalent for C++ functions
  static bool isMetadata(const Napi::Object);

  static bool getMetadata(Napi::Value, oboe_metadata_t*);

  static Napi::Object NewInstance(Metadata*);
  static Napi::Object NewInstance();
  static Napi::Object NewInstance(Napi::Env, Napi::Value);

  static Napi::Value sampleFlagIsSet(const Napi::CallbackInfo& info);
  static bool format(oboe_metadata_t*, size_t, char*);

private:
  static Napi::FunctionReference constructor;
  static char* PutHex(uint8_t, char*, char = 'a');

public:
  static Napi::Object Init(Napi::Env, Napi::Object);
};

class Event : public Napi::ObjectWrap<Event> {
  friend class Reporter;
  friend class Metadata;
  friend class Log;

public:
  Event();
  Event(const Napi::CallbackInfo& info);
  ~Event();
  //explicit Event(const oboe_metadata_t*, bool);

  oboe_event_t event;
  int oboe_status;
  Napi::Value addInfo(const Napi::CallbackInfo& info);
  Napi::Value addEdge(const Napi::CallbackInfo& info);
  Napi::Value getMetadata(const Napi::CallbackInfo& info);
  Napi::Value toString(const Napi::CallbackInfo& info);
  Napi::Value setSampleFlagTo(const Napi::CallbackInfo& info);
  Napi::Value getSampleFlag(const Napi::CallbackInfo& info);

  static bool isEvent(Napi::Object);

  static Napi::Object NewInstance(Napi::Env);
  static Napi::Object NewInstance(Napi::Env, oboe_metadata_t*, bool = true);

private:
  static Napi::FunctionReference constructor;

public:
  static Napi::Object Init(Napi::Env, Napi::Object);
};

//
// OboeContext is a collection of functions providing access to the oboe
// context functions.
//
namespace OboeContext {
  Napi::Object Init(Napi::Env, Napi::Object);
}

//
// Reporter is not really a class but used to be.
//
class Reporter : public Napi::ObjectWrap<Reporter> {

public:
  Reporter();
  Reporter(const Napi::CallbackInfo&);
  ~Reporter();
  int send_event_x(const Napi::CallbackInfo&, int);


  //static Napi::Value New(const Napi::CallbackInfo& info);
  Napi::Value isReadyToSample(const Napi::CallbackInfo& info);
  Napi::Value sendReport(const Napi::CallbackInfo& info);
  Napi::Value sendStatus(const Napi::CallbackInfo& info);
  Napi::Value sendHttpSpan(const Napi::CallbackInfo& info);
  Napi::Value sendNonHttpSpan(const Napi::CallbackInfo& info);
  static Napi::Value send_span(const Napi::CallbackInfo&, send_generic_span_t);

private:
  static Napi::FunctionReference constructor;

public:
  static Napi::Object Init(Napi::Env, Napi::Object);
};

//
// Sanitizer is a collection of functions
//
namespace Sanitizer {
  Napi::Object Init(Napi::Env, Napi::Object);
}

//
// Config is a collection of functions
//
namespace Config {
  Napi::Object Init(Napi::Env, Napi::Object);
}

#endif  // NODE_OBOE_H_
