/**
 * \file geodesicdirect.cpp
 * \brief Matlab mex file for geographic to UTM/UPS conversions
 *
 * Copyright (c) Charles Karney (2010) <charles@karney.com> and licensed under
 * the LGPL.  For more information, see http://geographiclib.sourceforge.net/
 **********************************************************************/

// Compile in Matlab with
// [Unix]
// mex -I/usr/local/include -L/usr/local/lib -Wl,-rpath=/usr/local/lib -lGeographic geodesicdirect.cpp
// [Windows]
// mex -I../include -L../windows/Release -lGeographic geodesicdirect.cpp

// $Id$

#include <algorithm>
#include <GeographicLib/Geodesic.hpp>
#include <mex.h>

using namespace std;
using namespace GeographicLib;

void mexFunction( int nlhs, mxArray* plhs[],
                  int nrhs, const mxArray* prhs[] ) {

  if (nrhs < 1)
    mexErrMsgTxt("One input argument required.");
  else if (nrhs > 3)
    mexErrMsgTxt("More than three input arguments specified.");
  else if (nrhs == 2)
    mexErrMsgTxt("Must specify repicrocal flattening with the major radius.");
  else if (nlhs > 2)
    mexErrMsgTxt("More than two output arguments specified.");

  if (!( mxIsDouble(prhs[0]) && !mxIsComplex(prhs[0]) ))
    mexErrMsgTxt("geodesic coordinates are not of type double.");

  if (mxGetN(prhs[0]) != 4)
    mexErrMsgTxt("geodesic coordinates must be M x 4 matrix.");

  double a = Constants::WGS84_a(), r = Constants::WGS84_r();
  if (nrhs == 3) {
    if (!( mxIsDouble(prhs[1]) && !mxIsComplex(prhs[1]) &&
           mxGetNumberOfElements(prhs[1]) == 1 ))
      mexErrMsgTxt("major radius is not a real scalar.");
    a = mxGetScalar(prhs[1]);
    if (!( mxIsDouble(prhs[2]) && !mxIsComplex(prhs[2]) &&
           mxGetNumberOfElements(prhs[2]) == 1 ))
      mexErrMsgTxt("reciprocal flattening is not a real scalar.");
    r = mxGetScalar(prhs[2]);
  }

  int m = mxGetM(prhs[0]);

  double* lat1 = mxGetPr(prhs[0]);
  double* lon1 = lat1 + m;
  double* azi1 = lat1 + 2*m;
  double* s12 = lat1 + 3*m;

  plhs[0] = mxCreateDoubleMatrix(m, 3, mxREAL);
  double* lat2 = mxGetPr(plhs[0]);
  std::fill(lat2, lat2 + 3*m, Math::NaN());
  double* lon2 = lat2 + m;
  double* azi2 = lat2 + 2*m;
  double* m12 = NULL;
  double* M12 = NULL;
  double* M21 = NULL;
  double* S12 = NULL;
  bool aux = nlhs == 2;

  if (aux) {
    plhs[1] = mxCreateDoubleMatrix(m, 4, mxREAL);
    m12 = mxGetPr(plhs[1]);
    std::fill(m12, m12 + 4*m, Math::NaN());
    M12 = m12 + m;
    M21 = m12 + 2*m;
    S12 = m12 + 3*m;
  }

  try {
    const Geodesic g(a, r);
    for (int i = 0; i < m; ++i) {
      if (!(abs(lat1[i]) > 90) &&
          !(lon1[i] < -180 || lon1[i] > 360) &&
          !(azi1[i] < -180 || azi1[i] > 360)) {
        if (aux) 
          g.Direct(lat1[i], lon1[i], azi1[i], s12[i],
                   lat2[i], lon2[i], azi2[i], m12[i], M12[i], M21[i], S12[i]);
        else
          g.Direct(lat1[i], lon1[i], azi1[i], s12[i],
                   lat2[i], lon2[i], azi2[i]);
      }
    }
  }
  catch (const std::exception& e) {
    mexErrMsgTxt(e.what());
  }
}
