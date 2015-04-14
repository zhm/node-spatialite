var sqlite3 = module.exports = exports = require('sqlite3');
var fs = require('fs');

sqlite3.Database.prototype.spatialite = function(callback) {
  var base_path    = __dirname + '/../build/Release/mod_spatialite';
  this.loadExtension(base_path, callback);
};
