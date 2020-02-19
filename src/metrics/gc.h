#pragma once

#include <nan.h>

namespace ao { namespace metrics { namespace gc {
  bool start();
  bool stop();
  bool getInterval(const v8::Local<v8::Object>);
}}}
