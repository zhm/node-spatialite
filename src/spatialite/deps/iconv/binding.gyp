{
  'variables': { 'target_arch%': 'ia32' },

  'target_defaults': {
    'default_configuration': 'Debug',
    'configurations': {
      'Debug': {
        'defines': [ 'DEBUG', '_DEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 1, # static debug
          },
        },
      },
      'Release': {
        'defines': [ 'NDEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 0, # static release
          },
        },
      }
    },
    'msvs_settings': {
      'VCCLCompilerTool': {
      },
      'VCLibrarianTool': {
      },
      'VCLinkerTool': {
        'GenerateDebugInformation': 'true',
      },
    },
    'include_dirs': [
      'config/<(OS)/<(target_arch)',
      '.',
      'iconv/include',
      'iconv/lib',
      'iconv/srclib'
    ],
    'conditions': [
      ['OS == "win"', {
        'defines': [
          'WIN32'
        ],
      }]
    ],
  },

  'targets': [
    {
      'target_name': 'iconv',
      'type': 'static_library',
      'sources': [
        # 'iconv/extras/iconv_string.c',
        'iconv/lib/iconv.c',
        # 'iconv/lib/iconv_relocatable.c',
        'iconv/libcharset/lib/localcharset.c',
        # 'iconv/libcharset/lib/localcharset_relocatable.c',
        # 'iconv/srclib/allocator.c',
        # 'iconv/srclib/areadlink.c',
        # 'iconv/srclib/c-ctype.c',
        # 'iconv/srclib/canonicalize-lgpl.c',
        # 'iconv/srclib/careadlinkat.c',
        # 'iconv/srclib/error.c',
        # 'iconv/srclib/lstat.c',
        # 'iconv/srclib/malloca.c',
        # 'iconv/srclib/memmove.c',
        # 'iconv/srclib/progname.c',
        # 'iconv/srclib/progreloc.c',
        # 'iconv/srclib/read.c',
        # 'iconv/srclib/readlink.c',
        # 'iconv/srclib/relocatable.c',
        # 'iconv/srclib/safe-read.c',
        # 'iconv/srclib/setenv.c',
        # 'iconv/srclib/stat.c',
        # 'iconv/srclib/stdio-write.c',
        # 'iconv/srclib/strerror.c',
        # 'iconv/srclib/xmalloc.c',
        # 'iconv/srclib/xreadlink.c',
        # 'iconv/srclib/xstrdup.c'
      ],
      'defines': [
        'BUILDING_LIBCHARSET',
        'LIBDIR="."',
        'INSTALLDIR="."',
        'NO_XMALLOC',
        'HAVE_CONFIG_H',
        'EXEEXT=""',
        'LIBPATHVAR="."'
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'config/<(OS)/<(target_arch)',
          '.',
          'iconv/include'
        ],
      },
    },
  ]
}

