import math

class Math(object):
  """
  Additional math routines for GeographicLib.

  This defines constants:
    epsilon, difference between 1 and the next bigger number
    minval, minimum positive number
    maxval, maximum finite number
    degree, the number of radians in a degree
    nan, not a number
    int, infinity
  """
  
  epsilon = math.pow(2.0, -52)
  minval = math.pow(2.0, -1022)
  maxval = math.pow(2.0, 1023) * (2 - epsilon)
  degree = math.pi/180
  nan = float("nan")
  inf = float("inf")
  def sq(x):
    """Square a number"""

    return x * x
  sq = staticmethod(sq)
  def cbrt(x):
    """Real cube root of a number"""

    y = math.pow(abs(x), 1/3.0)
    return y if x >= 0 else -y
  cbrt = staticmethod(cbrt)
  def isfinite(x):
    """Test for finiteness"""

    return abs(x) <= Math.maxval
  isfinite = staticmethod(isfinite)
