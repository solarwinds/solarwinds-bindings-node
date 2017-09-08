{
  'targets': [
    {
      'target_name': 'appoptics-bindings',
      'include_dirs': [
        "<!(node -e \"require('nan')\")"
      ],
      'sources': [
        'src/bindings.cc'
      ],
      'conditions': [
        ['OS in "linux mac"', {
          # includes reference oboe/oboe.h, so
          'include_dirs': [
            '<(module_root_dir)/'
          ],
          'libraries': [
            '-loboe',
            '-L<(module_root_dir)/lib/',
            '-Wl,-rpath-link,<(module_root_dir)/lib/',
            '-Wl,-rpath,<(module_root_dir)/lib/'
          ],
        }]
      ]
    }
  ]
}