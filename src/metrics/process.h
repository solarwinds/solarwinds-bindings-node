#pragma once

#include <nan.h>

namespace ao::metrics::process {
  bool start();
  bool stop();
  bool getInterval(const v8::Local<v8::Object>);
}
