{
  'targets': [
    { 
      'target_name': 'platform-qt',
      'product_name': 'mbgl-platform-qt',
      'type': 'static_library',
      'standalone_static_library': 1,
      'hard_dependency': 1,

      'includes': [
        '../gyp/qt.gypi',
      ],

      'sources': [
        '../include/mbgl/platform/qt/qmapboxgl.hpp',
        '../platform/default/log_stderr.cpp',
        '../platform/default/string_stdlib.cpp',
        '../platform/default/thread.cpp',
        '../platform/qt/application_root.cpp',
        '../platform/qt/asset_root.cpp',
        '../platform/qt/async_task.cpp',
        '../platform/qt/async_task_impl.hpp',
        '../platform/qt/image.cpp',
        '../platform/qt/qmapboxgl.cpp',
        '../platform/qt/qmapboxgl.qrc',
        '../platform/qt/qmapboxgl_p.hpp',
        '../platform/qt/run_loop.cpp',
        '../platform/qt/run_loop_impl.hpp',
        '../platform/qt/timer.cpp',
        '../platform/qt/timer_impl.hpp',
      ],

      'variables': {
        'cflags_cc': [
          '<@(boost_cflags)',
          '<@(nunicode_cflags)',
          '<@(variant_cflags)',
          '<@(opengl_cflags)',
          '<@(qt_cflags)',
          '-Wno-error',
          '-fPIC',
        ],
        'ldflags': [
          '<@(nunicode_ldflags)',
          '<@(variant_ldflags)',
          '<@(opengl_ldflags)',
          '<@(qt_ldflags)',
        ],
        'libraries': [
          '<@(nunicode_static_libs)',
        ],
      },

      'include_dirs': [
        '../include',
        '../src',
      ],

      'conditions': [
        ['OS == "mac"', {
          'xcode_settings': {
            'OTHER_CPLUSPLUSFLAGS': [ '<@(cflags_cc)' ],
          }
        }, {
          'cflags_cc': [ '<@(cflags_cc)' ],
        }]
      ],

      'link_settings': {
        'conditions': [
          ['OS == "mac"', {
            'libraries': [ '<@(libraries)' ],
            'xcode_settings': { 'OTHER_LDFLAGS': [ '<@(ldflags)' ] }
          }, {
            'libraries': [ '<@(libraries)', '<@(ldflags)' ],
          }]
        ],
      },
    },
  ],
}
