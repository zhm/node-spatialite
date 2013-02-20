# This script is for installing dependencies in Linux for testing

sudo apt-get update &&
sudo apt-get install -y python-software-properties python g++ make clang build-essential &&
# sudo add-apt-repository ppa:ubuntugis/ubuntugis-unstable -y &&
# sudo apt-get update &&
# sudo apt-get install -y postgresql-9.1 postgresql-server-dev-9.1 postgresql-plpython-9.1 postgresql-contrib &&
# sudo apt-get install -y libxml2-dev libgeos-dev libproj-dev libgdal-dev libfreexl-dev sqlite3 postgis &&
sudo add-apt-repository -y ppa:chris-lea/node.js &&
sudo apt-get install -y nodejs npm &&
sudo npm install -g node-gyp

# cd ~

# wget http://www.gaia-gis.it/gaia-sins/freexl-1.0.0e.tar.gz && tar -xvf freexl-1.0.0e.tar.gz
# wget http://download.osgeo.org/geos/geos-3.3.7.tar.bz2 && tar -xvf geos-3.3.7.tar.bz2
# wget http://download.osgeo.org/postgis/source/postgis-2.0.2.tar.gz && tar -xvf postgis-2.0.2.tar.gz
# wget http://download.osgeo.org/proj/proj-4.8.0.tar.gz && tar -xvf proj-4.8.0.tar.gz
# wget http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.14.tar.gz && tar -xvf libiconv-1.14.tar.gz
# wget http://www.gaia-gis.it/gaia-sins/libspatialite-sources/libspatialite-4.0.0.tar.gz && tar -xvf libspatialite-4.0.0.tar.gz
