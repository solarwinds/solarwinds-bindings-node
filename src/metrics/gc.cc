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

namespace ao::metrics::gc {
using namespace v8;

const int maxTypeCount = kGCTypeAll + 1;
typedef struct GCData {
	uint64_t gcTime;
    uint64_t gcCount;
    uint64_t typeCounts[maxTypeCount];
} GCData_t;

struct hdr_histogram* histogram;

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

  // build histogram for elapsed times
  hdr_record_values(histogram, et, 1);

  // keep raw up-to-date
  raw.gcCount += 1;
  raw.gcTime += et;

  const int index = type & kGCTypeAll;
  raw.typeCounts[index] += 1;
  interval.typeCounts[index] += 1;

  // keep cumulative up-to-date
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

  hdr_init(1, INT64_C(3600000000), 3, &histogram);

  Nan::AddGCPrologueCallback(recordBeforeGC);
  Nan::AddGCEpilogueCallback(afterGC);

  return true;
}

bool stop () {
  int status = enabled ? true : false;

  if (enabled) {
    enabled = false;
    memset(&raw, 0, sizeof(raw));
    memset(&interval_base, 0, sizeof(interval_base));
    memset(&interval, 0, sizeof(interval));
    Nan::RemoveGCPrologueCallback(recordBeforeGC);
    Nan::RemoveGCEpilogueCallback(afterGC);
    hdr_close(histogram);
    histogram = nullptr;
  }

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

  // int64_t value = hdr_value_at_percentile(h, double percentile);
  // percentile is 99.0, 100.0, etc.
  // double mean = hdr_mean(h); double stddev = hdr_stddev(h);
  // int64_t min = hdr_min(h); int64_t max = hdr_max(h);
  std::map<std::string, double>::iterator it;
  for (it = PERCENTILES.begin(); it != PERCENTILES.end(); it++) {
    const int64_t p = hdr_value_at_percentile(histogram, it->second);
    Nan::Set(obj, Nan::New(it->first).ToLocalChecked(), Nan::New<Number>(p));
  }
  Nan::Set(obj, Nan::New("min").ToLocalChecked(), Nan::New<Number>(hdr_min(histogram)));
  Nan::Set(obj, Nan::New("max").ToLocalChecked(), Nan::New<Number>(hdr_max(histogram)));

  // the next two values are NaN if no GCs so default them to 0.
  const double mean = count ? hdr_mean(histogram) : 0;
  Nan::Set(obj, Nan::New("mean").ToLocalChecked(), Nan::New<Number>(mean));
  const double stddev = count ? hdr_stddev(histogram) : 0;
  Nan::Set(obj, Nan::New("stddev").ToLocalChecked(), Nan::New<Number>(stddev));

  Local<Object> counts = Nan::New<Object>();
  for (int i = 0; i < maxTypeCount; i++) {
    uint64_t count = raw.typeCounts[i] - interval_base.typeCounts[i];
    if (count != 0) {
      Nan::Set(counts, i, Nan::New<Number>(count));
    }
    // reset the base
    interval_base.typeCounts[i] = raw.typeCounts[i];
  }
  Nan::Set(obj, Nan::New("gcTypeCounts").ToLocalChecked(), counts);

  // reset the interval base values
  interval_base.gcCount = raw.gcCount;
  interval_base.gcTime = raw.gcTime;

  // reset the histogram
  hdr_reset(histogram);

  return true;
}


} // namespace ao::metrics::gc
