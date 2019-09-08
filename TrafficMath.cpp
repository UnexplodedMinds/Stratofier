/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QtDebug>
#include <QFile>
#include <QSettings>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>

#include <math.h>

#include "StratofierDefs.h"
#include "TrafficMath.h"
#include "StratuxStreams.h"
#include "Builder.h"


extern StratuxSituation g_situation;
extern QSettings       *g_pSet;


// This was implemented to cut down on the airport lookup by lat/long that takes long enough to be noticeable on the display (it's threaded but you can see it filling back in)
QList<Airport> g_airportCache;


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


// Get every airport in the cache that's within twice the distance of the current heading indicator radius
void TrafficMath::updateNearbyAirports( QList<Airport> *pAirports, double dDist )
{
    Airport ap;

    pAirports->clear();
    dDist *= 2;
    foreach( ap, g_airportCache )
    {
        ap.bd = TrafficMath::haversine( g_situation.dGPSlat, g_situation.dGPSlong, ap.dLat, ap.dLong );
        ap.bd.dBearing += 90.0;

        if( ap.bd.dDistance <= dDist )
            pAirports->append( ap );
    }
}


void TrafficMath::updateAirport( Airport *pAirport )
{
    pAirport->bd = TrafficMath::haversine( g_situation.dGPSlat, g_situation.dGPSlong, pAirport->dLat, pAirport->dLong );
    pAirport->bd.dBearing += 90.0;
}


void TrafficMath::cacheAirports()
{
    bool    bExternalStorage = g_pSet->value( "ExternalStorage", false ).toBool();
    int     iDataSet = g_pSet->value( "CurrDataSet", 0 ).toInt();
    QString qsInternal, qsExternal;
    QFile   aipDatabase;

    Builder::getStorage( &qsInternal, &qsExternal );

    if( iDataSet == 0 )
    {
        qsInternal.append( "/data/space.skyfun.stratofier/airports_us.aip" );
        qsExternal.append( "/data/space.skyfun.stratofier/airports_us.aip" );
    }
    else
    {
        qsInternal.append( "/data/space.skyfun.stratofier/airports_ca.aip" );
        qsExternal.append( "/data/space.skyfun.stratofier/airports_ca.aip" );
    }

    if( bExternalStorage )
        aipDatabase.setFileName( qsExternal );
    else
        aipDatabase.setFileName( qsInternal );

    if( !aipDatabase.open( QIODevice::ReadOnly ) )
        return;

    QDomDocument aipDoc( "XML_OpenAIP" );
    QDomElement  xRoot, xAirportElem, xChildElem, xSubChildElem, xWayPtElem;
    QDomNode     xAirportNode, xChildNode, xSubChildNode, xWayPtNode;
    QString      qsTemp, qsErrMsg;
    Airport      ap;
    int          iErrLine, iErrCol;

    if( !aipDoc.setContent( &aipDatabase, &qsErrMsg, &iErrLine, &iErrCol ) )
    {
        qDebug() << qsErrMsg << iErrLine << iErrCol;
        aipDatabase.close();
        return;
    }
    aipDatabase.close();

    xRoot = aipDoc.documentElement();

    // Don't proceed if this isn't an OpenAIP file
    if( xRoot.tagName() != "OPENAIP" )
        return;

    // Total test points in test results file
    xWayPtNode = xRoot.firstChild();

    g_airportCache.clear();
    while( !xWayPtNode.isNull() )
    {
        xWayPtElem = xWayPtNode.toElement();
        if( xWayPtElem.tagName() == "WAYPOINTS" )
        {
            xAirportNode = xWayPtNode.firstChild();
            while( !xAirportNode.isNull() )
            {
                xAirportElem = xAirportNode.toElement();
                if( (xAirportElem.tagName() == "AIRPORT") && (xAirportElem.attribute( "TYPE" ) != "HELI_CIVIL") )
                {
                    ap.bd.dBearing = 0.0;
                    ap.bd.dDistance = 0.0;
                    ap.dLat = 0.0;
                    ap.dLong = 0.0;
                    ap.dElev = 0.0;
                    ap.bGrass = false;
                    ap.qsID.clear();
                    ap.runways.clear();

                    xChildNode = xAirportNode.firstChild();
                    while( !xChildNode.isNull() )
                    {
                        xChildElem = xChildNode.toElement();
                        if( xChildElem.tagName() == "ICAO" )
                            ap.qsID = xChildElem.text();
                        else if( xChildElem.tagName() == "NAME" )
                        {
                            ap.qsName = xChildElem.text();
                            // Clean up really long names
                            ap.qsName.replace( "REGIONAL", "RGNL" );
                            ap.qsName.replace( "EXECUTIVE", "EXEC" );
                            ap.qsName.replace( "INTERNATIONAL", "INTL" );
                            ap.qsName.replace( "AERODROME", "AERO" );
                            ap.qsName.replace( "BALLOONPORT", "BALLOON" );
                            ap.qsName.remove( "AIRPORT" );
                            ap.qsName.remove( "FIELD" );
                        }
                        else if( xChildElem.tagName() == "GEOLOCATION" )
                        {
                            xSubChildNode = xChildNode.firstChild();
                            while( !xSubChildNode.isNull() )
                            {
                                xSubChildElem = xSubChildNode.toElement();
                                if( xSubChildElem.tagName() == "LAT" )
                                    ap.dLat = xSubChildElem.text().toDouble();
                                else if( xSubChildElem.tagName() == "LON" )
                                    ap.dLong = xSubChildElem.text().toDouble();
                                else if( xSubChildElem.tagName() == "ELEV" )
                                {
                                    if( xSubChildElem.attribute( "UNIT" ).contains( "M", Qt::CaseInsensitive ) )
                                        ap.dElev = xSubChildElem.text().toDouble() * MetersToFeet;
                                    else
                                        ap.dElev = xSubChildElem.text().toDouble();
                                }
                                xSubChildNode = xSubChildNode.nextSibling();
                            }
                        }
                        else if( xChildElem.tagName() == "RWY" )
                        {
                            xSubChildNode = xChildNode.firstChild();
                            while( !xSubChildNode.isNull() )
                            {
                                xSubChildElem = xSubChildNode.toElement();
                                if( xSubChildElem.tagName() == "SFC" )
                                {
                                    if( xSubChildElem.text() == "GRAS" )
                                        ap.bGrass = true;
                                }
                                else if( xSubChildElem.tagName() == "DIRECTION" )
                                    ap.runways.append( xSubChildElem.attribute( "TC" ).toInt() );
                                xSubChildNode = xSubChildNode.nextSibling();
                            }
                        }
                        xChildNode = xChildNode.nextSibling();
                    }
                    if( ap.qsID.isEmpty() )
                        ap.qsID = "R";
                    g_airportCache.append( ap );    // Note the cache has no haversine transforms to get the bearing and distance since that's handled by updateNearbyAirports
                }
                xAirportNode = xAirportNode.nextSibling();
            }
        }
        xWayPtNode = xWayPtNode.nextSibling();
    }
}
