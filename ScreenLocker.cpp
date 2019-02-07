#include <QAndroidJniObject>
#include <QtDebug>

#include "jni.h"

#include "ScreenLocker.h"

ScreenLocker::ScreenLocker()
{
    QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod( "org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;" );

    if( activity.isValid() )
    {
        QAndroidJniObject serviceName = QAndroidJniObject::getStaticObjectField<jstring>( "android/content/Context", "POWER_SERVICE" );

        if( serviceName.isValid() )
        {
            QAndroidJniObject powerMgr = activity.callObjectMethod( "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", serviceName.object<jobject>() );

            if ( powerMgr.isValid() )
            {
                jint              levelAndFlags = QAndroidJniObject::getStaticField<jint>( "android/os/PowerManager", "SCREEN_DIM_WAKE_LOCK" );
                QAndroidJniObject tag = QAndroidJniObject::fromString( "My Tag" );

                m_screenLock = powerMgr.callObjectMethod( "newWakeLock", "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;", levelAndFlags, tag.object<jstring>() );
            }
        }
    }

    if( m_screenLock.isValid() )
    {
        m_screenLock.callMethod<void>( "acquire", "()V" );

        qInfo() << "Screen stay on enabled";
    }
    else
        assert( false );
}

ScreenLocker::~ScreenLocker()
{
    if ( m_screenLock.isValid() )
    {
        m_screenLock.callMethod<void>( "release", "()V" );
        qInfo() << "Screen stay on disabled";
    }
}
