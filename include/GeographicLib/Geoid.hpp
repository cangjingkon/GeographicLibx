/**
 * \file Geoid.hpp
 * \brief Header for GeographicLib::Geoid class
 *
 * Copyright (c) Charles Karney (2009-2012) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * http://geographiclib.sourceforge.net/
 **********************************************************************/

#if !defined(GEOGRAPHICLIB_GEOID_HPP)
#define GEOGRAPHICLIB_GEOID_HPP 1

#include <string>
#include <vector>
#include <fstream>
#include <GeographicLib/Constants.hpp>

#if defined(_MSC_VER)
// Squelch warnings about dll vs vector
#pragma warning (push)
#pragma warning (disable: 4251)
#endif

#if !defined(PGM_PIXEL_WIDTH)
/**
 * The size of the pixel data in the pgm data files for the geoids.  2
 * is the standard size corresponding to a maxval 2^16-1.  Setting it
 * to 4 uses a maxval of 2^32-1 and changes the extension for the data
 * files from .pgm to .pgm4.  Note that the format of these pgm4 files
 * is a non-standard extension of the pgm format.
 **********************************************************************/
#define PGM_PIXEL_WIDTH 2
#endif

namespace GeographicLib {

  /**
   * \brief Looking up the height of the geoid
   *
   * This class evaluated the height of one of the standard geoids, EGM84,
   * EGM96, or EGM2008 by bilinear or cubic interpolation into a rectangular
   * grid of data.  These geoid models are documented in
   * - EGM84:
   *   http://earth-info.nga.mil/GandG/wgs84/gravitymod/wgs84_180/wgs84_180.html
   * - EGM96:
   *   http://earth-info.nga.mil/GandG/wgs84/gravitymod/egm96/egm96.html
   * - EGM2008:
   *   http://earth-info.nga.mil/GandG/wgs84/gravitymod/egm2008
   *
   * The geoids are defined in terms of spherical harmonics.  However in order
   * to provide a quick and flexible method of evaluating the geoid heights,
   * this class evaluates the height by interpolation into a grid of
   * precomputed values.
   *
   * See \ref geoid for details of how to install the data sets, the data
   * format, estimates of the interpolation errors, and how to use caching.
   *
   * In addition to returning the geoid height, the gradient of the geoid can
   * be calculated.  The gradient is defined as the rate of change of the geoid
   * as a function of position on the ellipsoid.  This uses the parameters for
   * the WGS84 ellipsoid.  The gradient defined in terms of the interpolated
   * heights.  As a result of the way that the geoid data is stored, the
   * calculation of gradients can result in large quantization errors.  This is
   * particularly acute for fine grids, at high latitudes, and for the easterly
   * gradient.
   *
   * This class is typically \e not thread safe in that a single instantiation
   * cannot be safely used by multiple threads because of the way the object
   * reads the data set and because it maintains a single-cell cache.  If
   * multiple threads need to calculate geoid heights they should all construct
   * thread-local instantiations.  Alternatively, set the optional \e
   * threadsafe parameter to true in the constructor.  This causes the
   * constructor to read all the data into memory and to turn off the
   * single-cell caching which results in a Geoid object which \e is thread
   * safe.
   *
   * Example of use:
   * \include example-Geoid.cpp
   *
   * <a href="GeoidEval.1.html">GeoidEval</a> is a command-line utility
   * providing access to the functionality of Geoid.
   **********************************************************************/

  class GEOGRAPHIC_EXPORT Geoid {
  private:
    typedef Math::real real;
#if PGM_PIXEL_WIDTH != 4
    typedef unsigned short pixel_t;
    static const unsigned pixel_size_ = 2;
    static const unsigned pixel_max_ = 0xffffu;
#else
    typedef unsigned pixel_t;
    static const unsigned pixel_size_ = 4;
    static const unsigned pixel_max_ = 0xffffffffu;
#endif
    static const unsigned stencilsize_ = 12;
    static const unsigned nterms_ = ((3 + 1) * (3 + 2))/2; // for a cubic fit
    static const real c0_;
    static const real c0n_;
    static const real c0s_;
    static const real c3_[stencilsize_ * nterms_];
    static const real c3n_[stencilsize_ * nterms_];
    static const real c3s_[stencilsize_ * nterms_];

    std::string _name, _dir, _filename;
    const bool _cubic;
    const real _a, _e2, _degree, _eps;
    mutable std::ifstream _file;
    real _rlonres, _rlatres;
    std::string _description, _datetime;
    real _offset, _scale, _maxerror, _rmserror;
    int _width, _height;
    unsigned long long _datastart, _swidth;
    bool _threadsafe;
    // Area cache
    mutable std::vector< std::vector<pixel_t> > _data;
    mutable bool _cache;
    // NE corner and extent of cache
    mutable int _xoffset, _yoffset, _xsize, _ysize;
    // Cell cache
    mutable int _ix, _iy;
    mutable real _v00, _v01, _v10, _v11;
    mutable real _t[nterms_];
    void filepos(int ix, int iy) const {
      _file.seekg(
#if !(defined(__GNUC__) && __GNUC__ < 4)
                  // g++ 3.x doesn't know about the cast to streamoff.
                  std::ios::streamoff
#endif
                  (_datastart +
                   pixel_size_ * (unsigned(iy)*_swidth + unsigned(ix))));
    }
    real rawval(int ix, int iy) const {
      if (ix < 0)
        ix += _width;
      else if (ix >= _width)
        ix -= _width;
      if (_cache && iy >= _yoffset && iy < _yoffset + _ysize &&
          ((ix >= _xoffset && ix < _xoffset + _xsize) ||
           (ix + _width >= _xoffset && ix + _width < _xoffset + _xsize))) {
        return real(_data[iy - _yoffset]
                    [ix >= _xoffset ? ix - _xoffset : ix + _width - _xoffset]);
      } else {
        if (iy < 0 || iy >= _height) {
          iy = iy < 0 ? -iy : 2 * (_height - 1) - iy;
          ix += (ix < _width/2 ? 1 : -1) * _width/2;
        }
        try {
          filepos(ix, iy);
          char a, b;
          _file.get(a);
          _file.get(b);
          unsigned r = ((unsigned char)(a) << 8) | (unsigned char)(b);
          if (pixel_size_ == 4) {
            _file.get(a);
            _file.get(b);
            r = (r << 16) | ((unsigned char)(a) << 8) | (unsigned char)(b);
          }
          return real(r);
        }
        catch (const std::exception& e) {
          // throw GeographicErr("Error reading " + _filename + ": "
          //                      + e.what());
          // triggers complaints about the "binary '+'" under Visual Studio.
          // So use '+=' instead.
          std::string err("Error reading ");
          err += _filename;
          err += ": ";
          err += e.what();
          throw GeographicErr(err);
        }
      }
    }
    real height(real lat, real lon, bool gradp,
                real& grade, real& gradn) const;
    Geoid(const Geoid&);            // copy constructor not allowed
    Geoid& operator=(const Geoid&); // copy assignment not allowed
  public:

    /**
     * Flags indicating conversions between heights above the geoid and heights
     * above the ellipsoid.
     **********************************************************************/
    enum convertflag {
      /**
       * The multiplier for converting from heights above the geoid to heights
       * above the ellipsoid.
       **********************************************************************/
      ELLIPSOIDTOGEOID = -1,
      /**
       * No conversion.
       **********************************************************************/
      NONE = 0,
      /**
       * The multiplier for converting from heights above the ellipsoid to
       * heights above the geoid.
       **********************************************************************/
      GEOIDTOELLIPSOID = 1,
    };

    /** \name Setting up the geoid
     **********************************************************************/
    ///@{
    /**
     * Construct a geoid.
     *
     * @param[in] name the name of the geoid.
     * @param[in] path (optional) directory for data file.
     * @param[in] cubic (optional) interpolation method; false means bilinear,
     *   true (the default) means cubic.
     * @param[in] threadsafe (optional), if true, construct a thread safe
     *   object.  The default is false
     * @exception GeographicErr if the data file cannot be found, is
     *   unreadable, or is corrupt.
     * @exception GeographicErr if \e threadsafe is true but the memory
     *   necessary for caching the data can't be allocated.
     *
     * The data file is formed by appending ".pgm" to the name.  If \e path is
     * specified (and is non-empty), then the file is loaded from directory, \e
     * path.  Otherwise the path is given by DefaultGeoidPath().  If the \e
     * threadsafe parameter is true, the data set is read into memory, the data
     * file is closed, and single-cell caching is turned off; this results in a
     * Geoid object which \e is thread safe.
     **********************************************************************/
    explicit Geoid(const std::string& name, const std::string& path = "",
                   bool cubic = true, bool threadsafe = false);

    /**
     * Set up a cache.
     *
     * @param[in] south latitude (degrees) of the south edge of the cached area.
     * @param[in] west longitude (degrees) of the west edge of the cached area.
     * @param[in] north latitude (degrees) of the north edge of the cached area.
     * @param[in] east longitude (degrees) of the east edge of the cached area.
     * @exception GeographicErr if the memory necessary for caching the data
     *   can't be allocated (in this case, you will have no cache and can try
     *   again with a smaller area).
     * @exception GeographicErr if there's a problem reading the data.
     * @exception GeographicErr if this is called on a threadsafe Geoid.
     *
     * Cache the data for the specified "rectangular" area bounded by the
     * parallels \e south and \e north and the meridians \e west and \e east.
     * \e east is always interpreted as being east of \e west, if necessary by
     * adding 360<sup>o</sup> to its value.  \e south and \e north should be in
     * the range [-90<sup>o</sup>, 90<sup>o</sup>]; \e west and \e east should
     * be in the range (-540<sup>o</sup>, 540<sup>o</sup>).
     **********************************************************************/
    void CacheArea(real south, real west, real north, real east) const;

    /**
     * Cache all the data.
     *
     * @exception GeographicErr if the memory necessary for caching the data
     *   can't be allocated (in this case, you will have no cache and can try
     *   again with a smaller area).
     * @exception GeographicErr if there's a problem reading the data.
     * @exception GeographicErr if this is called on a threadsafe Geoid.
     *
     * On most computers, this is fast for data sets with grid resolution of 5'
     * or coarser.  For a 1' grid, the required RAM is 450MB; a 2.5' grid needs
     * 72MB; and a 5' grid needs 18MB.
     **********************************************************************/
    void CacheAll() const { CacheArea(real(-90), real(0),
                                      real(90), real(360)); }

    /**
     * Clear the cache.  This never throws an error.  (This does nothing with a
     * thread safe Geoid.)
     **********************************************************************/
    void CacheClear() const throw();

    ///@}

    /** \name Compute geoid heights
     **********************************************************************/
    ///@{
    /**
     * Compute the geoid height at a point
     *
     * @param[in] lat latitude of the point (degrees).
     * @param[in] lon longitude of the point (degrees).
     * @exception GeographicErr if there's a problem reading the data; this
     *   never happens if (\e lat, \e lon) is within a successfully cached area.
     * @return geoid height (meters).
     *
     * The latitude should be in [-90<sup>o</sup>, 90<sup>o</sup>] and
     * longitude should be in (-540<sup>o</sup>, 540<sup>o</sup>).
     **********************************************************************/
    Math::real operator()(real lat, real lon) const {
      real gradn, grade;
      return height(lat, lon, false, gradn, grade);
    }

    /**
     * Compute the geoid height and gradient at a point
     *
     * @param[in] lat latitude of the point (degrees).
     * @param[in] lon longitude of the point (degrees).
     * @param[out] gradn northerly gradient (dimensionless).
     * @param[out] grade easterly gradient (dimensionless).
     * @exception GeographicErr if there's a problem reading the data; this
     *   never happens if (\e lat, \e lon) is within a successfully cached area.
     * @return geoid height (meters).
     *
     * The latitude should be in [-90<sup>o</sup>, 90<sup>o</sup>] and
     * longitude should be in (-540<sup>o</sup>, 540<sup>o</sup>).  As a result
     * of the way that the geoid data is stored, the calculation of gradients
     * can result in large quantization errors.  This is particularly acute for
     * fine grids, at high latitudes, and for the easterly gradient.  If you
     * need to compute the direction of the acceleration due to gravity
     * accurately, you should use GravityModel::Gravity.
     **********************************************************************/
    Math::real operator()(real lat, real lon, real& gradn, real& grade) const {
      return height(lat, lon, true, gradn, grade);
    }

    /**
     * Convert a height above the geoid to a height above the ellipsoid and
     * vice versa.
     *
     * @param[in] lat latitude of the point (degrees).
     * @param[in] lon longitude of the point (degrees).
     * @param[in] h height of the point (degrees).
     * @param[in] d a Geoid::convertflag specifying the direction of the
     *   conversion; Geoid::GEOIDTOELLIPSOID means convert a height above the
     *   geoid to a height above the ellipsoid; Geoid::ELLIPSOIDTOGEOID means
     *   convert a height above the ellipsoid to a height above the geoid.
     * @exception GeographicErr if there's a problem reading the data; this
     *   never happens if (\e lat, \e lon) is within a successfully cached area.
     * @return converted height (meters).
     **********************************************************************/
    Math::real ConvertHeight(real lat, real lon, real h,
                             convertflag d) const {
      real gradn, grade;
      return h + real(d) * height(lat, lon, true, gradn, grade);
    }

    ///@}

    /** \name Inspector functions
     **********************************************************************/
    ///@{
    /**
     * @return geoid description, if available, in the data file; if
     *   absent, return "NONE".
     **********************************************************************/
    const std::string& Description() const throw() { return _description; }

    /**
     * @return date of the data file; if absent, return "UNKNOWN".
     **********************************************************************/
    const std::string& DateTime() const throw() { return _datetime; }

    /**
     * @return full file name used to load the geoid data.
     **********************************************************************/
    const std::string& GeoidFile() const throw() { return _filename; }

    /**
     * @return "name" used to load the geoid data (from the first argument of
     *   the constructor).
     **********************************************************************/
    const std::string& GeoidName() const throw() { return _name; }

    /**
     * @return directory used to load the geoid data.
     **********************************************************************/
    const std::string& GeoidDirectory() const throw() { return _dir; }

    /**
     * @return interpolation method ("cubic" or "bilinear").
     **********************************************************************/
    const std::string Interpolation() const
    { return std::string(_cubic ? "cubic" : "bilinear"); }

    /**
     * @return estimate of the maximum interpolation and quantization error
     *   (meters).
     *
     * This relies on the value being stored in the data file.  If the value is
     * absent, return -1.
     **********************************************************************/
    Math::real MaxError() const throw() { return _maxerror; }

    /**
     * @return estimate of the RMS interpolation and quantization error
     *   (meters).
     *
     * This relies on the value being stored in the data file.  If the value is
     * absent, return -1.
     **********************************************************************/
    Math::real RMSError() const throw() { return _rmserror; }

    /**
     * @return offset (meters).
     *
     * This in used in converting from the pixel values in the data file to
     * geoid heights.
     **********************************************************************/
    Math::real Offset() const throw() { return _offset; }

    /**
     * @return scale (meters).
     *
     * This in used in converting from the pixel values in the data file to
     * geoid heights.
     **********************************************************************/
    Math::real Scale() const throw() { return _scale; }

    /**
     * @return true if the object is constructed to be thread safe.
     **********************************************************************/
    bool ThreadSafe() const throw() { return _threadsafe; }

    /**
     * @return true if a data cache is active.
     **********************************************************************/
    bool Cache() const throw() { return _cache; }

    /**
     * @return west edge of the cached area; the cache includes this edge.
     **********************************************************************/
    Math::real CacheWest() const throw() {
      return _cache ? ((_xoffset + (_xsize == _width ? 0 : _cubic)
                        + _width/2) % _width - _width/2) / _rlonres :
        0;
    }

    /**
     * @return east edge of the cached area; the cache excludes this edge.
     **********************************************************************/
    Math::real CacheEast() const throw() {
      return  _cache ?
        CacheWest() +
        (_xsize - (_xsize == _width ? 0 : 1 + 2 * _cubic)) / _rlonres :
        0;
    }

    /**
     * @return north edge of the cached area; the cache includes this edge.
     **********************************************************************/
    Math::real CacheNorth() const throw() {
      return _cache ? 90 - (_yoffset + _cubic) / _rlatres : 0;
    }

    /**
     * @return south edge of the cached area; the cache excludes this edge
     *   unless it's the south pole.
     **********************************************************************/
    Math::real CacheSouth() const throw() {
      return _cache ? 90 - ( _yoffset + _ysize - 1 - _cubic) / _rlatres : 0;
    }

    /**
     * @return \e a the equatorial radius of the WGS84 ellipsoid (meters).
     *
     * (The WGS84 value is returned because the supported geoid models are all
     * based on this ellipsoid.)
     **********************************************************************/
    Math::real MajorRadius() const throw()
    { return Constants::WGS84_a<real>(); }

    /**
     * @return \e f the flattening of the WGS84 ellipsoid.
     *
     * (The WGS84 value is returned because the supported geoid models are all
     * based on this ellipsoid.)
     **********************************************************************/
    Math::real Flattening() const throw() { return Constants::WGS84_f<real>(); }
    ///@}

    /// \cond SKIP
    /**
     * <b>DEPRECATED</b>
     * @return \e r the inverse flattening of the WGS84 ellipsoid.
     **********************************************************************/
    Math::real InverseFlattening() const throw()
    { return 1/Constants::WGS84_f<real>(); }
    /// \endcond

    /**
     * @return the default path for geoid data files.
     *
     * This is the value of the environment variable GEOID_PATH, if set;
     * otherwise, it is $GEOGRAPHICLIB_DATA/geoids if the environment variable
     * GEOGRAPHICLIB_DATA is set; otherwise, it is a compile-time default
     * (/usr/local/share/GeographicLib/geoids on non-Windows systems and
     * C:/Documents and Settings/All Users/Application
     * Data/GeographicLib/geoids on Windows systems).
     **********************************************************************/
    static std::string DefaultGeoidPath();

    /**
     * @return the default name for the geoid.
     *
     * This is the value of the environment variable GEOID_NAME, if set,
     * otherwise, it is "egm96-5".  The Geoid class does not use this function;
     * it is just provided as a convenience for a calling program when
     * constructing a Geoid object.
     **********************************************************************/
    static std::string DefaultGeoidName();

  };

} // namespace GeographicLib

#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#endif  // GEOGRAPHICLIB_GEOID_HPP
