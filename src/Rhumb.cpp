/**
 * \file Rhumb.cpp
 * \brief Implementation for GeographicLib::Rhumb and GeographicLib::RhumbLine
 * classes
 *
 * Copyright (c) Charles Karney (2012) <charles@karney.com> and licensed under
 * the MIT/X11 License.  For more information, see
 * http://geographiclib.sourceforge.net/
 **********************************************************************/

#include <algorithm>
#include <GeographicLib/Rhumb.hpp>

namespace GeographicLib {

  using namespace std;

  const Rhumb& Rhumb::WGS84() {
    static const Rhumb wgs84(Constants::WGS84_a(), Constants::WGS84_f(), false);
    return wgs84;
  }

  void Rhumb::Inverse(real lat1, real lon1, real lat2, real lon2,
                      real& s12, real& azi12) const {
    real
      lon12 = Math::AngDiff(Math::AngNormalize(lon1), Math::AngNormalize(lon2)),
      psi1 = _ell.IsometricLatitude(lat1),
      psi2 = _ell.IsometricLatitude(lat2),
      psi12 = psi2 - psi1,
      h = Math::hypot(lon12, psi12);
    azi12 = 0 - atan2(-lon12, psi12) / Math::degree();
    real dmudpsi = DIsometricToRectifying(psi2 * Math::degree(),
                                          psi1 * Math::degree());
    s12 = h * dmudpsi * _ell.QuarterMeridian() / 90;
  }

  RhumbLine Rhumb::Line(real lat1, real lon1, real azi12) const
  { return RhumbLine(*this, lat1, lon1, azi12, _exact); }

  void Rhumb::Direct(real lat1, real lon1, real azi12, real s12,
                     real& lat2, real& lon2) const
  { Line(lat1, lon1, azi12).Position(s12, lat2, lon2); }

  Math::real Rhumb::DE(real x, real y) const {
    const EllipticFunction& ei = _ell._ell;
    real d = x - y;
    if (x * y <= 0)
      return d ? (ei.E(x) - ei.E(y)) / d : 1;
    // See DLMF: Eqs (19.11.2) and (19.11.4) letting
    // theta -> x, phi -> -y, psi -> z
    //
    // (E(x) - E(y)) / d = E(z)/d - k2 * sin(x) * sin(y) * sin(z)/d
    //
    // tan(z/2) = (sin(x)*Delta(y) - sin(y)*Delta(x)) / (cos(x) + cos(y))
    //          = d * Dsin(x,y) * (sin(x) + sin(y))/(cos(x) + cos(y)) /
    //             (sin(x)*Delta(y) + sin(y)*Delta(x))
    //          = t = d * Dt
    // sin(z) = 2*t/(1+t^2); cos(z) = (1-t^2)/(1+t^2)
    // Alt (this only works for |z| <= pi/2 -- however, this conditions holds
    // if x*y > 0):
    // sin(z) = d * Dsin(x,y) * (sin(x) + sin(y))/
    //          (sin(x)*cos(y)*Delta(y) + sin(y)*cos(x)*Delta(x))
    // cos(z) = sqrt((1-sin(z))*(1+sin(z)))
    real sx = sin(x), sy = sin(y), cx = cos(x), cy = cos(y);
    real Dt = Dsin(x, y) * (sx + sy) /
      ((cx + cy) * (sx * ei.Delta(sy, cy) + sy * ei.Delta(sx, cx))),
      t = d * Dt, Dsz = 2 * Dt / (1 + t*t),
      sz = d * Dsz, cz = (1 - t) * (1 + t) / (1 + t*t);
    return ((sz ? ei.E(sz, cz, ei.Delta(sz, cz)) / sz : 1)
            - ei.k2() * sx * sy) * Dsz;
  }

  Math::real Rhumb::DRectifying(real latx, real laty) const {
    real
      phix = latx * Math::degree(), tbetx = _ell._f1 * tano(phix),
      phiy = laty * Math::degree(), tbety = _ell._f1 * tano(phiy);
    return (Math::pi()/2) * _ell._b * _ell._f1 * DE(atan(tbetx), atan(tbety))
      * Dtan(phix, phiy) * Datan(tbetx, tbety) / _ell.QuarterMeridian();
  }

  Math::real Rhumb::DIsometric(real latx, real laty) const {
    real
      phix = latx * Math::degree(), tx = tano(phix),
      phiy = laty * Math::degree(), ty = tano(phiy);
    return Dasinh(tx, ty) * Dtan(phix, phiy)
      - Deatanhe(sin(phix), sin(phiy)) * Dsin(phix, phiy);
  }

  Math::real Rhumb::SinSeries(real x, real y, const real c[], int n) {
    // N.B. n >= 0 and c[] has n+1 elements 0..n, of which c[0] is ignored.
    //
    // Use Clenshaw summation to evaluate
    //   m = (g(x) + g(y)) / 2         -- mean value
    //   s = (g(x) - g(y)) / (x - y)   -- average slope
    // where
    //   g(x) = sum(c[j]*sin(2*j*x), j = 1..n)
    //
    // This function returns only s and m is discarded.
    //
    // Write
    //   t = [m; s]
    //   t = sum(c[j] * f[j](x,y), j = 1..n)
    // where
    //   f[j](x,y) = [ (sin(2*j*x)+sin(2*j*y))/2 ]
    //               [ (sin(2*j*x)-sin(2*j*y))/d ]
    //
    //             = [       sin(j*p)*cos(j*d) ]
    //               [ (2/d)*sin(j*d)*cos(j*p) ]
    // and
    //    p = x+y, d = x-y
    //
    //   f[j+1](x,y) = A * f[j](x,y) - f[j-1](x,y)
    //
    //   A = [  2*cos(p)*cos(d)      -sin(p)*sin(d)*d]
    //       [ -4*sin(p)*sin(d)/d   2*cos(p)*cos(d)  ]
    //
    // Let b[n+1] = b[n+2] = [0 0; 0 0]
    //     b[j] = A * b[j+1] - b[j+2] + c[j] * I for j = n..1
    //    t =  b[1] * f[1](x,y)
    real p = x + y, d = x - y,
      cp = cos(p), cd =     cos(d),
      sp = sin(p), sd = d ? sin(d)/d : 1,
      m = 2 * cp * cd, s = sp * sd;
    // 2x2 matrices stored in row-major order
    const real a[4] = {m, -s * d * d, -4 * s, m};
    real ba[4] = {0, 0, 0, 0};
    real bb[4] = {0, 0, 0, 0};
    real* b0 = ba;
    real* b1 = bb;
    if (n > 0) b0[0] = b0[3] = c[n];
    for (int j = n - 1; j > 0; --j) { // j = n-1 .. 1
      std::swap(b0, b1);
      // b0 = A * b1 - b0 + c[j] * I
      b0[0] = a[0] * b1[0] + a[1] * b1[2] - b0[0] + c[j];
      b0[1] = a[0] * b1[1] + a[1] * b1[3] - b0[1];
      b0[2] = a[2] * b1[0] + a[3] * b1[2] - b0[2];
      b0[3] = a[2] * b1[1] + a[3] * b1[3] - b0[3] + c[j];
    }
    b1[0] = sp * cd; b1[2] = 2 * sd * cp;
    // Here is the (unused) expression for m
    // m = b0[0] * b1[0] + b0[1] * b1[2];
    s = b0[2] * b1[0] + b0[3] * b1[2];
    return s;
  }

  Math::real Rhumb::DConformalToRectifying(real chix, real chiy) const {
    return 1 + SinSeries(chix, chiy,
                         _ell.ConformalToRectifyingCoeffs(), tm_maxord);
  }

  Math::real Rhumb::DRectifyingToConformal(real mux, real muy) const {
    return 1 - SinSeries(mux, muy,
                         _ell.RectifyingToConformalCoeffs(), tm_maxord);
  }

  Math::real Rhumb::DIsometricToRectifying(real psix, real psiy) const {
    if (_exact) {
      real
        latx = _ell.InverseIsometricLatitude(psix/Math::degree()),
        laty = _ell.InverseIsometricLatitude(psiy/Math::degree());
      return DRectifying(latx, laty) / DIsometric(latx, laty);
    } else
      return DConformalToRectifying(gd(psix), gd(psiy)) * Dgd(psix, psiy);
  }

  Math::real Rhumb::DRectifyingToIsometric(real mux, real muy) const {
    real
      latx = _ell.InverseRectifyingLatitude(mux/Math::degree()),
      laty = _ell.InverseRectifyingLatitude(muy/Math::degree());
    return _exact ?
      DIsometric(latx, laty) / DRectifying(latx, laty) :
      Dgdinv(_ell.ConformalLatitude(latx) * Math::degree(),
             _ell.ConformalLatitude(laty) * Math::degree()) *
      DRectifyingToConformal(mux, muy);
  }

  RhumbLine::RhumbLine(const Rhumb& rh, real lat1, real lon1, real azi12,
                       bool exact)
    : _rh(rh)
    , _exact(exact)
    , _lat1(lat1)
    , _lon1(Math::AngNormalize(lon1))
    , _azi12(Math::AngNormalize(azi12))
  {
    real alp12 = azi12 * Math::degree();
    _salp =     azi12  == -180 ? 0 : sin(alp12);
    _calp = abs(azi12) ==   90 ? 0 : cos(alp12);
    _mu1 = _rh._ell.RectifyingLatitude(lat1);
    _psi1 = _rh._ell.IsometricLatitude(lat1);
    _r1 = _rh._ell.CircleRadius(lat1);
  }

  void RhumbLine::Position(real s12, real& lat2, real& lon2) const {
    real
      mu12 = s12 * _calp * 90 / _rh._ell.QuarterMeridian(),
      mu2 = _mu1 + mu12;
    if (abs(mu2) <= 90) {
      if (_calp) {
        lat2 = _rh._ell.InverseRectifyingLatitude(mu2);
        real psi12 = _rh.DRectifyingToIsometric( mu2 * Math::degree(),
                                                 _mu1 * Math::degree()) * mu12;
        lon2 = _salp * psi12 / _calp;
      } else {
        lat2 = _lat1;
        lon2 = _salp * s12 / (_r1 * Math::degree());
      }
      lon2 = Math::AngNormalize2(_lon1 + lon2);
    } else {
      // Reduce to the interval [-180, 180)
      mu2 = Math::AngNormalize2(mu2);
      // Deal with points on the anti-meridian
      if (abs(mu2) > 90) mu2 = Math::AngNormalize(180 - mu2);
      lat2 = _rh._ell.InverseRectifyingLatitude(mu2);
      lon2 = Math::NaN();
    }
  }

} // namespace GeographicLib
