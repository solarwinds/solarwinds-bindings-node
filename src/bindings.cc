#include "bindings.h"

// Components
#include "sanitizer.cc"
#include "metadata.cc"
#include "context.cc"
#include "config.cc"
#include "event.cc"
#include "reporter.cc"

extern "C" {

// Register the exposed parts of the module
void init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  Nan::Set(exports, Nan::New("MAX_SAMPLE_RATE").ToLocalChecked(), Nan::New(OBOE_SAMPLE_RESOLUTION));
  Nan::Set(exports, Nan::New("MAX_METADATA_PACK_LEN").ToLocalChecked(), Nan::New(OBOE_MAX_METADATA_PACK_LEN));
  Nan::Set(exports, Nan::New("MAX_TASK_ID_LEN").ToLocalChecked(), Nan::New(OBOE_MAX_TASK_ID_LEN));
  Nan::Set(exports, Nan::New("MAX_OP_ID_LEN").ToLocalChecked(), Nan::New(OBOE_MAX_OP_ID_LEN));
  Nan::Set(exports, Nan::New("TRACE_NEVER").ToLocalChecked(), Nan::New(OBOE_TRACE_NEVER));
  Nan::Set(exports, Nan::New("TRACE_ALWAYS").ToLocalChecked(), Nan::New(OBOE_TRACE_ALWAYS));

  Reporter::Init(exports);
  OboeContext::Init(exports);
  Sanitizer::Init(exports);
  Metadata::Init(exports);
  Event::Init(exports);
  Config::Init(exports);

  char* key = getenv("APPOPTICS_SERVICE_KEY");
  oboe_init(key);
}

NODE_MODULE(appoptics_bindings, init)

}
