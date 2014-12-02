{
  'includes': [ 'common.gypi' ],
  'targets': [
    {
      'target_name': 'stalker',
      'type': 'executable',
      'sources': [
        'src/stalker.c'
      ],
      'dependencies': [
        'deps/libuv/uv.gyp:libuv'
      ],
    }
  ]
}