#pragma once

#include <napi.h>

namespace ao { namespace metrics { namespace gc {
  bool start();
  bool stop();
  bool getInterval(Napi::Object&);
}}}
