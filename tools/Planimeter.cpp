/**
 * \file Planimeter.cpp
 * \brief Command line utility for measuring the area of geodesic polygons
 *
 * Copyright (c) Charles Karney (2010-2012) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * http://geographiclib.sourceforge.net/
 *
 * Compile and link with
 *   g++ -g -O3 -I../include -I../man -o Planimeter \
 *       Planimeter.cpp \
 *       ../src/DMS.cpp \
 *       ../src/GeoCoords.cpp \
 *       ../src/Geodesic.cpp \
 *       ../src/GeodesicLine.cpp \
 *       ../src/MGRS.cpp \
 *       ../src/PolarStereographic.cpp \
 *       ../src/PolygonArea.cpp \
 *       ../src/TransverseMercator.cpp \
 *       ../src/UTMUPS.cpp
 *
 * See the <a href="Planimeter.1.html">man page</a> for usage
 * information.
 **********************************************************************/

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <GeographicLib/PolygonArea.hpp>
#include <GeographicLib/DMS.hpp>
#include <GeographicLib/Utility.hpp>
#include <GeographicLib/GeoCoords.hpp>
#include <GeographicLib/Ellipsoid.hpp>

#if defined(_MSC_VER)
// Squelch warnings about constant conditional expressions
#  pragma warning (disable: 4127)
#endif

#include "Planimeter.usage"

int main(int argc, char* argv[]) {
  try {
    using namespace GeographicLib;
    typedef Math::real real;
    real
      a = Math::NaN(),
      f = Math::NaN();
    bool reverse = false, sign = true, polyline = false,
      exact = false, authalic = false;
    int prec = 6;
    std::string istring, ifile, ofile, cdelim;
    char lsep = ';';
    Math::set_digits10(19);

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
          a = Utility::num<real>(std::string(argv[m + 1]));
          f = Utility::fract<real>(std::string(argv[m + 2]));
        }
        catch (const std::exception& e) {
          std::cerr << "Error decoding arguments of -e: " << e.what() << "\n";
          return 1;
        }
        m += 2;
      } else if (arg == "-p") {
        if (++m == argc) return usage(1, true);
        try {
          prec = Utility::num<int>(std::string(argv[m]));
        }
        catch (const std::exception&) {
          std::cerr << "Precision " << argv[m] << " is not a number\n";
          return 1;
        }
        Math::set_digits10(std::max(19, prec + 12));
      } else if (arg == "-E") {
        exact = true;
        authalic = false;
      } else if (arg == "-Q") {
        exact = false;
        authalic = true;
      } else if (arg == "--input-string") {
        if (++m == argc) return usage(1, true);
        istring = argv[m];
      } else if (arg == "--input-file") {
        if (++m == argc) return usage(1, true);
        ifile = argv[m];
      } else if (arg == "--output-file") {
        if (++m == argc) return usage(1, true);
        ofile = argv[m];
      } else if (arg == "--line-separator") {
        if (++m == argc) return usage(1, true);
        if (std::string(argv[m]).size() != 1) {
          std::cerr << "Line separator must be a single character\n";
          return 1;
        }
        lsep = argv[m][0];
      } else if (arg == "--comment-delimiter") {
        if (++m == argc) return usage(1, true);
        cdelim = argv[m];
      } else if (arg == "--version") {
        std::cout
          << argv[0] << ": GeographicLib version "
          << GEOGRAPHICLIB_VERSION_STRING << "\n";
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
        m = istring.find(lsep, m);
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
        std::cerr << "Cannot open " << ofile << " for writing\n";
        return 1;
      }
    }
    std::ostream* output = !ofile.empty() ? &outfile : &std::cout;

    if (Math::isnan(a)) a = Constants::WGS84_a();
    if (Math::isnan(f)) f = Constants::WGS84_f();
    const Ellipsoid ellip(a, f);
    if (authalic) {
      using std::sqrt;
      a = sqrt(ellip.Area() / (4 * Math::pi()));
      f = 0;
    }
    const Geodesic geod(a, f);
    const GeodesicExact geode(a, f);
    PolygonArea poly(geod, polyline);
    PolygonAreaExact polye(geode, polyline);
    GeoCoords p;

    // Max precision = 10: 0.1 nm in distance, 10^-15 deg (= 0.11 nm),
    // 10^-11 sec (= 0.3 nm).
    prec = std::min(10 + Math::extra_digits(), std::max(0, prec));
    std::string s;
    real perimeter, area;
    unsigned num;
    std::string eol("\n");
    while (std::getline(*input, s)) {
      if (!cdelim.empty()) {
        std::string::size_type m = s.find(cdelim);
        if (m != std::string::npos) {
          eol = " " + s.substr(m) + "\n";
          s = s.substr(0, m);
        }
      }
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
        num = exact ? polye.Compute(reverse, sign, perimeter, area) :
          poly.Compute(reverse, sign, perimeter, area);
        if (num > 0) {
          *output << num << " " << Utility::str(perimeter, prec);
          if (!polyline) {
            *output << " " << Utility::str(area, std::max(0, prec - 5));
          }
          *output << eol;
        }
        exact ? polye.Clear() : poly.Clear();
        eol = "\n";
      } else {
        exact ? polye.AddPoint(p.Latitude(), p.Longitude()) :
          poly.AddPoint(authalic ? ellip.AuthalicLatitude(p.Latitude()) :
                        p.Latitude(),
                        p.Longitude());
      }
    }
    num = exact ? polye.Compute(reverse, sign, perimeter, area):
      poly.Compute(reverse, sign, perimeter, area);
    if (num > 0) {
      *output << num << " " << Utility::str(perimeter, prec);
      if (!polyline) {
        *output << " " << Utility::str(area, std::max(0, prec - 5));
      }
      *output << eol;
    }
    exact ? polye.Clear() : poly.Clear();
    eol = "\n";
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
