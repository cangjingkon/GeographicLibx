/**
 * \file Planimeter.cpp
 * \brief Command line utility for measuring the area of geodesic polygons
 *
 * Copyright (c) Charles Karney (2010, 2011) <charles@karney.com> and licensed
 * under the LGPL.  For more information, see
 * http://geographiclib.sourceforge.net/
 *
 * Compile with -I../include and link with Geodesic.o GeodesicLine.o DMS.o
 *
 * See the <a href="Planimeter.1.html">man page</a> for usage
 * information.
 **********************************************************************/

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/DMS.hpp>
#include <GeographicLib/GeoCoords.hpp>

#include "Planimeter.usage"

/// \cond SKIP
namespace GeographicLib {

  class GeodesicPolygon {
  private:
    typedef Math::real real;
    const Geodesic& _g;
    const real _area0;          // Full ellipsoid area
    const bool _polyline;       // Assume polyline (don't close and skip area)
    unsigned _num;
    int _crossings;
    Accumulator<real> _areasum, _perimetersum;
    real _lat0, _lon0, _lat1, _lon1;
    // Copied from Geodesic class
    static inline real AngNormalize(real x) throw() {
      // Place angle in [-180, 180).  Assumes x is in [-540, 540).
      //
      // g++ 4.4.4 holds a temporary in an extended register causing an error
      // with the triangle 89,0.1;89,90.1;89,-179.9.  The volatile declaration
      // fixes this.  (The bug probably triggered because transit and
      // AngNormalize are inline functions.  So don't port this change over to
      // Geodesic.hpp.)
      volatile real y = x;
      return y >= 180 ? y - 360 : y < -180 ? y + 360 : y;
    }
    static inline int transit(real lon1, real lon2) {
      // Return 1 or -1 if crossing prime meridian in east or west direction.
      // Otherwise return zero.
      lon1 = AngNormalize(lon1);
      lon2 = AngNormalize(lon2);
      // treat lon12 = -180 as an eastward geodesic, so convert to 180.
      real lon12 = -AngNormalize(lon1 - lon2); // In (-180, 180]
      int cross =
        lon1 < 0 && lon2 >= 0 && lon12 > 0 ? 1 :
        lon2 < 0 && lon1 >= 0 && lon12 < 0 ? -1 : 0;
      return cross;
    }
  public:
    GeodesicPolygon(const Geodesic& g, bool polyline) throw()
      : _g(g)
      , _area0(_g.EllipsoidArea())
      , _polyline(polyline)
    {
      Clear();
    }
    void Clear() throw() {
      _num = 0;
      _crossings = 0;
      _areasum = 0;
      _perimetersum = 0;
      _lat0 = _lon0 = _lat1 = _lon1 = 0;
    }
    void AddPoint(real lat, real lon) throw() {
      if (_num == 0) {
        _lat0 = _lat1 = lat;
        _lon0 = _lon1 = lon;
      } else {
        real s12, S12, t;
        _g.GenInverse(_lat1, _lon1, lat, lon,
                      Geodesic::DISTANCE |
                      (_polyline ? 0 : Geodesic::AREA),
                      s12, t, t, t, t, t, S12);
        _perimetersum += s12;
        if (!_polyline) {
          _areasum += S12;
          _crossings += transit(_lon1, lon);
        }
        _lat1 = lat;
        _lon1 = lon;
      }
      ++_num;
    }
    unsigned Compute(bool reverse, bool sign,
                     real& perimeter, real& area) const throw() {
      real s12, S12, t;
      if (_num < 2) {
        perimeter = area = 0;
        return _num;
      }
      if (_polyline) {
        perimeter = _perimetersum();
        area = 0;
        return _num;
      }
      _g.GenInverse(_lat1, _lon1, _lat0, _lon0,
                    Geodesic::DISTANCE | Geodesic::AREA,
                    s12, t, t, t, t, t, S12);
      perimeter = _perimetersum(s12);
      Accumulator<real> tempsum(_areasum);
      tempsum += S12;
      int crossings = _crossings + transit(_lon1, _lon0);
      if (crossings & 1)
        tempsum += (tempsum < 0 ? 1 : -1) * _area0/2;
      // area is with the clockwise sense.  If !reverse convert to
      // counter-clockwise convention.
      if (!reverse)
        tempsum *= -1;
      // If sign put area in (-area0/2, area0/2], else put area in [0, area0)
      if (sign) {
        if (tempsum > _area0/2)
          tempsum -= _area0;
        else if (tempsum <= -_area0/2)
          tempsum += _area0;
      } else {
        if (tempsum >= _area0)
          tempsum -= _area0;
        else if (tempsum < 0)
          tempsum += _area0;
      }
      area = tempsum();
      return _num;
    }
  };

}
/// \endcond

int main(int argc, char* argv[]) {
  try {
    using namespace GeographicLib;
    typedef Math::real real;
    real
      a = Constants::WGS84_a<real>(),
      f = Constants::WGS84_f<real>();
    bool reverse = false, sign = true, polyline = false;
    std::string istring, ifile, ofile;

    for (int m = 1; m < argc; ++m) {
      std::string arg(argv[m]);
      if (arg == "-r")
        reverse = !reverse;
      else if (arg == "-s")
        sign = !sign;
      else if (arg == "-l")
        polyline = !polyline;
      else if (arg == "-e") {
        if (m + 2 >= argc) return usage(1, true);
        try {
          a = DMS::Decode(std::string(argv[m + 1]));
          f = DMS::DecodeFraction(std::string(argv[m + 2]));
        }
        catch (const std::exception& e) {
          std::cerr << "Error decoding arguments of -e: " << e.what() << "\n";
          return 1;
        }
        m += 2;
      } else if (arg == "--input-string") {
        if (++m == argc) return usage(1, true);
        istring = argv[m];
      } else if (arg == "--input-file") {
        if (++m == argc) return usage(1, true);
        ifile = argv[m];
      } else if (arg == "--output-file") {
        if (++m == argc) return usage(1, true);
        ofile = argv[m];
      } else if (arg == "--version") {
        std::cout
          << argv[0]
          << ": $Id$\n"
          << "GeographicLib version " << GEOGRAPHICLIB_VERSION_STRING << "\n";
        return 0;
      } else
        return usage(!(arg == "-h" || arg == "--help"), arg != "--help");
    }

    if (!ifile.empty() && !istring.empty()) {
      std::cerr << "Cannot specify --input-string and --input-file together\n";
      return 1;
    }
    if (ifile == "-") ifile.clear();
    std::ifstream infile;
    std::istringstream instring;
    if (!ifile.empty()) {
      infile.open(ifile.c_str());
      if (!infile.is_open()) {
        std::cerr << "Cannot open " << ifile << " for reading\n";
        return 1;
      }
    } else if (!istring.empty()) {
      std::string::size_type m = 0;
      while (true) {
        m = istring.find(';', m);
        if (m == std::string::npos)
          break;
        istring[m] = '\n';
      }
      instring.str(istring);
    }
    std::istream* input = !ifile.empty() ? &infile :
      (!istring.empty() ? &instring : &std::cin);

    std::ofstream outfile;
    if (ofile == "-") ofile.clear();
    if (!ofile.empty()) {
      outfile.open(ofile.c_str());
      if (!outfile.is_open()) {
        std::cerr << "Cannot open " << ofile << " for wrting\n";
        return 1;
      }
    }
    std::ostream* output = !ofile.empty() ? &outfile : &std::cout;

    const Geodesic geod(a, f);
    GeodesicPolygon poly(geod, polyline);
    GeoCoords p;

    std::string s;
    real perimeter, area;
    unsigned num;
    while (std::getline(*input, s)) {
      bool endpoly = s.empty();
      if (!endpoly) {
        try {
          p.Reset(s);
          if (Math::isnan(p.Latitude()) || Math::isnan(p.Longitude()))
            endpoly = true;
        }
        catch (const GeographicErr&) {
          endpoly = true;
        }
      }
      if (endpoly) {
        num = poly.Compute(reverse, sign, perimeter, area);
        if (num > 0) {
          *output << num << " "
                  << DMS::Encode(perimeter, 8, DMS::NUMBER);
          if (!polyline)
            *output << " " << DMS::Encode(area, 3, DMS::NUMBER);
          *output << "\n";
        }
        poly.Clear();
      } else
        poly.AddPoint(p.Latitude(), p.Longitude());
    }
    num = poly.Compute(reverse, sign, perimeter, area);
    if (num > 0) {
      *output << num << " "
              << DMS::Encode(perimeter, 8, DMS::NUMBER);
      if (!polyline)
        *output << " " << DMS::Encode(area, 3, DMS::NUMBER);
      *output << "\n";
    }
    poly.Clear();
    return 0;
  }
  catch (const std::exception& e) {
    std::cerr << "Caught exception: " << e.what() << "\n";
    return 1;
  }
  catch (...) {
    std::cerr << "Caught unknown exception\n";
    return 1;
  }
}
