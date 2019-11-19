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
QList<Airport>  g_airportCache;
QList<Airspace> g_airspaceCache;


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

    while( ret.dBearing > 180 )
        ret.dBearing -= 360;
    while( ret.dBearing < -180 )
        ret.dBearing += 360;

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

        if( ap.bd.dDistance <= dDist )
            pAirports->append( ap );
    }
}


void TrafficMath::updateNearbyAirspaces( QList<Airspace> *pAirspaces, double dDist )
{
    Airspace    as;
    BearingDist bd;
    QPointF     pt;

    pAirspaces->clear();
    dDist *= 4.0;

    // Build a list of points that are vectors
    foreach( as, g_airspaceCache )
    {
        pt = as.shape.boundingRect().center();
        bd = TrafficMath::haversine( g_situation.dGPSlat, g_situation.dGPSlong, pt.y(), pt.x() );

        as.shapeHav.clear();
        if( bd.dDistance <= dDist )
        {
            foreach( pt, as.shape )
            {
                bd = TrafficMath::haversine( g_situation.dGPSlat, g_situation.dGPSlong, pt.y(), pt.x() );
                as.shapeHav.append( bd );
            }
            pAirspaces->append( as );
        }
    }
}


void TrafficMath::updateAirport( Airport *pAirport )
{
    pAirport->bd = TrafficMath::haversine( g_situation.dGPSlat, g_situation.dGPSlong, pAirport->dLat, pAirport->dLong );
}


void TrafficMath::cacheAirports()
{
    QString                       qsInternal;
    QFile                         aipDatabase;
    QMap<Canvas::CountryCode, QString> urlMap;

    Builder::populateUrlMap( &urlMap );

    QVariantList countries = g_pSet->value( "CountryAirports", QVariantList() ).toList();
    QVariant     country;

    g_airportCache.clear();
    foreach( country, countries )
    {
        Builder::getStorage( &qsInternal );
        qsInternal.append( QString( "/data/space.skyfun.stratofier/%1.aip" ).arg( urlMap[static_cast<Canvas::CountryCode>( country.toInt() )] ) );

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
                        ap.frequencies.clear();

                        xChildNode = xAirportNode.firstChild();
                        while( !xChildNode.isNull() )
                        {
                            Frequency f;

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
                            else if( xChildElem.tagName() == "RADIO" )
                            {
                                xSubChildNode = xChildNode.firstChild();
                                while( !xSubChildNode.isNull() )
                                {
                                    xSubChildElem = xSubChildNode.toElement();
                                    if( xSubChildElem.tagName() == "FREQUENCY" )
                                    {
                                        f.dFreq = xSubChildElem.text().toDouble();
                                    }
                                    else if( xSubChildElem.tagName() == "DESCRIPTION" )
                                    {
                                        f.qsDescription = xSubChildElem.text();
                                    }
                                    xSubChildNode = xSubChildNode.nextSibling();
                                }
                                ap.frequencies.append( f );
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
}


void TrafficMath::cacheAirspaces()
{
    QString qsInternal;
    QFile   aipDatabase;

    g_airspaceCache.clear();
    Builder::getStorage( &qsInternal );
    qsInternal.append( "/data/space.skyfun.stratofier/airspace_united_states_us.aip" );

    aipDatabase.setFileName( qsInternal );

    if( !aipDatabase.open( QIODevice::ReadOnly ) )
        return;

    QDomDocument aipDoc( "XML_OpenAIP" );
    QDomElement  xRoot, xWayPtElem, xAirspaceElem, xChildElem, xSubChildElem;
    QDomNode     xWayPtNode, xAirspaceNode, xChildNode, xSubChildNode;
    QString      qsTemp, qsErrMsg;
    Airspace     as;
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

    while( !xWayPtNode.isNull() )
    {
        xWayPtElem = xWayPtNode.toElement();
        if( xWayPtElem.tagName() == "AIRSPACES" )
        {
            xAirspaceNode = xWayPtNode.firstChild();
            while( !xAirspaceNode.isNull() )
            {
                xAirspaceElem = xAirspaceNode.toElement();
                if( xAirspaceElem.tagName() == "ASP" )
                {
                    as.qsName.clear();
                    as.eType = Canvas::Airspace_Class_G;
                    as.iAltTop = 0;
                    as.iAltBottom = 0;
                    as.shape.clear();

                    QString qsTemp = xAirspaceElem.attribute( "CATEGORY" );

                    if( qsTemp == "G" )
                        as.eType = Canvas::Airspace_Class_G;
                    else if( qsTemp == "E" )
                        as.eType = Canvas::Airspace_Class_E;
                    else if( qsTemp == "D" )
                        as.eType = Canvas::Airspace_Class_D;
                    else if( qsTemp == "C" )
                        as.eType = Canvas::Airspace_Class_C;
                    // The AIP database doesn't distinguish between MOA, TFR and SFRA types; most can be resolved by the name but those that can't will
                    // remain assigned to this unofficial type and should probably be colored the same as Restricted or Prohibited
                    else if( qsTemp == "DANGER" )
                        as.eType = Canvas::Airspace_Danger;
                    else if( qsTemp == "PROHIBITED" )
                        as.eType = Canvas::Airspace_Prohibited;
                    else if( qsTemp == "RESTRICTED" )
                        as.eType = Canvas::Airspace_Restricted;

                    xChildNode = xAirspaceNode.firstChild();
                    while( !xChildNode.isNull() )
                    {
                        xChildElem = xChildNode.toElement();
                        if( xChildElem.tagName() == "NAME" )
                        {
                            as.qsName = xChildElem.text();
                            // Try to resolve the ambiguous "DANGER" category to what it really is
                            if( as.qsName.contains( "MOA" ) || as.qsName.contains( "BY NOTAM" ) )
                                as.eType = Canvas::Airspace_MOA;
                            else if( as.qsName.contains( "TFR" ) )
                                as.eType = Canvas::Airspace_TFR;
                            else if( as.qsName.contains( "SFRA" ) )
                                as.eType = Canvas::Airspace_SFRA;
                        }
                        else if( xChildElem.tagName() == "ALTLIMIT_TOP" )
                        {
                            xSubChildNode = xChildNode.firstChild();
                            while( !xSubChildNode.isNull() )
                            {
                                xSubChildElem = xSubChildNode.toElement();
                                if( xSubChildElem.tagName() == "ALT" )
                                    as.iAltTop = xSubChildElem.text().toInt();  // Assume feet
                                xSubChildNode = xSubChildNode.nextSibling();
                            }
                        }
                        else if( xChildElem.tagName() == "ALTLIMIT_BOTTOM" )
                        {
                            xSubChildNode = xChildNode.firstChild();
                            while( !xSubChildNode.isNull() )
                            {
                                xSubChildElem = xSubChildNode.toElement();
                                if( xSubChildElem.tagName() == "ALT" )
                                    as.iAltTop = xSubChildElem.text().toInt();  // Assume feet
                                xSubChildNode = xSubChildNode.nextSibling();
                            }
                        }
                        else if( xChildElem.tagName() == "GEOMETRY" )
                        {
                            xSubChildNode = xChildNode.firstChild();
                            while( !xSubChildNode.isNull() )
                            {
                                xSubChildElem = xSubChildNode.toElement();
                                if( xSubChildElem.tagName() == "POLYGON" )
                                {
                                    qsTemp = xSubChildElem.text();

                                    QStringList qslPolyCoords = qsTemp.split( ',' );
                                    QString     qsCoordPair;

                                    foreach( qsCoordPair, qslPolyCoords )
                                    {
                                        qsCoordPair = qsCoordPair.trimmed();

                                        QStringList qslCoords = qsCoordPair.split( ' ' );
                                        if( qslCoords.count() == 2 )
                                            as.shape.append( QPointF( qslCoords.first().toDouble(), qslCoords.last().toDouble() ) );
                                    }
                                }
                                xSubChildNode = xSubChildNode.nextSibling();
                            }
                        }
                        xChildNode = xChildNode.nextSibling();
                    }
                    g_airspaceCache.append( as );    // Note the cache has no haversine transforms to get the bearing and distance since that's handled by updateNearbyAirports
                }
                xAirspaceNode = xAirspaceNode.nextSibling();
            }
        }
        xWayPtNode = xWayPtNode.nextSibling();
    }
}
