#pragma once

#include <nan.h>

namespace ao { namespace metrics { namespace eventloop {
  bool start();
  bool stop();
  bool getInterval(const v8::Local<v8::Object>);
}}}
