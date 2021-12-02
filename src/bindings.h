#ifndef NODE_OBOE_H_
#define NODE_OBOE_H_

#include <iostream>
#include <string>

#include <napi.h>
#include <oboe/oboe.h>

typedef int (*send_generic_span_t) (char*, uint16_t, oboe_span_params_t*);

//
// Event - work with oboe's oboe_event_t structure.
//
class Event : public Napi::ObjectWrap<Event> {

static size_t total_created;      // the total number created
static size_t total_destroyed;    // destructor has been called
static size_t ptotal_destroyed;   // previous total destructed
static size_t total_bytes_alloc;  // total allocated active events
static size_t bytes_freed;        // allocated bytes freed
static size_t small_active;       // small not yet destructed
static size_t full_active;        // full events not yet destructed
static uint64_t lifetime;         // cumulative microsecs lifetime
static uint64_t plifetime;        // previous total lifetime
static uint64_t s_sendtime;       // cumulative microsecs time until sent
static uint64_t s_psendtime;      // previous sendtime
static size_t actual_bytes_used;  // total number of event bytes @ send (event + used bson buffer)
static size_t sent_count;         // total number of events sent
static size_t s_psent_count;      // previous count of events sent

public:
  Event(const Napi::CallbackInfo& info);
  ~Event();

private:
  // C++ callable constructor.
  static Napi::Object NewInstance(Napi::Env);

  // the oboe event this instance manages
  oboe_event_t event;
  // keep track of whether oboe_event_init() has been called. if so
  // then oboe_event_destroy() must be called to free the bson buffer.
  bool initialized;

  // stats

  // size of this event (including bson buffer) but exclusive of c++
  // object overhead. it is usually bigger than the actual number of
  // bytes used by the event.
  size_t bytes_allocated;

  uint64_t creation_time;   // time event was created
  uint64_t send_time;       // time event was sent

 public:
  // methods that manipulate the instance's oboe_event_t
  Napi::Value addInfo(const Napi::CallbackInfo& info);
  Napi::Value addEdge(const Napi::CallbackInfo& info);
  Napi::Value setSampleFlagTo(const Napi::CallbackInfo& info);
  Napi::Value getSampleFlag(const Napi::CallbackInfo& info);

  Napi::Value getBytesAllocated(const Napi::CallbackInfo& info);

  // formatting the strings
  Napi::Value toString(const Napi::CallbackInfo& info);
  const static int ff_header = 1;
  const static int ff_task = 2;
  const static int ff_op = 4;
  const static int ff_flags = 8;
  const static int ff_sample = 16;
  const static int ff_separators = 32;
  const static int ff_lowercase = 64;

  // predefined formats
  const static int fmtHuman =
      ff_header | ff_task | ff_op | ff_flags | ff_separators | ff_lowercase;
  const static int fmtLog = ff_task | ff_sample | ff_separators;

  // size needed to format is the size needed + 3 for delimiters to split the
  // parts.
  const static size_t fmtBufferSize = OBOE_MAX_METADATA_PACK_LEN + 3;

  Napi::Value sendStatus(const Napi::CallbackInfo& info);
  Napi::Value sendReport(const Napi::CallbackInfo& info);

private:
  int send_event_x(int channel);

public:
  // methods that create an invalid event that contains only metadata.
  static Napi::Value makeRandom(const Napi::CallbackInfo& info);
  static Napi::Value makeFromBuffer(const Napi::CallbackInfo& info);

  // C++ instanceof equivalent
  static bool isEvent(Napi::Object);

  // C++ method to create an unitialized, invalid oboe event.
  static Napi::Object makeFromOboeMetadata(const Napi::Env env, oboe_metadata_t& omd);

  static Napi::Value getEventStats(const Napi::CallbackInfo& info);

 private:
  static Napi::FunctionReference constructor;

public:
  static Napi::Object Init(Napi::Env, Napi::Object);
};

//
// Settings is a collection of functions for getting/setting
// oboe's tracing settings
//
namespace Settings {
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
