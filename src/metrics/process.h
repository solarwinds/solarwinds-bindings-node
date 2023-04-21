#pragma once

#include <napi.h>

namespace ao { namespace metrics { namespace process {
  bool start();
  bool stop();
  bool getInterval(Napi::Object&);
}}}
