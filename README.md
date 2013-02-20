# node-spatialite

Standalone, cross-platform SpatiaLite 4.0 binding for node.js with all features enabled. The goal of this project is to have a completely standalone build of SpatiaLite that doesn't require any system dependencies (no need to `apt-get install ...` or install any system libraries other than the ones required to build native node.js modules. The result is a consistent build across platforms with a guarantee of functionality and the versions of the dependencies. There are number of ways to get SpatiaLite binaries on the various platforms, but none of them include all of the features in a consistent way. This project contains a complete gyp build system for SpatiaLite - so someone might find it useful for other things outside of nodejs also.

If you're familiar with SQL, this is a great way to get easy access to the power of GEOS, Proj4, and SQL without needing a full PostGIS server. A fair amount of work was put into getting [liblwgeom](https://www.gaia-gis.it/fossil/libspatialite/wiki?name=liblwgeom-4.0) support compiled across OS X, Linux, and Windows so you can use the geometry validation functions `ST_MakeValid`, `ST_IsValid`, `ST_Split`, and more.

# Usage
This module simply exposes [node-sqlite3](https://github.com/developmentseed/node-sqlite3) with an additional method on the `Database` object to enable SpatiaLite. I opted to re-use this awesome library and dynamically load the extension so you can still use the `node-sqlite3` API without SpatiaLite if you like.

Here is a simple example that shows the usage of GEOS-enabled `Centroid` and LWGEOM-enabled `ST_MakeValid`.

```js
sql = require('spatialite');
db = new sql.Database(':memory:');
db.spatialite(function() {
  db.each("SELECT AsGeoJSON(ST_MakeValid(Centroid(GeomFromText('POLYGON ((30 10, 10 20, 20 40, 40 40, 30 10))'))));", function(err, row) {
    console.log(row);
  });
});
```

# Tested on
- OS X 64bit
- Linux 64bit (Ubuntu)
- Linux 32bit (Ubuntu)
- Windows 64bit
- Windows 32bit (pending some testing)

# Included in this build
- SpatiaLite 4.0.0 [SQL Reference](http://www.gaia-gis.it/gaia-sins/spatialite-sql-4.0.0.html)
- GEOS-3.4.0-dev - Compiled with GEOS_TRUNK flag so all of the latest triangulation functions are available:
  - `DelaunayTriangulation`
  - `VoronojDiagram`
  - `ConcaveHull`
- Proj 4.8.0
- iconv 1.14
- FreeXL 1.0.0e
- liblwgeom 2.0.2 (from PostGIS 2.0.2) available functions:
  - `MakeValid`
  - `MakeValidDiscarded`
  - `Segmentize`
  - `Split`
  - `SplitLeft`
  - `SplitRight`
  - `Azimuth`
  - `SnapToGrid`
  - `GeoHash`
  - `AsX3D`
  - `MaxDistance`
  - `3DDistance`
  - `3DMaxDistance`

# Building

There's a lot of code in these dependencies and the build scripts are fairly complex. If it doesn't work, submit an issue!

To build the shared library, you will need to first install `node-gyp`.

    $ npm install -g node-gyp

Build it:

    $ node-gyp configure build

# Notes:

A lot of this was inspired by @TooTallNate's post on embedding dependencies in node modules.

http://n8.io/converting-a-c-library-to-gyp/

# License

This module is BSD licensed. The dependencies have their own licenses which are available in their directories.
