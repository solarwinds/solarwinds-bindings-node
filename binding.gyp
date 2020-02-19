{
'targets': [{
    'target_name': 'appoptics-bindings',
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
    # preprocessor only (in bindings.o for some reason)
    #'cflags': ['-E'],
    # suppress warnings
    #'cflags': ['-w'],
    'sources': [
        'src/bindings.cc'
    ],
    'conditions': [
        ['OS in "linux"', {
        # includes reference oboe/oboe.h, so
        'include_dirs': [
          '<!@(node --no-warnings -p "require(\'node-addon-api\').include")',
          '<(module_root_dir)/'
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
    'target_name': 'ao-metrics',
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
  }]
}
