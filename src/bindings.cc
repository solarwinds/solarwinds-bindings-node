#include "bindings.h"

// Components
#include "sanitizer.cc"
#include "metadata.cc"
#include "context.cc"
#include "config.cc"
#include "event.cc"
#include "reporter.cc"

Napi::Value oboeInit(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString() || (info.Length() > 1 && !info[1].IsObject())) {
        Napi::TypeError::New(env, "invalid arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    std::string service_key = info[0].As<Napi::String>();

    // setup the options
    oboe_init_options_t options;
    options.version = 1;
    options.hostname_alias = NULL;
    options.log_level = 0;
    options.log_file_path = NULL;
    options.max_transactions = 0;

    // if not options supplied then init with an empty struct.
    if (info.Length() == 1) {
        oboe_init(service_key.c_str(), &options);
        return env.Null();
    }

    Napi::Object opts = info[1].ToObject();

    // hostnameAlias is the only field in oboe_init_options_t that node uses at this point
    if (opts.Has("hostnameAlias")) {
        Napi::Value alias = opts.Get("hostnameAlias");
        if (!alias.IsEmpty()) {
            //Napi::Value aliasString = alias.As<Napi::String>();
            std::string aliasString = alias.As<Napi::String>();
            options.hostname_alias = aliasString.c_str();
        }
    }

    oboe_init(service_key.c_str(), &options);
    return env.Null();
}


// Register the exposed parts of the module
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  // should I set a new handle scope?
  Napi::HandleScope scope(env);

  exports.Set("MAX_SAMPLE_RATE", Napi::Number::New(env, OBOE_SAMPLE_RESOLUTION));
  exports.Set("MAX_METADATA_PACK_LEN", Napi::Number::New(env, OBOE_MAX_METADATA_PACK_LEN));
  exports.Set("MAX_TASK_ID_LEN", Napi::Number::New(env, OBOE_MAX_TASK_ID_LEN));
  exports.Set("MAX_OP_ID_LEN", Napi::Number::New(env, OBOE_MAX_OP_ID_LEN));
  exports.Set("TRACE_NEVER", Napi::Number::New(env, OBOE_TRACE_NEVER));
  exports.Set("TRACE_ALWAYS", Napi::Number::New(env, OBOE_TRACE_ALWAYS));

  exports.Set("oboeInit", Napi::Function::New(env, oboeInit));

  std::cout << "before Init()s\n";
  exports = Reporter::Init(env, exports);
  std::cout << "after reporter Init()\n";
  exports = OboeContext::Init(env, exports);
  exports = Sanitizer::Init(env, exports);
  exports = Metadata::Init(env, exports);
  exports = Event::Init(env, exports);
  exports = Config::Init(env, exports);

  return exports;

}

NODE_API_MODULE(appoptics_bindings, Init)
