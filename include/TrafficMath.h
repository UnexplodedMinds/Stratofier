/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
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

    static void cacheAirports();
    static void cacheAirspaces();
    static void updateNearbyAirports( QList<Airport> *pAirports, Airport *pDirect, Airport *pFrom, Airport *pTo, double dDist );
    static void updateNearbyAirspaces( QList<Airspace> *pAirspaces, double dDist );
    static int  findAirport( Airport *pAirport, QList<Airport> *apList );
};

#endif // TRAFFICMATH_H
