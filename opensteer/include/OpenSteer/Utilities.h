// ----------------------------------------------------------------------------
//
//
// OpenSteer -- Steering Behaviors for Autonomous Characters
//
// Copyright (c) 2002-2003, Sony Computer Entertainment America
// Original author: Craig Reynolds <craig_reynolds@playstation.sony.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
//
// ----------------------------------------------------------------------------
//
//
// Utilities for OpenSteering
//
// 10-04-04 bk:  put everything into the OpenSteer namespace
// 07-09-02 cwr: created 
//
//
// ----------------------------------------------------------------------------

#ifndef OPENSTEER_UTILITIES_H
#define OPENSTEER_UTILITIES_H

#include <iostream>  // for ostream, <<, etc.
#include <cstdlib>   // for rand, etc.
#include <cfloat>    // for FLT_MAX, etc.
#include <cmath>     // for sqrt, etc.

// for memory leak detection during debugging
#include <stdlib.h>
#include <crtdbg.h>

// memory leak detection new operator replacment
/// ref: http://stackoverflow.com/questions/3202520/c-memory-leak-testing-with-crtdumpmemoryleaks-does-not-output-line-numb
#ifdef _DEBUG
#define DEBUG_NEW_PLACEMENT (_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define DEBUG_NEW_PLACEMENT
#endif

// ----------------------------------------------------------------------------
// For the sake of Windows, apparently this is a "Linux/Unix thing"

// API export
#ifdef OPENSTEER_EXPORT
#define OPENSTEER_API __declspec(dllexport)
#else 
#define OPENSTEER_API __declspec(dllimport)
#endif

#ifndef OPENSTEER_M_PI
#define OPENSTEER_M_PI 3.14159265358979323846
#endif


namespace OpenSteer {

    // ----------------------------------------------------------------------------
    // Generic interpolation


    template<class T> inline T interpolate (double alpha, const T& x0, const T& x1)
    {
        return x0 + ((x1 - x0) * alpha);
    }


    // ----------------------------------------------------------------------------
    // Random number utilities


    // Returns a double randomly distributed between 0 and 1

    inline double frandom01 (void)
    {
        return (((double) rand ()) / ((double) RAND_MAX));
    }


    // Returns a double randomly distributed between lowerBound and upperBound

    inline double frandom2 (double lowerBound, double upperBound)
    {
        return lowerBound + (frandom01 () * (upperBound - lowerBound));
    }


    // ----------------------------------------------------------------------------
    // Constrain a given value (x) to be between two (ordered) bounds: min
    // and max.  Returns x if it is between the bounds, otherwise returns
    // the nearer bound.


    inline double clip (const double x, const double min, const double max)
    {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }


    // ----------------------------------------------------------------------------
    // remap a value specified relative to a pair of bounding values
    // to the corresponding value relative to another pair of bounds.
    // Inspired by (dyna:remap-interval y y0 y1 z0 z1)


    inline double remapInterval (double x,
                                double in0, double in1,
                                double out0, double out1)
    {
        // uninterpolate: what is x relative to the interval in0:in1?
        double relative = (x - in0) / (in1 - in0);

        // now interpolate between output interval based on relative x
        return interpolate (relative, out0, out1);
    }


    // Like remapInterval but the result is clipped to remain between
    // out0 and out1


    inline double remapIntervalClip (double x,
                                    double in0, double in1,
                                    double out0, double out1)
    {
        // uninterpolate: what is x relative to the interval in0:in1?
        double relative = (x - in0) / (in1 - in0);

        // now interpolate between output interval based on relative x
        return interpolate (clip (relative, 0, 1), out0, out1);
    }


    // ----------------------------------------------------------------------------
    // classify a value relative to the interval between two bounds:
    //     returns -1 when below the lower bound
    //     returns  0 when between the bounds (inside the interval)
    //     returns +1 when above the upper bound


    inline int intervalComparison (double x, double lowerBound, double upperBound)
    {
        if (x < lowerBound) return -1;
        if (x > upperBound) return +1;
        return 0;
    }



    // ----------------------------------------------------------------------------


    inline double scalarRandomWalk (const double initial, 
                                   const double walkspeed,
                                   const double min,
                                   const double max)
    {
        const double next = initial + (((frandom01() * 2) - 1) * walkspeed);
        if (next < min) return min;
        if (next > max) return max;
        return next;
    }


    // ----------------------------------------------------------------------------


    inline double square (double x)
    {
        return x * x;
    }


    // ----------------------------------------------------------------------------
    // for debugging: prints one line with a given C expression, an equals sign,
    // and the value of the expression.  For example "angle = 35.6"


    #define debugPrint(e) (std::cout << #e" = " << (e) << std::endl << std::flush)


    // ----------------------------------------------------------------------------
    // blends new values into an accumulator to produce a smoothed time series
    //
    // Modifies its third argument, a reference to the double accumulator holding
    // the "smoothed time series."
    //
    // The first argument (smoothRate) is typically made proportional to "dt" the
    // simulation time step.  If smoothRate is 0 the accumulator will not change,
    // if smoothRate is 1 the accumulator will be set to the new value with no
    // smoothing.  Useful values are "near zero".
    //
    // Usage:
    //         blendIntoAccumulator (dt * 0.4, currentFPS, smoothedFPS);


    template<class T>
    inline void blendIntoAccumulator (const double smoothRate,
                                      const T& newValue,
                                      T& smoothedAccumulator)
    {
        smoothedAccumulator = interpolate (clip (smoothRate, 0, 1),
                                           smoothedAccumulator,
                                           newValue);
    }


    // ----------------------------------------------------------------------------
    // given a new Angle and an old angle, adjust the new for angle wraparound (the
    // 0->360 flip), returning a value equivalent to newAngle, but closest in
    // absolute value to oldAngle.  For radians fullCircle = OPENSTEER_M_PI*2, for degrees
    // fullCircle = 360.  Based on code in stuart/bird/fish demo's camera.cc
    //
    // (not currently used)

    /*
      inline double distance1D (const double a, const double b)
      {
          const double d = a - b;
          return (d > 0) ? d : -d;
      }


      double adjustForAngleWraparound (double newAngle,
                                      double oldAngle,
                                      double fullCircle)
      {
          // adjust newAngle for angle wraparound: consider its current value (a)
          // as well as the angle 2pi larger (b) and 2pi smaller (c).  Select the
          // one closer (magnitude of difference) to the current value of oldAngle.
          const double a = newAngle;
          const double b = newAngle + fullCircle;
          const double c = newAngle - fullCircle;
          const double ad = distance1D (a, oldAngle);
          const double bd = distance1D (b, oldAngle);
          const double cd = distance1D (c, oldAngle);

          if ((bd < ad) && (bd < cd)) return b;
          if ((cd < ad) && (cd < bd)) return c;
          return a;
      }
    */


    // ----------------------------------------------------------------------------
    // Functions to encapsulate cross-platform differences for several <cmath>
    // functions.  Specifically, the C++ standard says that these functions are
    // in the std namespace (std::sqrt, etc.)  Apparently the MS VC6 compiler (or
    // its header files) do not implement this correctly and the function names
    // are in the global namespace.  We hope these -XXX versions are a temporary
    // expedient, to be removed later.


    #ifdef _WIN32

    inline double floorXXX (double x)          {return floor (x);}
    inline double  sqrtXXX (double x)          {return sqrt (x);}
    inline double   sinXXX (double x)          {return sin (x);}
    inline double   cosXXX (double x)          {return cos (x);}
    inline double   absXXX (double x)          {return abs (x);}
    inline int     absXXX (int x)            {return abs (x);}
    inline double   maxXXX (double x, double y) {if (x > y) return x; else return y;}
    inline double   minXXX (double x, double y) {if (x < y) return x; else return y;}

    #else

    inline double floorXXX (double x)          {return std::floor (x);}
    inline double  sqrtXXX (double x)          {return std::sqrt (x);}
    inline double   sinXXX (double x)          {return std::sin (x);}
    inline double   cosXXX (double x)          {return std::cos (x);}
    inline double   absXXX (double x)          {return std::abs (x);}
    inline int     absXXX (int x)            {return std::abs (x);}
    inline double   maxXXX (double x, double y) {return std::max (x, y);}
    inline double   minXXX (double x, double y) {return std::min (x, y);}

    #endif


    // ----------------------------------------------------------------------------
    // round (x)  "round off" x to the nearest integer (as a double value)
    //
    // This is a Gnu-sanctioned(?) post-ANSI-Standard(?) extension (as in
    // http://www.opengroup.org/onlinepubs/007904975/basedefs/math.h.html)
    // which may not be present in all C++ environments.  It is defined in
    // math.h headers in Linux and Mac OS X, but apparently not in Win32:


    #ifdef _WIN32

    inline double round (double x)
    {
      if (x < 0)
          return -floorXXX (0.5 - x);
      else
          return  floorXXX (0.5 + x);
    }

    #else 
    
    inline double round( double x )
    {
        return ::round( x );
    }
    
    #endif

} // namespace OpenSteer
    
    
// ----------------------------------------------------------------------------
#endif // OPENSTEER_UTILITIES_H
