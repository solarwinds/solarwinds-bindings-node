#pragma once

#include <napi.h>

namespace ao { namespace metrics { namespace eventloop {
  bool start();
  bool stop();
  bool getInterval(Napi::Object&);
}}}
