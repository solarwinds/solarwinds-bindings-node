// portion of env var after 'APPOPTICS_': {bindings-name, type: [s]tring, [i]nteger, or [b]oolean}
const keyMap = {
  // these have been documented for end-users; the names should not be changed
  SERVICE_KEY: {name: 'serviceKey', type: 's'},
  TRUSTEDPATH: {name: 'trustedPath', type: 's'},
  HOSTNAME_ALIAS: {name: 'hostnameAlias', type: 's'},
  DEBUG_LEVEL: {name: 'logLevel', type: 'i'},

  // feel free to rationalize the following

  // used by node agent
  REPORTER: {name: 'reporter', type: 's'},
  COLLECTOR: {name: 'endpoint', type: 's'},
  TOKEN_BUCKET_CAPACITY: {name: 'tokenBucketCapacity', type: 'i'},      // file and udp reporter
  TOKEN_BUCKET_RATE: {name: 'tokenBucketRate', type: 'i'},              // file and udp reporter

  // not used by node agent
  BUFSIZE: {name: 'bufferSize', type: 'i'},
  LOGNAME: {name: 'logFilePath', type: 's'},
  TRACE_METRICS: {name: 'traceMetrics', type: 'b'},
  HISTOGRAM_PRECISION: {name: 'histogramPrecision', type: 'i'},
  MAX_TRANSACTIONS: {name: 'maxTransactions', type: 'i'},
  FLUSH_MAX_WAIT_TIME: {name: 'flushMaxWaitTime', type: 'i'},
  EVENTS_FLUSH_INTERVAL: {name: 'eventsFlushInterval', type: 'i'},
  EVENTS_FLUSH_BATCH_SIZE: {name: 'eventsFlushBatchSize', type: 'i'},
  REPORTER_FILE_SINGLE: {name: 'oneFilePerEvent', type: 'b'},           // file reporter

  // supported in bindings
  EC2_METADATA_TIMEOUT: {name: 'ec2MetadataTimeout', type: 'i'},
}

module.exports = keyMap;
