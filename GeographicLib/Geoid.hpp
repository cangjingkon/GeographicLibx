/**
 * \file Geoid.hpp
 * \brief Header for GeographicLib::Geoid class
 *
 * Copyright (c) Charles Karney (2009) <charles@karney.com>
 * and licensed under the LGPL.  For more information, see
 * http://charles.karney.info/geographic/
 **********************************************************************/

#if !defined(GEOGRAPHICLIB_GEOID_HPP)
#define GEOGRAPHICLIB_GEOID_HPP "$Id$"

#include <vector>
#include <string>
#include <fstream>

namespace GeographicLib {

  class Geoid {
  private:
    std::string _filename;
    const double _a, _e2, _degree, _eps;
    mutable std::ifstream _file;
    double _rlonres, _rlatres;
    std::string _description;
    double _offset, _scale, _maxerror, _rmserror;
    unsigned _width, _height;
    unsigned _datastart;
    // Area cache
    mutable std::vector< std::vector<unsigned short> > _data;
    mutable bool _cache;
    // NE corner and extent of cache
    mutable unsigned _xoffset, _yoffset, _xsize, _ysize;
    // Cell cache
    mutable unsigned _ix, _iy;
    mutable double _v00, _v01, _v10, _v11;

    double rawval(unsigned ix, unsigned iy) const {
      if (ix >= _width)
	ix -= _width;
      if (_cache && iy >= _yoffset && iy < _yoffset + _ysize &&
	  ((ix >= _xoffset && ix < _xoffset + _xsize) ||
	   (ix + _width >= _xoffset && ix + _width < _xoffset + _xsize))) {
	return double(_data
		      [iy - _yoffset]
		      [ix >= _xoffset ?
		       ix - _xoffset :
		       ix + _width - _xoffset]);
      } else {
	_file.seekg(_datastart + 2 * (iy * _width + ix), std::ios::beg);
	char a, b;
	_file.get(a);
	_file.get(b);
	return double((unsigned char)(a) * 256u + (unsigned char)(b));
      }
    }
    double height(double lat, double lon, bool gradp,
	       double& grade, double& gradn) const;
  public:
    Geoid(const std::string& geoid, const std::string& path = "");
    void CacheArea(double south, double west, double north, double east) const;
    void CacheAll() const { CacheArea(-90.0, 0.0, 90.0, 360.0); }
    void CacheClear() const;
    double operator()(double lat, double lon) const {
      double gradn, grade;
      return height(lat, lon, false, gradn, grade);
    }
    double operator()(double lat, double lon, double& gradn, double& grade)
      const {
      return height(lat, lon, true, gradn, grade);
    }
    const std::string& Description() const { return _description; }
    const std::string& GeoidFile() const { return _filename; }
    double MaxError() const { return _maxerror; }
    double RMSError() const { return _rmserror; }
    static std::string DefaultPath();
    static std::string GeoidPath();
  };
} //namespace GeographicLib
#endif
