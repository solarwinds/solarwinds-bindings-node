#include <uv.h>
#include <map>

#include "hdr_histogram.h"
#include "process.h"

namespace ao { namespace metrics { namespace process {

bool enabled = true;
uv_rusage_t prev_rusage;

bool start() {
  uv_getrusage(&prev_rusage);
  enabled = true;
  return true;
}

bool stop() {
  enabled = false;
  return true;
}

int64_t usec_time (uv_timeval_t t) {
  return t.tv_sec * 1000 * 1000 + t.tv_usec;
}

//
// get interval data
//
bool getInterval(Napi::Object& obj) {
  if (!enabled) {
    return false;
  }

  uv_rusage_t rusage;
  uv_getrusage(&rusage);

  const uint64_t user_time = usec_time(rusage.ru_utime) - usec_time(prev_rusage.ru_utime);
  const uint64_t sys_time = usec_time(rusage.ru_stime) - usec_time(prev_rusage.ru_stime);

  obj.Set("user", Napi::Number::New(obj.Env(), user_time));
  obj.Set("system", Napi::Number::New(obj.Env(), sys_time));

  prev_rusage = rusage;

  return true;
}


}}} // ao::metrics::eventloop
