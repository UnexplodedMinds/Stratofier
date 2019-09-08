/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __SETTINGSDIALOG_H__
#define __SETTINGSDIALOG_H__

#include <QDialog>

#include "ui_SettingsDialog.h"
#include "Canvas.h"


class QNetworkAccessManager;
class QNetworkReply;
class QFile;


// NOTE: The reason this is called settings instead of Fuel or something similar is that the expectation that other non-fuel related settings
//       will go here or possibly be moved here from the menu.
class SettingsDialog : public QDialog, public Ui::SettingsDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog( QWidget *pParent, CanvasConstants *pC );
    ~SettingsDialog();

    StratofierSettings settings() { return m_settings; }

private:
    enum Getter
    {
        US,
        CA
    };

    void          loadSettings();
    void          startRequest( QUrl url );
    const QString settingsRoot();

    StratofierSettings m_settings;
    QString            m_qsInternalStoragePath;
    QString            m_qsExternalStoragePath;

    QNetworkAccessManager *m_pManager;
    QNetworkReply         *m_pReply;
    QFile                 *m_pFile;
    qint64                 m_fileSize;
    QUrl                   m_url;
    Getter                 m_eGetter;
    int                    m_iTally;
    CanvasConstants       *m_pC;

private slots:
    void traffic( bool bAll );
    void airports();
    void getMapData();
    void storage( bool bExternal );
    void dataSet( int iSet );
    void switchable();
    void runways();

    // Download slots
    void httpReadyRead();
    void httpDownloadFinished();
    void updateDownloadProgress( qint64, qint64 );

signals:
    void trafficToggled( bool );
    void showAirports( Canvas::ShowAirports );
    void showRunways( bool );
};

#endif // __SETTINGSDIALOG_H__
