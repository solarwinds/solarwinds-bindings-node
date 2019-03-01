#ifndef OBOE_CONFIG_H
#define OBOE_CONFIG_H

#include "bindings.h"

Napi::Value getVersionString(const Napi::CallbackInfo& info) {
  const char* version = oboe_config_get_version_string();
  return Napi::String::New(info.Env(), version);
}

Napi::Value getConfigSettings(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  oboe_settings_cfg_t* cfg = oboe_settings_cfg_get();

  // create the return object
  Napi::Object o = Napi::Object::New(env);

  Napi::Object config = Napi::Object::New(env);
  o.Set("config", config);
  Napi::Object stats = Napi::Object::New(env);
  o.Set("stats", stats);

  if (cfg != NULL) {
    #define aoSAMPLE_START OBOE_SETTINGS_FLAG_SAMPLE_START
    #define aoTHROUGH_ALWAYS OBOE_SETTINGS_FLAG_SAMPLE_THROUGH_ALWAYS

    config.Set("tracing_mode", Napi::Number::New(env, cfg->tracing_mode));
    config.Set("sample_rate", Napi::Number::New(env, cfg->sample_rate));
    config.Set("flag_sample_start", Napi::Boolean::New(env, cfg->settings && cfg->settings->flags & aoSAMPLE_START));
    config.Set("flag_through_always", Napi::Boolean::New(env, cfg->settings && cfg->settings->flags & aoTHROUGH_ALWAYS));
  }

  oboe_internal_stats_t* ostats = oboe_get_internal_stats();

  if (stats != NULL) {
    stats.Set("reporterInitCount", Napi::Number::New(env, ostats->reporters_initialized));
  }

  return o;
}

//
// put Config in a separate JavaScript namespace.
//
namespace Config {

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Object module = Napi::Object::New(env);
  module.Set("getVersionString", Napi::Function::New(env, getVersionString));
  module.Set("getSettings", Napi::Function::New(env, getConfigSettings));

  exports.Set("Config", module);

  return exports;
}

} // end namespace Config

#endif
