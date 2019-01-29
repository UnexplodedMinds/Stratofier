/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QtDebug>

#include <math.h>

#include "RoscoPiDefs.h"
#include "TrafficMath.h"
#include "StratuxStreams.h"


extern StratuxSituation g_situation;


// Find the distance and bearing from one lat/long to another
BearingDist TrafficMath::haversine( double dLat1, double dLong1, double dLat2, double dLong2 )
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


void TrafficMath::updateNearbyAirports( QList<Airport> *pAirports, double dDist )
{
// Not ideal but for local testing, good enough
#ifdef WIN32
    QFile faaDatabase( "D:/Source/RoscoPi/Airports.csv" );
#else
    QFile faaDatabase( "/home/pi/RoscoPi/Airports.csv" );
#endif

    if( !faaDatabase.open( QIODevice::ReadOnly ) )
        return;

    QString     qsLine;
    QStringList qsl;
    int         iCount;
    Airport     ap;
    int         iFound = 0;
    BearingDist bd;

    pAirports->clear();
    while( !faaDatabase.atEnd() )
    {
        qsLine = faaDatabase.readLine();
        qsLine = qsLine.trimmed();
        qsl = qsLine.split( ',' );
        iCount = qsl.count();
        iFound = 0;
        ap.runways.clear();
        if( iCount > 0 )
        {
            ap.qsID = qsl.first();
            iFound++;
        }
        if( iCount > 1 )
        {
            ap.qsName = qsl.at( 1 );
            iFound++;
        }
        if( iCount > 2 )
        {
            ap.bMilitary = (qsl.at( 2 ) == "MA");
            iFound++;
        }
        if( iCount > 3 )
        {
            ap.bPublic = (qsl.at( 3 ) == "PU");
            iFound++;
        }
        if( iCount > 4 )
        {
            QString qsLat = qsl.at( 4 );
            double  dPosNeg = (qsLat.right( 1 ) == "N" ? 1.0 : -1.0 );  // South is negative latitude (all US airports are North latitude)

            qsLat.chop( 1 );
            ap.dLat = qsLat.toDouble() / 3600.0 * dPosNeg;
            iFound++;
        }
        if( iCount > 5 )
        {
            QString qsLong = qsl.at( 5 );
            double  dPosNeg = (qsLong.right( 1 ) == "E" ? 1.0 : -1.0 );  // West is negative longitude (all US airports are West longitude)

            qsLong.chop( 1 );
            ap.dLong = qsLong.toDouble() / 3600.0 * dPosNeg;
            iFound++;
        }
        // Add all the available runways
        if( iCount > 9 )
        {
            for( int i = 9; i < iCount; i++ )
                ap.runways.append( qsl.at( i ).toInt() );
        }

        if( iFound == 6 )
        {
            bd = TrafficMath::haversine( g_situation.dGPSlat, g_situation.dGPSlong, ap.dLat, ap.dLong );
            if( bd.dDistance <= dDist )
            {
                ap.bd = bd;
                pAirports->append( ap );
            }
        }
    }
}

