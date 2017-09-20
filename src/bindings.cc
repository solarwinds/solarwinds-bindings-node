#include "bindings.h"

// Components
#include "sanitizer.cc"
#include "metadata.cc"
#include "context.cc"
#include "config.cc"
#include "event.cc"
#include "reporter.cc"
//#include "reporters/udp.cc"
//#include "reporters/file.cc"

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
  //Nan::Set(exports, Nan::New("TRACE_THROUGH").ToLocalChecked(), Nan::New(OBOE_TRACE_THROUGH));

  //FileReporter::Init(exports);
  //UdpReporter::Init(exports);
  Reporter::Init(exports);
  OboeContext::Init(exports);
  Sanitizer::Init(exports);
  Metadata::Init(exports);
  Event::Init(exports);
  Config::Init(exports);

  // hardcode service key for now
  oboe_init("795fb4947d15275d208c49cfd2412d4a5bf38742045b47236c94c4fe5f5b17c7:bruce-test");
}

NODE_MODULE(appoptics_bindings, init)

}
