{
'targets': [{
    'target_name': 'apm_bindings',
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'xcode_settings': { 'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'CLANG_CXX_LIBRARY': 'libc++',
        'MACOSX_DEPLOYMENT_TARGET': '10.7',
      },
      'msvs_settings': {
        'VCCLCompilerTool': { 'ExceptionHandling': 1 },
      },
    'include_dirs': [
      '<!@(node --no-warnings -p "require(\'node-addon-api\').include")',
    ],
    'defines': [
      'NAPI_VERSION=<(napi_build_version)'
    ],
    # preprocessor only (in bindings.o for some reason)
    #'cflags': ['-E'],
    # suppress warnings
    #'cflags': ['-w'],
    'sources': [
        'src/bindings.cc',
        'src/sanitizer.cc',
        'src/settings.cc',
        'src/config.cc',
        'src/event.cc',
        'src/event/event-to-string.cc',
        'src/event/event-send.cc',
        'src/reporter.cc',
    ],
    'conditions': [
        ['OS in "linux"', {
        # includes reference oboe/oboe.h, so
        'include_dirs': [
          '<!@(node --no-warnings -p "require(\'node-addon-api\').include")',
          '<(module_root_dir)/',
          '<(module_root_dir)/src',
        ],
        'libraries': [
            '-loboe',
            '-L<(module_root_dir)/oboe/',
            '-Wl,-rpath-link,<(module_root_dir)/oboe/',
            '-Wl,-rpath,\$$ORIGIN/../../oboe/'
        ],
        }]
    ]
  }, {
    'target_name': 'ao_metrics',
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'xcode_settings': { 'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'CLANG_CXX_LIBRARY': 'libc++',
        'MACOSX_DEPLOYMENT_TARGET': '10.7',
      },
      'msvs_settings': {
        'VCCLCompilerTool': { 'ExceptionHandling': 1 },
      },
    'include_dirs': [
      '<!@(node --no-warnings -p "require(\'nan\')")',
    ],
    'defines': [
      'NAPI_VERSION=<(napi_build_version)'
    ],
    # preprocessor only (in bindings.o for some reason)
    #'cflags': ['-E'],
    # suppress warnings
    #'cflags': ['-w'],
    'sources': [
        'src/metrics/metrics.cc',
        'src/metrics/gc.cc',
        'src/metrics/eventloop.cc',
        'src/metrics/process.cc'
    ],
    'conditions': [
        ['OS in "linux"', {
        # includes reference oboe/oboe.h, so
        'include_dirs': [
          '<!@(node --no-warnings -p "require(\'nan\')")',
          '<(module_root_dir)/',
          '<(module_root_dir)/metrics'
        ],
        'libraries': [
            '-loboe',
            '-L<(module_root_dir)/oboe/',
            '-Wl,-rpath-link,<(module_root_dir)/oboe/',
            '-Wl,-rpath,\$$ORIGIN/../../oboe/'
        ],
        }]
    ]
  }, {
    "target_name": "action_after_build",
    "type": "none",
    "dependencies": ["<(module_name)"],
    "copies": [
      {
        "files": [
          # explicitly copy targets
          "<(PRODUCT_DIR)/apm_bindings.node",
          "<(PRODUCT_DIR)/ao_metrics.node",
          # this will be linked to the correct version of liboboe by setup-liboboe.js
          #"<(module_root_dir)/oboe/liboboe-1.0.so.0",
          #"<!(readlink -f <(module_root_dir)/oboe/liboboe-1.0.so.0)"
        ],
        "destination": "<(module_path)/"
      }
    ]
  }]
}
