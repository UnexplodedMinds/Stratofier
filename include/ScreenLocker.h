#ifndef __SCREENLOCKER_H__
#define __SCREENLOCKER_H__

#include <QAndroidJniObject>


class ScreenLocker
{
public:
    ScreenLocker();
    virtual ~ScreenLocker();

private:
    QAndroidJniObject m_screenLock;
};

#endif // __SCREENLOCKER_H__
