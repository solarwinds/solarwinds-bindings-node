#ifndef NODE_OBOE_H_
#define NODE_OBOE_H_

#include <iostream>
#include <string>

#include <napi.h>
#include <oboe/oboe.h>

typedef int (*send_generic_span_t) (char*, uint16_t, oboe_span_params_t*);

//
// Metadata - work with oboe's oboe_metadata_t structure.
//
class Metadata : public Napi::ObjectWrap<Metadata> {

public:
  Metadata();
  Metadata(const Napi::CallbackInfo& info);
  ~Metadata();

  // keep a copy of the metadata.
  oboe_metadata_t metadata;

  // these functions operate on the metadata
  bool sampleFlagIsOn();
  Napi::Value isValid(const Napi::CallbackInfo& info);
  Napi::Value getSampleFlag(const Napi::CallbackInfo& info);
  Napi::Value setSampleFlagTo(const Napi::CallbackInfo& info);
  Napi::Value toString(const Napi::CallbackInfo& info);

  // return a Metadata object
  static Napi::Value fromContext(const Napi::CallbackInfo& info);
  static Napi::Value fromString(const Napi::CallbackInfo& info);
  static Napi::Value makeRandom(const Napi::CallbackInfo& info);

  // instanceof equivalent for C++ functions
  static bool isMetadata(const Napi::Object);

  // extract metadata from Napi::Value which may be an instance
  // of Metadata, an Event, a string, or Napi::External referring
  // to oboe_metadata_t.
  static bool getMetadata(Napi::Value, oboe_metadata_t*);

  // work with existing metadata
  static Napi::Value sampleFlagIsSet(const Napi::CallbackInfo& info);
  static bool format(oboe_metadata_t*, size_t, char*, uint flags, char sep = ':');
  const static int ff_none = 0;
  const static int ff_header = 1;
  const static int ff_task = 2;
  const static int ff_op = 4;
  const static int ff_flags = 8;
  const static int ff_sample = 16;
  const static int ff_separators = 32;
  const static int ff_lowercase = 64;

  // C++ callable for creating an instance of Metadata
  static Napi::Object NewInstance(Napi::Env, Napi::Value);

private:
  static Napi::FunctionReference constructor;

public:
  static Napi::Object Init(Napi::Env, Napi::Object);
};


//
// Event - work with oboe's oboe_event_t structure.
//
class Event : public Napi::ObjectWrap<Event> {

public:
  Event(const Napi::CallbackInfo& info);
  ~Event();

  // the oboe event this instance manages
  oboe_event_t event;
  // oboe status returned by constructor rather than throwing a JavaScript
  // error. allows C++ code to do cleanup if necessary.
  int oboe_status;

  // methods that manipulate the instance's oboe_event_t
  Napi::Value addInfo(const Napi::CallbackInfo& info);
  Napi::Value addEdge(const Napi::CallbackInfo& info);
  Napi::Value getMetadata(const Napi::CallbackInfo& info);
  Napi::Value toString(const Napi::CallbackInfo& info);
  Napi::Value setSampleFlagTo(const Napi::CallbackInfo& info);
  Napi::Value getSampleFlag(const Napi::CallbackInfo& info);

  // C++ instanceof equivalent
  static bool isEvent(Napi::Object);

  // C++ callable constructors.
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
// Reporter is a collection of functions providing access to oboe's
// send functions.
//
namespace Reporter {
  Napi::Object Init(Napi::Env, Napi::Object);
}

//
// Sanitizer provides the sanitize function.
//
namespace Sanitizer {
  Napi::Object Init(Napi::Env, Napi::Object);
}

//
// Config provides the getVersionString function.
//
namespace Config {
  Napi::Object Init(Napi::Env, Napi::Object);
}

#endif  // NODE_OBOE_H_
