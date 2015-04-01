{
  'targets': [
    {
      'target_name': 'traceview-bindings',
      'include_dirs': [
        "<!(node -e \"require('nan')\")"
      ],
      'sources': [
        'src/bindings.cc'
      ],
      'conditions': [
        ['OS=="linux"', {
          'libraries': [
            '-loboe'
          ],
          'ldflags': [
            '-Wl,-rpath /usr/local/lib'
          ]
        }]
      ]
    }
  ]
}
