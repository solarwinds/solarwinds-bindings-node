#include <nan.h>
#include <map>

#include "hdr_histogram.h"
#include "gc.h"

/* https://v8docs.nodesource.com/node-8.16/d4/da0/v8_8h_source.html#l06365
  enum GCType {
    kGCTypeScavenge = 1 << 0,
    kGCTypeMarkSweepCompact = 1 << 1,
    kGCTypeIncrementalMarking = 1 << 2,
    kGCTypeProcessWeakCallbacks = 1 << 3,
    kGCTypeAll = kGCTypeScavenge | kGCTypeMarkSweepCompact |
                 kGCTypeIncrementalMarking | kGCTypeProcessWeakCallbacks
  };

  enum GCCallbackFlags {
    kNoGCCallbackFlags = 0,
    kGCCallbackFlagConstructRetainedObjectInfos = 1 << 1,
    kGCCallbackFlagForced = 1 << 2,
    kGCCallbackFlagSynchronousPhantomCallbackProcessing = 1 << 3,
    kGCCallbackFlagCollectAllAvailableGarbage = 1 << 4,
    kGCCallbackFlagCollectAllExternalMemory = 1 << 5,
    kGCCallbackScheduleIdleGarbageCollection = 1 << 6,
  };
 */

namespace ao { namespace metrics { namespace gc {
using namespace v8;

void set_histogram_values(hdr_histogram*, Local<Object>);

typedef struct GCData {
	uint64_t gcTime;
  uint64_t gcCount;
  uint64_t majorCount;
  uint64_t minorCount;
} GCData_t;

struct hdr_histogram* h_major;
struct hdr_histogram* h_minor;

GCData_t raw;
GCData_t interval_base;
GCData_t interval;

std::map<std::string, double> PERCENTILES = {
  {"p50", 50.0}, {"p75", 75.0}, {"p90", 90.0}, {"p95", 95.0}, {"p99", 99.0}
};

bool enabled = false;

uint64_t gcStartTime;

static NAN_GC_CALLBACK(recordBeforeGC) {
  gcStartTime = uv_hrtime();
}

NAN_GC_CALLBACK(afterGC) {
  // convert elapsed time to μ seconds
  uint64_t et = (uv_hrtime() - gcStartTime + 500) / 1000;

  // build histogram for elapsed times. scavenges are minor and
  // mark/sweep/compact are major. the other two types are just
  // phases of a major garbage collection and are typically very
  // low values that result in a large standard deviation and lower
  // mean if added into major garbage collections.
  // https://stackoverflow.com/questions/60171534/what-do-v8s-incrementalmarking-and-processweakcallbacks-garbage-collections-do/60172044#60172044
  if (type & kGCTypeMarkSweepCompact) {
    raw.majorCount += 1;
    interval.majorCount += 1;
    hdr_record_values(h_major, et, 1);
  } else if (type & kGCTypeScavenge) {
    raw.minorCount += 1;
    interval.minorCount += 1;
    hdr_record_values(h_minor, et, 1);
  } else {
    // don't update counts or time
    return;
  }

  // keep raw up-to-date
  raw.gcCount += 1;
  raw.gcTime += et;

  // keep the interval data up-to-date
  interval.gcCount += 1;
  interval.gcTime += et;
}


bool initialize() {
  return true;
}

//
// start recording GC events
//
bool start() {
  if (enabled) {
    return false;
  }
  enabled = true;

  // interval base is data at last fetch of interval information so
  // the first fetch is everything before.
  memset(&interval_base, 0, sizeof(interval_base));
  memset(&interval, 0, sizeof(interval));

  hdr_init(1, INT64_C(3600000000), 3, &h_major);
  hdr_init(1, INT64_C(3600000000), 3, &h_minor);

  Nan::AddGCPrologueCallback(recordBeforeGC);
  Nan::AddGCEpilogueCallback(afterGC);

  return true;
}

bool stop () {
  int status = enabled;

  if (enabled) {
    enabled = false;
    memset(&raw, 0, sizeof(raw));
    memset(&interval_base, 0, sizeof(interval_base));
    memset(&interval, 0, sizeof(interval));
    Nan::RemoveGCPrologueCallback(recordBeforeGC);
    Nan::RemoveGCEpilogueCallback(afterGC);
    hdr_close(h_major);
    h_major = nullptr;
    hdr_close(h_minor);
    h_minor = nullptr;
  }

  // if it wasn't enabled return false indicating so, else true.
  return status;
}

bool getInterval(const v8::Local<v8::Object> obj) {

  if (!enabled) {
    return false;
  }

  uint64_t count = raw.gcCount - interval_base.gcCount;
  Nan::Set(obj, Nan::New("gcCount").ToLocalChecked(), Nan::New<Number>(count));

  uint64_t time = raw.gcTime - interval_base.gcTime;
  Nan::Set(obj, Nan::New("gcTime").ToLocalChecked(), Nan::New<Number>(time));

  uint64_t majorCount = raw.majorCount - interval_base.majorCount;
  uint64_t minorCount = raw.minorCount - interval_base.minorCount;

  Local<Object> major = Nan::New<Object>();
  Local<Object> minor = Nan::New<Object>();

  set_histogram_values(h_major, major);
  set_histogram_values(h_minor, minor);

  Nan::Set(obj, Nan::New("major").ToLocalChecked(), major);
  Nan::Set(obj, Nan::New("minor").ToLocalChecked(), minor);

  Nan::Set(major, Nan::New("count").ToLocalChecked(), Nan::New<Number>(majorCount));
  Nan::Set(minor, Nan::New("count").ToLocalChecked(), Nan::New<Number>(minorCount));

  // reset the interval base values
  interval_base.gcCount = raw.gcCount;
  interval_base.gcTime = raw.gcTime;
  interval_base.majorCount = raw.majorCount;
  interval_base.minorCount = raw.minorCount;

  // reset the histograms
  hdr_reset(h_major);
  hdr_reset(h_minor);

  return true;
}

void set_histogram_values(hdr_histogram* h, Local<Object> obj) {
  std::map<std::string, double>::iterator it;
  for (it = PERCENTILES.begin(); it != PERCENTILES.end(); it++) {
    const int64_t p = hdr_value_at_percentile(h, it->second);
    Nan::Set(obj, Nan::New(it->first).ToLocalChecked(), Nan::New<Number>(p));
  }

  // the next two values are NaN if no GCs executed so default them to 0.
  // It's cheating a little bit by looking into hists code but min is set to
  // INT64_MAX during histogram initialization.
  double mean = 0;
  double stddev = 0;
  int64_t max = 0;

  int64_t min = hdr_min(h);

  // this checks to see if the histogram is empty.
  if (min < INT64_MAX) {
    max = hdr_max(h);
    mean = hdr_mean(h);
    stddev = hdr_stddev(h);
    // reset the histogram if it received any data.
    hdr_reset(h);
  }

  Nan::Set(obj, Nan::New("min").ToLocalChecked(), Nan::New<Number>(min));
  Nan::Set(obj, Nan::New("max").ToLocalChecked(), Nan::New<Number>(max));
  Nan::Set(obj, Nan::New("mean").ToLocalChecked(), Nan::New<Number>(mean));
  Nan::Set(obj, Nan::New("stddev").ToLocalChecked(), Nan::New<Number>(stddev));
}

}}} // namespace ao::metrics::gc
