/*
Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#ifndef TRAFFICMATH_H
#define TRAFFICMATH_H


class TrafficMath
{
public:
    struct BearingDist
    {
        double dBearing;
        double dDistance;
    };

    static BearingDist haversine( double dLat1, double dLong1, double dLat2, double dLong2 );
    static double      radiansRel( double dAng );
    static double      degHeading( double dAng );
};

#endif // TRAFFICMATH_H
