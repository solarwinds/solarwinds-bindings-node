#include <nan.h>
#include <math.h>
#include <iostream>
#include <map>
#include "hdr_histogram.h"

using namespace v8;

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

const int maxTypeCount = kGCTypeAll + 1;
typedef struct GCData {
	uint64_t gcTime;
    uint64_t gcCount;
    uint64_t typeCounts[maxTypeCount];
} GCData_t;

struct hdr_histogram* histogram;

GCData_t raw;
GCData_t cumulative_base;
GCData_t cumulative;

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
  cumulative.typeCounts[index] += 1;

  // keep cumulative up-to-date
  cumulative.gcCount += 1;
  cumulative.gcTime += et;
}

//
// start recording GC events
//
static NAN_METHOD(start) {
  if (enabled) {
      Nan::ThrowError("already started");
      return;
  }
  enabled = true;

  // cumulative base is data at last fetch of cumulative information so
  // the first fetch is everything before.
  memset(&cumulative_base, 0, sizeof(cumulative_base));
  memset(&cumulative, 0, sizeof(cumulative));

  hdr_init(1, INT64_C(3600000000), 3, &histogram);

  Nan::AddGCPrologueCallback(recordBeforeGC);
  Nan::AddGCEpilogueCallback(afterGC);

  info.GetReturnValue().Set(Nan::New<Number>(1));
}

static NAN_METHOD(stop) {
    int status = enabled ? 1 : 0;

    if (enabled) {
        enabled = false;
        memset(&raw, 0, sizeof(raw));
        memset(&cumulative_base, 0, sizeof(cumulative_base));
        memset(&cumulative, 0, sizeof(cumulative));
        Nan::RemoveGCPrologueCallback(recordBeforeGC);
        Nan::RemoveGCEpilogueCallback(afterGC);
        hdr_close(histogram);
        histogram = nullptr;
    }

    info.GetReturnValue().Set(Nan::New<Number>(status));
}

void getCumulative(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();

    v8::Local<v8::Object> obj = Nan::New<v8::Object>();

    // assign to b to avoid unused result warnings. if Set() fails there are
    // big problems.
    uint64_t count = raw.gcCount - cumulative_base.gcCount;
    v8::Maybe<bool> b = obj->Set(context, Nan::New("gcCount").ToLocalChecked(),
        Nan::New<Number>(count));

    uint64_t time = raw.gcTime - cumulative_base.gcTime;
    b = obj->Set(context, Nan::New("gcTime").ToLocalChecked(),
        Nan::New<Number>(time));

    // int64_t value = hdr_value_at_percentile(h, double percentile);
    // percentile is 99.0, 100.0, etc.
    // double mean = hdr_mean(h); double stddev = hdr_stddev(h);
    // int64_t min = hdr_min(h); int64_t max = hdr_max(h);
    std::map<std::string, double>::iterator it;
    for (it = PERCENTILES.begin(); it != PERCENTILES.end(); it++) {
      const int64_t p = hdr_value_at_percentile(histogram, it->second);
      b = obj->Set(context, Nan::New(it->first).ToLocalChecked(),
          Nan::New<Number>(p));
    }
    b = obj->Set(context, Nan::New("min").ToLocalChecked(),
        Nan::New<Number>(hdr_min(histogram)));
    b = obj->Set(context, Nan::New("max").ToLocalChecked(),
        Nan::New<Number>(hdr_max(histogram)));
    // the next two values are NaN if no GCs so default them to 0.
    const double mean = count ? hdr_mean(histogram) : 0;
    b = obj->Set(context, Nan::New("mean").ToLocalChecked(), Nan::New<Number>(mean));
    const double stddev = count ? hdr_stddev(histogram) : 0;
    b = obj->Set(context, Nan::New("stddev").ToLocalChecked(), Nan::New<Number>(stddev));

    Local<Object> counts = Nan::New<v8::Object>();
    for (int i = 0; i < maxTypeCount; i++) {
      uint64_t count = raw.typeCounts[i] - cumulative_base.typeCounts[i];
      if (count != 0) {
        Nan::Set(counts, i, Nan::New<Number>(count));
      }
      // reset the base
      cumulative_base.typeCounts[i] = raw.typeCounts[i];
    }
    Nan::Set(obj, Nan::New("gcTypeCounts").ToLocalChecked(), counts);

    // reset the cumulative base values
    cumulative_base.gcCount = raw.gcCount;
    cumulative_base.gcTime = raw.gcTime;

    // reset the histogram
    hdr_reset(histogram);

    info.GetReturnValue().Set(obj);
}

NAN_MODULE_INIT(init) {
	Nan::HandleScope scope;

	Nan::Set(target, Nan::New("start").ToLocalChecked(),
      Nan::GetFunction(Nan::New<FunctionTemplate>(start)).ToLocalChecked());
	Nan::Set(target, Nan::New("stop").ToLocalChecked(),
      Nan::GetFunction(Nan::New<FunctionTemplate>(stop)).ToLocalChecked());

    Nan::Set(target, Nan::New("getCumulative").ToLocalChecked(),
      Nan::GetFunction(Nan::New<FunctionTemplate>(getCumulative)).ToLocalChecked());
}

NODE_MODULE(metrics, init)

//********************************************************************************************
//********************************************************************************************
// #include "hdr_histogram.h"
//
// struct hdr_histogram* h;
// hdr_init(1, INT64_C(3600000000), 3, &h);
//
// hdr_record_values(h, value, 1);
// hdr_record_value() just calls hdr_record_values() with a 1
//
// int64_t value = hdr_value_at_percentile(h, double percentile); // percentile
// is 99.0, 100.0, etc. double mean = hdr_mean(h); double stddev =
// hdr_stddev(h); int64_t min = hdr_min(h); int64_t max = hdr_max(h);
//
// hdr_reset(h); // reuse histogram.
// hdr_close(h); // if not null.
//
