var sqlite3 = module.exports = exports = require('sqlite3');
var fs = require('fs');
var path = require('path');
var os = require('os');

sqlite3.Database.prototype.spatialite = function(callback) {
  var extension = {
    darwin: '.dylib',
    linux: '.so',
    win32: '.dll'
  }[os.platform()] || '.so';

  var distPath = path.join(__dirname,
                           '..',
                           'dist',
                           os.platform(),
                           os.arch(),
                           'mod_spatialite' + extension);

  var compiledPath = path.join(__dirname,
                               '..',
                               'build',
                               'Release',
                               'spatialite' + extension);

  if (fs.existsSync(distPath)) {
    this.loadExtension(distPath, callback);
  } else if (fs.existsSync(compiledPath)) {
    this.loadExtension(compiledPath, callback);
  } else {
    throw new Error('Unable to find spatialite extension.\nPaths:\n ' + distPath + '\n ' + compiledPath);
  }
};
