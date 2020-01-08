/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __STRATUXSTREAMS_H__
#define __STRATUXSTREAMS_H__

#include <QDateTime>
#include <QString>


struct StratuxSituation
{
    double    dLastGPSFixSinceMidnight;
    double    dGPSlat;
    double    dGPSlong;
    int       iGPSFixQuality;
    double    dGPSHeightAboveEllipsoid;
    double    dGPSGeoidSep;
    int       iGPSSats;
    int       iGPSSatsTracked;
    int       iGPSSatsSeen;
    double    dGPSHorizAccuracy;
    int       iGPSNACp;
    double    dGPSAltMSL;
    double    dGPSVertAccuracy;
    double    dGPSVertSpeed;
    QDateTime lastGPSFixTime;
    double    dGPSTrueCourse;
    double    dGPSTurnRate;
    double    dGPSGroundSpeed;
    QDateTime lastGPSGroundTrackTime;
    QDateTime gpsDateTime;
    QDateTime lastGPSTimeStratuxTime;
    QDateTime lastValidNMEAMessageTime;
    QString   qsLastNMEAMsg;
    int       iGPSPosSampleRate;
    double    dBaroTemp;
    double    dBaroPressAlt;
    double    dBaroVertSpeed;
    QDateTime lastBaroMeasTime;
    double    dAHRSpitch;
    double    dAHRSroll;
    double    dAHRSGyroHeading;
    double    dAHRSMagHeading;
    double    dAHRSSlipSkid;
    double    dAHRSTurnRate;
    double    dAHRSGLoad;
    double    dAHRSGLoadMin;
    double    dAHRSGLoadMax;
    QDateTime lastAHRSAttTime;
    int       iAHRSStatus;
    bool      bHaveWTData;
    double    dTAS;
};


// Traffic struct
// NOTE ICAO is deliberately missing since it's used in a map in a higher level class to differentiate aircraft
struct StratuxTraffic
{
    QString   qsReg;
    double    dSigLevel;
    int       iSquawk;
    bool      bOnGround;
    double    dLat;
    double    dLong;
    bool      bPosValid;
    double    dAlt;
    double    dTrack;
    double    dSpeed;
    double    dVertSpeed;
    QString   qsTail;
    QDateTime lastSeen;
    QDateTime timestamp;
    int       iLastSource;
    double    dBearing;
    double    dDist;
    double    dAge;
    bool      bHasADSB;
    QDateTime lastActualReport; // Many of the timestamps appear to be bogus, at least the non-ADSB ones. This is the actual time this ICAO registration was reported and what we key off of to cull old entries
};


struct StratuxStatus
{
    int  iUATTrafficTracking;
    int  iESTrafficTracking;
    int  iGPSSatsLocked;
    bool bGPSConnected;
};


struct WingThingTelemetry
{
    double  dAirspeed;
    double  dAltitude;
    double  dHeading;
    double  dBaroPress;
    double  dTemp;
    int     iChecksum;
};

#endif // __STRATUXSTREAMS_H__
