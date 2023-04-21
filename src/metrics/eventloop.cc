#include <uv.h>
#include <map>

#include "hdr_histogram.h"
#include "eventloop.h"

namespace ao { namespace metrics { namespace eventloop {

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

bool getInterval(Napi::Object& obj) {
  if (!enabled) {
    return false;
  }
  // add each configured percentile
  std::map<std::string, double>::iterator it;
  for (it = PERCENTILES.begin(); it != PERCENTILES.end(); it++) {
    const int64_t p = hdr_value_at_percentile(hist, it->second);
    obj.Set(it->first, Napi::Number::New(obj.Env(), p));
  }

  // the next two values are NaN if no eventloops executed so default them to 0.
  // It's cheating a little bit by looking into hists code but min is set to
  // INT64_MAX during histogram initialization.
  double mean = 0;
  double stddev = 0;
  int64_t max = 0;

  int64_t min = hdr_min(hist);

  // this checks to see if this histogram is empty.
  if (min < INT64_MAX) {
    max = hdr_max(hist);
    mean = hdr_mean(hist);
    stddev = hdr_stddev(hist);
    // reset the histogram if it received any data.
    hdr_reset(hist);
  }

  obj.Set("min", Napi::Number::New(obj.Env(), min));
  obj.Set("max", Napi::Number::New(obj.Env(), max));
  obj.Set("mean", Napi::Number::New(obj.Env(), mean));
  obj.Set("stddev", Napi::Number::New(obj.Env(), stddev));

  return true;
}

}}} // ao::metrics::eventloop
