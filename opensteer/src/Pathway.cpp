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
// Pathway and PolylinePathway, for path following.
//
// 10-04-04 bk:  put everything into the OpenSteer namespace
// 06-03-02 cwr: created
//
//
// ----------------------------------------------------------------------------


#include "Pathway.h"


// ----------------------------------------------------------------------------
// construct a PolylinePathway given the number of points (vertices),
// an array of points, and a path radius.


OpenSteer::PolylinePathway::PolylinePathway (const int _pointCount,
                                  const Vec3 _points[],
                                  const double _radius,
                                  const bool _cyclic)
{
    initialize (_pointCount, _points, _radius, _cyclic);
}


// ----------------------------------------------------------------------------
// utility for constructors


void
OpenSteer::PolylinePathway::initialize (const int _pointCount,
                                        const Vec3 _points[],
                                        const double _radius,
                                        const bool _cyclic)
{
    // set data members, allocate arrays
    radius = _radius;
    cyclic = _cyclic;
    pointCount = _pointCount;
    totalPathLength = 0;
	if (isInit) Destruct();
	isInit = true;

    if (cyclic) pointCount++;
    lengths = new DEBUG_NEW_PLACEMENT double [pointCount];
    points  = new DEBUG_NEW_PLACEMENT Vec3 [pointCount];
    normals = new DEBUG_NEW_PLACEMENT Vec3 [pointCount];

    // loop over all points
    for (int i = 0; i < pointCount; i++)
    {
        // copy in point locations, closing cycle when appropriate
        const bool closeCycle = cyclic && (i == pointCount-1);
        const int j = closeCycle ? 0 : i;
        points[i] = _points[j];

        // for the end of each segment
        if (i > 0)
        {
            // compute the segment length
            normals[i] = points[i] - points[i-1];
            lengths[i] = normals[i].length ();

            // find the normalized vector parallel to the segment
            normals[i] *= 1 / lengths[i];

            // keep running total of segment lengths
            totalPathLength += lengths[i];
        }
    }
}

// Kaveh096: destructor to get rid of memory created in initialize
 void OpenSteer::PolylinePathway::Destruct(void)
{
	if (isInit)
	{
		delete [] lengths;
		delete [] points;
		delete [] normals;
		isInit = false;
	}
}

// ----------------------------------------------------------------------------
// Given an arbitrary point ("A"), returns the nearest point ("P") on
// this path.  Also returns, via output arguments, the path tangent at
// P and a measure of how far A is outside the Pathway's "tube".  Note
// that a negative distance indicates A is inside the Pathway.


OpenSteer::Vec3
OpenSteer::PolylinePathway::mapPointToPath (const Vec3& point,
                                            Vec3& tangent,
                                            double& outside)
{
    double d;
    double minDistance = FLT_MAX;
    Vec3 onPath;

    // loop over all segments, find the one nearest to the given point
    for (int i = 1; i < pointCount; i++)
    {
        segmentLength = lengths[i];
        segmentNormal = normals[i];
        d = pointToSegmentDistance (point, points[i-1], points[i]);
        if (d < minDistance)
        {
            minDistance = d;
            onPath = chosen;
            tangent = segmentNormal;
        }
    }

    // measure how far original point is outside the Pathway's "tube"
    outside = Vec3::distance (onPath, point) - radius;

    // return point on path
    return onPath;
}


// ----------------------------------------------------------------------------
// given an arbitrary point, convert it to a distance along the path


double
OpenSteer::PolylinePathway::mapPointToPathDistance (const Vec3& point)
{
    double d;
    double minDistance = FLT_MAX;
    double segmentLengthTotal = 0;
    double pathDistance = 0;

    for (int i = 1; i < pointCount; i++)
    {
        segmentLength = lengths[i];
        segmentNormal = normals[i];
        d = pointToSegmentDistance (point, points[i-1], points[i]);
        if (d < minDistance)
        {
            minDistance = d;
            pathDistance = segmentLengthTotal + segmentProjection;
        }
        segmentLengthTotal += segmentLength;
    }

    // return distance along path of onPath point
    return pathDistance;
}


// ----------------------------------------------------------------------------
// given a distance along the path, convert it to a point on the path


OpenSteer::Vec3
OpenSteer::PolylinePathway::mapPathDistanceToPoint (double pathDistance)
{
    // clip or wrap given path distance according to cyclic flag
    double remaining = pathDistance;
    if (cyclic)
    {
        remaining = (double) fmod (pathDistance, totalPathLength);
    }
    else
    {
        if (pathDistance < 0) return points[0];
        if (pathDistance >= totalPathLength) return points [pointCount-1];
    }

    // step through segments, subtracting off segment lengths until
    // locating the segment that contains the original pathDistance.
    // Interpolate along that segment to find 3d point value to return.
    Vec3 result;
    for (int i = 1; i < pointCount; i++)
    {
        segmentLength = lengths[i];
        if (segmentLength < remaining)
        {
            remaining -= segmentLength;
        }
        else
        {
            double ratio = remaining / segmentLength;
            result = interpolate (ratio, points[i-1], points[i]);
            break;
        }
    }
    return result;
}


// ----------------------------------------------------------------------------
// computes distance from a point to a line segment
//
// (I considered moving this to the vector library, but its too
// tangled up with the internal state of the PolylinePathway instance)


double
OpenSteer::PolylinePathway::pointToSegmentDistance (const Vec3& point,
                                                    const Vec3& ep0,
                                                    const Vec3& ep1)
{
    // convert the test point to be "local" to ep0
    local = point - ep0;

    // find the projection of "local" onto "segmentNormal"
    segmentProjection = segmentNormal.dot (local);

    // handle boundary cases: when projection is not on segment, the
    // nearest point is one of the endpoints of the segment
    if (segmentProjection < 0)
    {
        chosen = ep0;
        segmentProjection = 0;
        return Vec3::distance (point, ep0);
    }
    if (segmentProjection > segmentLength)
    {
        chosen = ep1;
        segmentProjection = segmentLength;
        return Vec3::distance (point, ep1);
    }

    // otherwise nearest point is projection point on segment
    chosen = segmentNormal * segmentProjection;
    chosen +=  ep0;
    return Vec3::distance (point, chosen);
}


// ----------------------------------------------------------------------------
