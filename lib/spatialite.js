var sqlite3 = module.exports = exports = require('sqlite3');
var fs = require('fs');

sqlite3.Database.prototype.spatialite = function(callback) {
  var base_path    = __dirname + '/../build/Release/spatialite';
  var linux_path   = base_path + '.so';
  var osx_path     = base_path + '.dylib';
  var windows_path = base_path + '.dll';

  var extension_path = null;

  if (fs.existsSync(linux_path)) {
    extension_path = linux_path;
  } else if (fs.existsSync(osx_path)) {
    extension_path = osx_path;
  } else if (fs.existsSync(windows_path)) {
    extension_path = windows_path;
  } else {
    throw new Error("Unable to find spatialite extension.");
  }

  this.loadExtension(extension_path, callback);
};
