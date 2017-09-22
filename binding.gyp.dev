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
    'defines': [
    ],
    'include_dirs': [
      'src/config/<(OS)/<(target_arch)',
      'src/spatialite/src/headers',
      'src'
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
      'target_name': 'spatialite',
      'type': 'shared_library',
      'direct_dependent_settings': {
        'include_dirs': [
          'src/config/<(OS)/<(target_arch)',
          'src/spatialite/src/headers'
        ],
        'defines': [
          'ENABLE_LWGEOM'
        ],
      },
      'dependencies': [
        'src/spatialite/deps/geos/binding.gyp:geos',
        'src/spatialite/deps/proj/binding.gyp:proj',
        'src/spatialite/deps/iconv/binding.gyp:iconv',
        'src/spatialite/deps/freexl/binding.gyp:freexl'
      ],
      'sources': [
        'src/spatialite/src/gaiaaux/gg_sqlaux.c',
        'src/spatialite/src/gaiaaux/gg_utf8.c',
        'src/spatialite/src/gaiaexif/gaia_exif.c',
        'src/spatialite/src/gaiageo/gg_advanced.c',
        'src/spatialite/src/gaiageo/gg_endian.c',
        'src/spatialite/src/gaiageo/gg_ewkt.c',
        'src/spatialite/src/gaiageo/gg_extras.c',
        'src/spatialite/src/gaiageo/gg_geodesic.c',
        'src/spatialite/src/gaiageo/gg_geoJSON.c',
        'src/spatialite/src/gaiageo/gg_geometries.c',
        'src/spatialite/src/gaiageo/gg_geoscvt.c',
        'src/spatialite/src/gaiageo/gg_gml.c',
        'src/spatialite/src/gaiageo/gg_kml.c',
        'src/spatialite/src/gaiageo/gg_lwgeom.c',
        'src/spatialite/src/gaiageo/gg_relations.c',
        'src/spatialite/src/gaiageo/gg_shape.c',
        'src/spatialite/src/gaiageo/gg_transform.c',
        'src/spatialite/src/gaiageo/gg_vanuatu.c',
        'src/spatialite/src/gaiageo/gg_voronoj.c',
        'src/spatialite/src/gaiageo/gg_wkb.c',
        'src/spatialite/src/gaiageo/gg_wkt.c',
        'src/spatialite/src/shapefiles/shapefiles.c',
        'src/spatialite/src/spatialite/mbrcache.c',
        'src/spatialite/src/spatialite/metatables.c',
        'src/spatialite/src/spatialite/spatialite.c',
        'src/spatialite/src/spatialite/statistics.c',
        'src/spatialite/src/spatialite/virtualdbf.c',
        'src/spatialite/src/spatialite/virtualfdo.c',
        'src/spatialite/src/spatialite/virtualnetwork.c',
        'src/spatialite/src/spatialite/virtualshape.c',
        'src/spatialite/src/spatialite/virtualspatialindex.c',
        'src/spatialite/src/spatialite/virtualXL.c',
        'src/spatialite/src/srsinit/epsg_inlined_00.c',
        'src/spatialite/src/srsinit/epsg_inlined_01.c',
        'src/spatialite/src/srsinit/epsg_inlined_02.c',
        'src/spatialite/src/srsinit/epsg_inlined_03.c',
        'src/spatialite/src/srsinit/epsg_inlined_04.c',
        'src/spatialite/src/srsinit/epsg_inlined_05.c',
        'src/spatialite/src/srsinit/epsg_inlined_06.c',
        'src/spatialite/src/srsinit/epsg_inlined_07.c',
        'src/spatialite/src/srsinit/epsg_inlined_08.c',
        'src/spatialite/src/srsinit/epsg_inlined_09.c',
        'src/spatialite/src/srsinit/epsg_inlined_10.c',
        'src/spatialite/src/srsinit/epsg_inlined_11.c',
        'src/spatialite/src/srsinit/epsg_inlined_12.c',
        'src/spatialite/src/srsinit/epsg_inlined_13.c',
        'src/spatialite/src/srsinit/epsg_inlined_14.c',
        'src/spatialite/src/srsinit/epsg_inlined_15.c',
        'src/spatialite/src/srsinit/epsg_inlined_16.c',
        'src/spatialite/src/srsinit/epsg_inlined_17.c',
        'src/spatialite/src/srsinit/epsg_inlined_18.c',
        'src/spatialite/src/srsinit/epsg_inlined_19.c',
        'src/spatialite/src/srsinit/epsg_inlined_20.c',
        'src/spatialite/src/srsinit/epsg_inlined_21.c',
        'src/spatialite/src/srsinit/epsg_inlined_22.c',
        'src/spatialite/src/srsinit/epsg_inlined_23.c',
        'src/spatialite/src/srsinit/epsg_inlined_24.c',
        'src/spatialite/src/srsinit/epsg_inlined_25.c',
        'src/spatialite/src/srsinit/epsg_inlined_26.c',
        'src/spatialite/src/srsinit/epsg_inlined_27.c',
        'src/spatialite/src/srsinit/epsg_inlined_28.c',
        'src/spatialite/src/srsinit/epsg_inlined_29.c',
        'src/spatialite/src/srsinit/epsg_inlined_30.c',
        'src/spatialite/src/srsinit/epsg_inlined_31.c',
        'src/spatialite/src/srsinit/epsg_inlined_32.c',
        'src/spatialite/src/srsinit/epsg_inlined_33.c',
        'src/spatialite/src/srsinit/epsg_inlined_34.c',
        'src/spatialite/src/srsinit/epsg_inlined_35.c',
        'src/spatialite/src/srsinit/epsg_inlined_36.c',
        'src/spatialite/src/srsinit/epsg_inlined_37.c',
        'src/spatialite/src/srsinit/epsg_inlined_38.c',
        'src/spatialite/src/srsinit/epsg_inlined_39.c',
        'src/spatialite/src/srsinit/epsg_inlined_40.c',
        'src/spatialite/src/srsinit/epsg_inlined_extra.c',
        'src/spatialite/src/srsinit/epsg_inlined_prussian.c',
        'src/spatialite/src/srsinit/epsg_inlined_wgs84_00.c',
        'src/spatialite/src/srsinit/epsg_inlined_wgs84_01.c',
        'src/spatialite/src/srsinit/srs_init.c',
        'src/spatialite/src/versioninfo/version.c',
        'src/spatialite/src/virtualtext/virtualtext.c'
       ]
    },
  ]
}
