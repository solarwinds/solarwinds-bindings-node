#include "bindings.h"

// Components
#include "sanitizer.cc"
#include "metadata.cc"
#include "context.cc"
#include "config.cc"
#include "event.cc"
#include "reporter.cc"

NAN_METHOD(oboeInit) {

    if (info.Length() < 1 || !info[0]->IsString() || (info.Length() > 1 && !info[1]->IsObject())) {
        return Nan::ThrowError("oboeInit - invalid calling signature");
    }
    Nan::Utf8String service_key(info[0]);

    // setup the options
    oboe_init_options_t options;
    options.version = 1;
    options.hostname_alias = NULL;
    options.log_level = 0;
    options.log_file_path = NULL;
    options.max_transactions = 0;

    // if not options supplied then init with an empty struct.
    if (info.Length() == 1) {
        oboe_init(*service_key, &options);
        return;
    }

    v8::Local<v8::Object> opts = info[1]->ToObject();
    // this is the only field in oboe_init_options_t that node uses at this point
    v8::Local<v8::String> hostnameAlias = Nan::New<v8::String>("hostnameAlias").ToLocalChecked();
    std::string alias;

    if (Nan::Has(opts, hostnameAlias).FromMaybe(false)) {
        Nan::MaybeLocal<v8::Value> value = Nan::Get(opts, hostnameAlias);
        if (!value.IsEmpty()) {
            alias = *Nan::Utf8String(value.ToLocalChecked()->ToString());
            options.hostname_alias = alias.c_str();
        } else {
            // there is no hostnameAlias available. just leave it empty.
        }
    }

    oboe_init(*service_key, &options);
}

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

  Nan::SetMethod(exports, "oboeInit", oboeInit);

  Reporter::Init(exports);
  OboeContext::Init(exports);
  Sanitizer::Init(exports);
  Metadata::Init(exports);
  Event::Init(exports);
  Config::Init(exports);

}

NODE_MODULE(appoptics_bindings, init)

}
