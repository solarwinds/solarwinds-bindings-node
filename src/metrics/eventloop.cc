#include <nan.h>
#include <map>

#include "hdr_histogram.h"
#include "eventloop.h"

namespace ao { namespace metrics { namespace eventloop {
using namespace v8;

// state = 0 not initialized
// state = 1 initialized
bool enabled = false;

uv_check_t check_handle;
uv_prepare_t prepare_handle;
uint64_t prev_check_time;
uint64_t prepare_time;
uint64_t poll_timeout;

struct hdr_histogram* hist;
std::map<std::string, double> PERCENTILES = {
  {"p50", 50.0}, {"p75", 75.0}, {"p90", 90.0}, {"p95", 95.0}, {"p99", 99.0}
};

//
// the check callback, after i/o polling
//
void check_cb(uv_check_t* handle) {
  uint64_t check_time = uv_hrtime();

  // get the time i/o polling took
  uint64_t poll_time = check_time - prepare_time;

  // before-io-polling t - previous-after-polling t
  uint64_t latency = prepare_time - prev_check_time;

  // get n-sec timeout from before i/o polling
  uint64_t timeout = poll_timeout * 1000 * 1000;

  // if polling took longer than the timeout add that to latency
  if (poll_time > timeout) {
    latency += poll_time - timeout;
  }

  // convert latency to μ seconds and record it.
  hdr_record_values(hist, (latency + 500) / 1000, 1);

  // reset time mark
  prev_check_time = check_time;
}

//
// the prepare callback, before i/o polling
//
void prepare_cb(uv_prepare_t* handle) {
  // mark time before i/o polling
  prepare_time = uv_hrtime();

  // get i/o polling timeout for this iteration because it's adjusted
  // each loop, if needed, to prevent drift.
  poll_timeout = uv_backend_timeout(uv_default_loop());
}

//
// start, stop, getInterval
//
bool start() {
  if (!enabled) {
    uv_check_init(uv_default_loop(), &check_handle);
    uv_prepare_init(uv_default_loop(), &prepare_handle);
    uv_unref(reinterpret_cast<uv_handle_t*>(&check_handle));
    uv_unref(reinterpret_cast<uv_handle_t*>(&prepare_handle));

    hdr_init(1, INT64_C(3600000000), 3, &hist);

    prev_check_time = uv_hrtime();

    // start listening to prepare and check callbacks.
    uv_check_start(&check_handle, &check_cb);
    uv_prepare_start(&prepare_handle, &prepare_cb);
    enabled = true;
    return true;
  }
  return false;
}

bool stop() {
  if (!enabled) {
    return false;
  }
  uv_check_stop(&check_handle);
  uv_prepare_stop(&prepare_handle);
  enabled = false;
  return true;
}

bool getInterval(const v8::Local<v8::Object> obj) {
  if (!enabled) {
    return false;
  }
  // add each configured percentile
  std::map<std::string, double>::iterator it;
  for (it = PERCENTILES.begin(); it != PERCENTILES.end(); it++) {
    const int64_t p = hdr_value_at_percentile(hist, it->second);
    Nan::Set(obj, Nan::New(it->first).ToLocalChecked(), Nan::New<Number>(p));
  }
  int64_t min = hdr_min(hist);
  Nan::Set(obj, Nan::New("min").ToLocalChecked(), Nan::New<Number>(min));
  Nan::Set(obj, Nan::New("max").ToLocalChecked(), Nan::New<Number>(hdr_max(hist)));

  // use min < INT64_MAX as indication that at least one value was added
  // to the histogram.
  bool value_added = min < INT64_MAX;

  // the next two values are NaN if no values added so default them to 0.
  const double mean = value_added ? hdr_mean(hist) : 0;
  Nan::Set(obj, Nan::New("mean").ToLocalChecked(), Nan::New<Number>(mean));
  const double stddev = value_added ? hdr_stddev(hist) : 0;
  Nan::Set(obj, Nan::New("stddev").ToLocalChecked(), Nan::New<Number>(stddev));

  // reset the histogram
  hdr_reset(hist);

  return true;
}

}}} // ao::metrics::eventloop
