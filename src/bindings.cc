#include "bindings.h"

// Components
#include "sanitizer.cc"
#include "metadata.cc"
#include "context.cc"
#include "config.cc"
#include "event.cc"
#include "reporter.cc"

//
// Initialize oboe
//
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

    // if not options supplied then init with default values.
    if (info.Length() == 1) {
        oboe_init(service_key.c_str(), &options);
        return env.Null();
    }

    Napi::Object opts = info[1].ToObject();
    std::string aliasString;

    // hostnameAlias is the only field in oboe_init_options_t that node uses at this point
    if (opts.Has("hostnameAlias")) {
        Napi::Value alias = opts.Get("hostnameAlias");
        if (!alias.IsEmpty()) {
            aliasString = alias.As<Napi::String>();
            options.hostname_alias = aliasString.c_str();
        }
    }

    oboe_init(service_key.c_str(), &options);
    return env.Null();
}

//
// simple utility to output using C++. Sometimes node doesn't
// manage to output console.log() calls before there is a problem.
//
Napi::Value o (const Napi::CallbackInfo& info) {
  if (info.Length() < 1) {
    return info.Env().Undefined();
  }
  for (uint i = 0; i < info.Length(); i++) {
    std::string s = info[i].ToString();
    std::cout << s;
  }
  // return the number of items output
  return Napi::Number::New(info.Env(), info.Length());
}

//
// Expose public parts of this module and invoke the other classes
// and namespace objects own Init() functions.
//
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  // constants
  exports.Set("MAX_SAMPLE_RATE", Napi::Number::New(env, OBOE_SAMPLE_RESOLUTION));
  exports.Set("MAX_METADATA_PACK_LEN", Napi::Number::New(env, OBOE_MAX_METADATA_PACK_LEN));
  exports.Set("MAX_TASK_ID_LEN", Napi::Number::New(env, OBOE_MAX_TASK_ID_LEN));
  exports.Set("MAX_OP_ID_LEN", Napi::Number::New(env, OBOE_MAX_OP_ID_LEN));
  exports.Set("TRACE_NEVER", Napi::Number::New(env, OBOE_TRACE_NEVER));
  exports.Set("TRACE_ALWAYS", Napi::Number::New(env, OBOE_TRACE_ALWAYS));

  // functions directly on the bindings object
  exports.Set("oboeInit", Napi::Function::New(env, oboeInit));
  exports.Set("o", Napi::Function::New(env, o));

  // classes and objects supplying different namespaces
  exports = Reporter::Init(env, exports);
  exports = OboeContext::Init(env, exports);
  exports = Sanitizer::Init(env, exports);
  exports = Metadata::Init(env, exports);
  exports = Event::Init(env, exports);
  exports = Config::Init(env, exports);

  return exports;

}

NODE_API_MODULE(appoptics_bindings, Init)
