{
'targets': [
    {
    'target_name': 'appoptics-bindings',
    'include_dirs': [
        "<!(node -e \"require('nan')\")"
    ],
    # preprocessor only (in bindings.o for some reason)
    #'cflags': ['-E'],
    # suppress warnings
    #'cflags': ['-w'],
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
            '-L<(module_root_dir)/oboe/',
            '-Wl,-rpath-link,<(module_root_dir)/oboe/',
            '-Wl,-rpath,\$$ORIGIN/../../oboe/'
        ],
        }]
    ]
    }
]
}
