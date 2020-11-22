## 10.1.0 lambda

1. ~~read env var APPOPTICS_SERVICE_NAME and pass to oboe_init() (set
lambda_service_name). Lambda doesn't need the key portion, just the name
portion of the  APM service key. service name is portion of service key
after ":". must use version 10 of oboe_init_options_t for the lambda_service_name
to be recognized.~~
2. empty service key is now allowed for oboe_init() in case of lambda (for ssl
reporter, indicated by error code OBOE_INIT_SSL_MISSING_KEY)
3. on agent startup if env var APPOPTICS_SAMPLE_RATE is set, use it to set local
sample rate with oboe_settings_rate_set(). this is needed because oboe receives
no messages from the collector. also re-enable TOKEN_BUCKET_RATE and _CAPACITY.
4. ~~get awsRequestId and pass into oboe via oboe_reporter_set_request_properties()
(https://docs.aws.amazon.com/lambda/latest/dg/nodejs-context.html). This needs to
happen before calling  oboe_reporter_flush()~~
5. call oboe_reporter_flush() when done handling request (after sending exit
event). can call oboe_get_reporter_type() to get type of reporter if desired.
6. 1 and 4 are now obsolete due to changes in approach.

- lambda detection both AWS_LAMBDA_FUNCTION_NAME and LAMBDA_TASK_ROOT in env.

## 6.0.0

1. use N-API (don't need to compile for supported environments)
2. remove all friend classes
3. convert classes with only static methods to namespaces.
4. Metadata
  - remove methods getMetadataData, copy, createEvent
5. Event
  - remove getEventData
6. OboeContext => namespace.
  - removed copy, startTrace.
  - added getTraceSettings
7. Reporter => namespace.
8. Utility (removed class).
9. Config => namespace.
  - removed getRevision, getVersion, checkVersion.
10. Sanitizer => namespace.
11. removed download capability in setup-liboboe (package dependencies went from 50 => 3)
12. added prepack script (one build fits all).
13. cleaned up compile-time dependencies (moved node-gyp and node-addon-api to devDependencies)


build system dependency notes
- catalog changes at class/function level
- alpine 3.4.6 doesn't run bindings as built - __printf_chk: symbol not found
- debian 8 doesn't run bindings as built - GLIBCXX_3.4.21 not found
-  (looks like maybe will run if built with GLIBCXX_3.4.)
-  Debian 8 has GLIBC "2.19-18+deb8u10", 2014 compiled by GNU CC 4.8.4.
- appoptics-bindings.node built on debian runs on alpine, debian, ubuntu
- appoptics-bindings.node built on alpine does not run on debian (links to musl)
-> production build process should build in a debian container.

pending
- consider holding context buffer in cls?
