/*
Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <math.h>

#include "TrafficMath.h"

#define ToRad      0.017453292519943296
#define TwoPi      6.283185307179586477
#define ToDeg      57.29577951308232088
#define MetersToNM 0.000539957


// Find the distance and bearing from one lat/long to another
TrafficMath::BearingDist TrafficMath::haversine( double dLat1, double dLong1, double dLat2, double dLong2 )
{
    BearingDist ret;

    double dRadiusEarth = 6371008.8;
    double deltaLat = radiansRel( dLat2 - dLat1 );
    double dAvgLat = radiansRel( (dLat2 + dLat1) / 2.0 );
    double deltaLong = radiansRel( dLong2 - dLong1 );
    double dDistN = deltaLat * dRadiusEarth;
    double dDistE = deltaLong * dRadiusEarth * fabs( cos( dAvgLat ) );

    ret.dDistance = pow( dDistN * dDistN + dDistE * dDistE, 0.5 ) * MetersToNM;
    ret.dBearing  = degHeading( atan2( dDistE, dDistN ) );

    return ret;
}


// Normalize angle and convert to radians
double TrafficMath::radiansRel( double dAng )
{
    while( dAng > 180 )
        dAng -= 360;
    while( dAng < -180 )
        dAng += 360;

    return dAng * ToRad;
}


// Normalize heading angle and convert to degrees
double TrafficMath::degHeading( double dAng )
{
    while( dAng < 0 )
        dAng += TwoPi;

    return dAng * ToDeg;
}

