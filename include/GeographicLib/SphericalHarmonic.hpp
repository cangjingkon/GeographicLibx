/**
 * \file SphericalHarmonic.hpp
 * \brief Header for GeographicLib::SphericalHarmonic class
 *
 * Copyright (c) Charles Karney (2011) <charles@karney.com> and licensed under
 * the MIT/X11 License.  For more information, see
 * http://geographiclib.sourceforge.net/
 **********************************************************************/

#if !defined(GEOGRAPHICLIB_SPHERICALHARMONIC_HPP)
#define GEOGRAPHICLIB_SPHERICALHARMONIC_HPP "$Id$"

#include <vector>
#include <GeographicLib/Constants.hpp>
#include <GeographicLib/SphericalEngine.hpp>
#include <GeographicLib/CircularEngine.hpp>

namespace GeographicLib {

  /**
   * \brief Spherical Harmonic series
   *
   * This class evaluates the spherical harmonic sum \verbatim
 V(x, y, z) = sum(n = 0..N)[ q^(n+1) * sum(m = 0..n)[
   (C[n,m] * cos(m*lambda) + S[n,m] * sin(m*lambda)) *
   P[n,m](cos(theta)) ] ]
\endverbatim
   * where
   * - <i>p</i><sup>2</sup> = <i>x</i><sup>2</sup> + <i>y</i><sup>2</sup>,
   * - <i>r</i><sup>2</sup> = <i>p</i><sup>2</sup> + <i>z</i><sup>2</sup>,
   * - \e q = <i>a</i>/<i>r</i>,
   * - \e theta = atan2(\e p, \e z) = the spherical \e colatitude,
   * - \e lambda = atan2(\e y, \e x) = the longitude.
   * - P<sub>\e nm</sub>(\e t) is the associated Legendre function of degree \e
   *   n and order \e m.
   *
   * One of two normalizations are supported for P<sub>\e nm</sub>
   * - fully normalized denoted by SphericalHarmonic::full; see Heiskanen and
   *   Moritz, Sec. 1-14 for its definition.
   * - Schmidt normalized denoted by SphericalHarmonic::schmidt.
   *
   * References:
   * - C. W. Clenshaw, A note on the summation of Chebyshev series,
   *   %Math. Tables Aids Comput. 9(51), 118-120 (1955).
   * - R. E. Deakin, Derivatives of the earth's potentials, Geomatics
   *   Research Australasia 68, 31-60, (June 1998).
   * - W. A. Heiskanen and H. Moritz, Physical Geodesy, (Freeman, San
   *   Fransisco, 1967).  (See Sec. 1-14, for a definition of Pbar.)
   * - S. A. Holmes and W. E. Featherstone, A unified approach to the
   *   Clenshaw summation and the recursive computation of very high degree
   *   and order normalised associated Legendre functions, J. Geod. 76(5),
   *   279-299 (2002).
   * - C. C. Tscherning and K. Poder, Some geodetic applications of Clenshaw
   *   summation, Boll. Geod. Sci. Aff. 41(4), 349-375 (1982).
   **********************************************************************/

  class GEOGRAPHIC_EXPORT SphericalHarmonic {
  public:
    enum normalization {
      full = SphericalEngine::full,
      schmidt = SphericalEngine::schmidt,
    };

  private:
    typedef Math::real real;
    SphericalEngine::coeff _c[1];
    real _a;
    normalization _norm;

  public:
    /**
     * A default constructor so that the object can be created when the
     * constructor for another object is initialized.  This default object can
     * then be reset with the default copy assignment operator.
     **********************************************************************/
    SphericalHarmonic() {}

    /**
     * Constructor with a full set of coefficients specified.
     *
     * @param[in] C the coefficients \e C<sub>\e nm</sub>.
     * @param[in] S the coefficients \e S<sub>\e nm</sub>.
     * @param[in] N the maximum degree and order of the sum
     * @param[in] a the reference radius appearing in the definition of the
     *   sum.
     * @param[in] norm the normalization for the associated Legrendre
     *   functions, either SphericalHarmonic::full (the default) or
     *   SphericalHarmonic::schmidt.
     *
     * The coefficients \e C<sub>\e nm</sub> and \e S<sub>\e nm</sub> are
     * stored in the one-dimensional vectors \e C and \e S which must contain
     * (\e N + 1)(\e N + 2)/2 elements, stored in "column-major" order.  Thus
     * for \e N = 3, the order would be:
     * <i>C</i><sub>00</sub>,
     * <i>C</i><sub>10</sub>,
     * <i>C</i><sub>20</sub>,
     * <i>C</i><sub>30</sub>,
     * <i>C</i><sub>11</sub>,
     * <i>C</i><sub>21</sub>,
     * <i>C</i><sub>31</sub>,
     * <i>C</i><sub>22</sub>,
     * <i>C</i><sub>32</sub>,
     * <i>C</i><sub>33</sub>.
     * In general the (\e n,\e m) element is at index \e m*\e N - \e m*(\e m -
     * 1)/2 + \e n.  The first (\e N + 1) elements of \e S are not referenced
     * (they should be 0).
     *
     * The class stores <i>pointers</i> to the first elements of \e C and \e S.
     * These arrays should not be altered or destroyed during the lifetime of a
     * SphericalHarmonic object.
     **********************************************************************/
    SphericalHarmonic(const std::vector<double>& C,
                      const std::vector<double>& S,
                      int N, real a, normalization norm = full)
      : _a(a)
      , _norm(norm)
    { _c[0] = SphericalEngine::coeff(C, S, N); }

    /**
     * Constructor with a subset of coefficients specified.
     *
     * @param[in] C the coefficients \e C<sub>\e nm</sub>.
     * @param[in] S the coefficients \e S<sub>\e nm</sub>.
     * @param[in] N the degree used to determine the layout of \e C and \e S.
     * @param[in] nmx the maximum degree used in the sum.  The sum over \e n is
     *   from 0 thru \e nmx.
     * @param[in] mmx the maximum order used in the sum.  The sum over \e m is
     *   from 0 thru min(\e n, \e mmx).
     * @param[in] a the reference radius appearing in the definition of the
     *   sum.
     * @param[in] norm the normalization for the associated Legrendre
     *   functions, either SphericalHarmonic::full (the default) or
     *   SphericalHarmonic::schmidt.
     *
     * The class stores <i>pointers</i> to the first elements of \e C and \e S.
     * These arrays should not be altered or destroyed during the lifetime of a
     * SphericalHarmonic object.
     **********************************************************************/
    SphericalHarmonic(const std::vector<double>& C,
                      const std::vector<double>& S,
                      int N, int nmx, int mmx,
                      real a, normalization norm = full)
      : _a(a)
      , _norm(norm)
    { _c[0] = SphericalEngine::coeff(C, S, N, nmx, mmx); }

    /**
     * Compute the spherical harmonic sum.
     *
     * @param[in] x cartesian coordinate.
     * @param[in] y cartesian coordinate.
     * @param[in] z cartesian coordinate.
     * @return \e V the spherical harmonic sum.
     **********************************************************************/
    Math::real operator()(real x, real y, real z)  const {
      real f[] = {1};
      real v = 0;
      real dummy;
      switch (_norm) {
      case full:
        v = SphericalEngine::Value<false, SphericalEngine::full, 1>
          (_c, f, x, y, z, _a, dummy, dummy, dummy);
        break;
      case schmidt:
        v = SphericalEngine::Value<false, SphericalEngine::schmidt, 1>
          (_c, f, x, y, z, _a, dummy, dummy, dummy);
        break;
      }
      return v;
    }

    /**
     * Compute a spherical harmonic sum and its gradient.
     *
     * @param[in] x cartesian coordinate.
     * @param[in] y cartesian coordinate.
     * @param[in] z cartesian coordinate.
     * @param[out] gradx \e x component of the gradient
     * @param[out] grady \e y component of the gradient
     * @param[out] gradz \e z component of the gradient
     * @return \e V the spherical harmonic sum.
     *
     * This is the same as the previous function, except that the components of
     * the gradients of the sum in the \e x, \e y, and \e z directions are
     * computed.
     **********************************************************************/
    Math::real operator()(real x, real y, real z,
                          real& gradx, real& grady, real& gradz) const {
      real f[] = {1};
      real v = 0;
      switch (_norm) {
      case full:
        v = SphericalEngine::Value<true, SphericalEngine::full, 1>
          (_c, f, x, y, z, _a, gradx, grady, gradz);
        break;
      case schmidt:
        v = SphericalEngine::Value<true, SphericalEngine::schmidt, 1>
          (_c, f, x, y, z, _a, gradx, grady, gradz);
        break;
      }
      return v;
    }

    /**
     * Compute CircularEngine to allow the efficient evaluation of several
     * points on a circle of latitude.
     *
     * @param[in] p the radius of the circle.
     * @param[in] z the height of the circle above the equatorial plane.
     * @param[in] gradp if true the returned object will be able to compute the
     *   gradient of the sum.
     * @return the CircularEngine object.
     *
     * SphericalHarmonic::operator() exchanges the order of the sums in the
     * definition, i.e., sum(n = 0..N)[sum(m = 0..n)[...]] becomes sum(m =
     * 0..N)[sum(n = m..N)[...]].  SphericalHarmonic::Circle performs the inner
     * sum over degree \e n (which entails about \e N<sup>2</sup> operations).
     * This leaves the returned CircularEngine object with the outer sum over
     * the order \e m to do (about \e N operations).
     *
     * Here's an example of computing the spherical sum at a sequence of
     * longitudes without using a CircularEngine object
     \code
  SphericalHarmonic h(...);     // Create the SphericalHarmonic object
  double r = 2, lat = 33, lon0 = 44, dlon = 0.01;
  double
    phi = lat * Math::degree<double>(),
    z = r * sin(phi), p = r * cos(phi);
  for (int i = 0; i <= 100; ++i) {
    real
      lon = lon0 + i * dlon,
      lam = lon * Math::degree<double>();
    std::cout << lon << " " << h(p * cos(lam), p * sin(lam), z) << "\n";
  }
     \endcode
     * Here is the same calculation done using a CircularEngine object.  This
     * will be about \e N/2 times faster.
     \code
  SphericalHarmonic h(...);     // Create the SphericalHarmonic object
  double r = 2, lat = 33, lon0 = 44, dlon = 0.01;
  double
    phi = lat * Math::degree<double>(),
    z = r * sin(phi), p = r * cos(phi);
  CircularEngine c(h(p, z, false)); // Create the CircularEngine object
  for (int i = 0; i <= 100; ++i) {
    real
      lon = lon0 + i * dlon;
    std::cout << lon << " " << c(lon) << "\n";
  }
     \endcode
     **********************************************************************/
    CircularEngine Circle(real p, real z, bool gradp) const {
      real f[] = {1};
      switch (_norm) {
      case full:
        return gradp ?
          SphericalEngine::Circle<true, SphericalEngine::full, 1>
          (_c, f, p, z, _a) :
          SphericalEngine::Circle<false, SphericalEngine::full, 1>
          (_c, f, p, z, _a);
        break;
      case schmidt:
      default:                  // To avoid compiler warnings
        return gradp ?
          SphericalEngine::Circle<true, SphericalEngine::schmidt, 1>
          (_c, f, p, z, _a) :
          SphericalEngine::Circle<false, SphericalEngine::schmidt, 1>
          (_c, f, p, z, _a);
        break;
      }
    }
  };

} // namespace GeographicLib

#endif  // GEOGRAPHICLIB_SPHERICALHARMONIC_HPP
