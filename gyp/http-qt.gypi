{
  'targets': [
    {
      'target_name': 'http-qt',
      'product_name': 'mbgl-http-qt',
      'type': 'static_library',
      'standalone_static_library': 1,
      'hard_dependency': 1,

      'includes': [
        '../gyp/qt.gypi',
      ],

      'sources': [
        '../platform/qt/http_context_qt.cpp',
        '../platform/qt/http_context_qt.hpp',
        '../platform/qt/http_request_qt.cpp',
        '../platform/qt/http_request_qt.hpp',
      ],

      'variables': {
        'cflags_cc': [
          '<@(qt_cflags)',
          '<@(boost_cflags)',
          '-Wno-error',
          '-fPIC',
        ],
        'defines': [
          '-DMBGL_HTTP_QT'
        ],
        'ldflags': [
          '<@(qt_ldflags)',
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
            'xcode_settings': { 'OTHER_LDFLAGS': [ '<@(ldflags)' ] }
          }, {
            'libraries': [ '<@(ldflags)' ],
          }]
        ],
      },

      'direct_dependent_settings': {
        'conditions': [
          ['OS == "mac"', {
            'xcode_settings': {
              'OTHER_CFLAGS': [ '<@(defines)' ],
              'OTHER_CPLUSPLUSFLAGS': [ '<@(defines)' ],
            }
          }, {
            'cflags': [ '<@(defines)' ],
            'cflags_cc': [ '<@(defines)' ],
          }]
        ],
      },
    },
  ],
}
