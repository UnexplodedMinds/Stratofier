/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#ifndef TRAFFICMATH_H
#define TRAFFICMATH_H

#include <QList>

#include "Canvas.h"


class TrafficMath
{
public:
    static BearingDist haversine( double dLat1, double dLong1, double dLat2, double dLong2 );
    static double      radiansRel( double dAng );
    static double      degHeading( double dAng );

    static void updateNearbyAirports( QList<Airport> *pAirports, double dDist );
};

#endif // TRAFFICMATH_H
