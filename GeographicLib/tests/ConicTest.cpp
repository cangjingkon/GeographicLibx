/**
 * \file ProjTest.cpp
 * \brief Command line utility for testing transverse Mercator projections
 *
 * Copyright (c) Charles Karney (2008, 2009, 2010) <charles@karney.com>
 * and licensed under the LGPL.  For more information, see
 * http://geographiclib.sourceforge.net/
 **********************************************************************/

#include "GeographicLib/LambertConformalConic.hpp"
#include "GeographicLib/PolarStereographic.hpp"
#include "GeographicLib/TransverseMercator.hpp"
#include "GeographicLib/TransverseMercatorExact.hpp"
#include "GeographicLib/Constants.hpp"
#include <string>
#include <limits>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <stdexcept>

GeographicLib::Math::real
dist(GeographicLib::Math::real a, GeographicLib::Math::real r,
     GeographicLib::Math::real lat0, GeographicLib::Math::real lon0,
     GeographicLib::Math::real lat1, GeographicLib::Math::real lon1) {
  using namespace GeographicLib;
  typedef Math::real real;
  real
    phi = lat0 * Constants::degree(),
    f = r != 0 ? 1/r : 0,
    e2 = f * (2 - f),
    sinphi = sin(phi),
    n = 1/sqrt(1 - e2 * sinphi * sinphi),
      // See Wikipedia article on latitude
    hlon = std::cos(phi) * n,
    hlat = (1 - e2) * n * n * n,
    dlon = lon1 - lon0;
  if (dlon >= 180) dlon -= 360;
  else if (dlon < -180) dlon += 360;
  return a * Constants::degree() *
    Math::hypot((lat1 - lat0) * hlat, dlon * hlon);
}

int usage(int retval) {
  ( retval ? std::cerr : std::cout ) <<
"ConicTest -l -s\n\
$Id$\n\
\n\
Checks conic projections\n";
  return retval;
}

int main(int argc, char* argv[]) {
  using namespace GeographicLib;
  using namespace std;
  typedef Math::real real;
  //  Math::extended degree = atan2(Math::extended(1), Math::extended(1))/45;
  bool lambert = true;
  bool albert = false;
  bool checkstdlats = false;
  for (int m = 1; m < argc; ++m) {
    std::string arg(argv[m]);
    if (arg == "-l") {
      lambert = true;
      albert = false;
    } else if (arg == "-s") {
      checkstdlats = true;
    } else
      return usage(arg != "-h");
  }

  real a = Constants::WGS84_a(), r = Constants::WGS84_r();
  try {
    while (true) {
      Math::extended lat1, lat2, lat0, k0;
      if (!(cin >> lat1 >> lat2 >> lat0 >> k0))
	break;
      const LambertConformalConic lam(a, r, real(lat1), real(lat2), real(1));
      Math::extended lat0a = lam.OriginLatitude(), k0a = lam.CentralScale();
      cout << lat1 << " " << lat2 << " " << lat0 << " " << lat0a << " " << lat0a - lat0 << " " << k0a - k0 << "\n";
      /*
      const LambertConformalConic lamb(a, r, real(sin(lat1*degree)), real(cos(lat1*degree)), real(sin(lat2*degree)), real(cos(lat2*degree)), real(1));
      lat0a = lamb.OriginLatitude(); k0a = lamb.CentralScale();
      cout << lat1 << " " << lat2 << " " << lat0 << " " << lat0a << " " << lat0a - lat0 << " " << k0a - k0 << "\n";
      */
    }
  }
  catch (const std::exception& e) {
    std::cout << "ERROR: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
