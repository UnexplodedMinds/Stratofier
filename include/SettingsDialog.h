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
    void          loadSettings();
    void          startRequestAirports( QUrl url );
    void          startRequestAirspace( QUrl url );
    const QString settingsRoot();
    bool          nextCountryAirport();
    bool          nextCountryAirspace();

    StratofierSettings m_settings;
    QString            m_qsInternalStoragePath;
    QString            m_qsExternalStoragePath;

    QNetworkAccessManager *m_pManager;
    QNetworkReply         *m_pReply;
    QFile                 *m_pFile;
    qint64                 m_fileSize;
    QUrl                   m_url;
    int                    m_iTally;
    CanvasConstants       *m_pC;

    QMap<Canvas::CountryCodeAirports, QString>         m_mapUrlsAirports;
    QMapIterator<Canvas::CountryCodeAirports, QString> m_mapItAirports;
    QMap<Canvas::CountryCodeAirspace, QString>         m_mapUrlsAirspaces;
    QMapIterator<Canvas::CountryCodeAirspace, QString> m_mapItAirspaces;

private slots:
    void init();
    void getMapDataAirports();
    void getMapDataAirspace();
    void storage();
    void switchable();
    void selCountries();
    void magDevChange();

    // Download slots
    void httpReadyRead();
    void httpDownloadFinishedAirports();
    void httpDownloadFinishedAirspace();
    void updateDownloadProgressAirports( qint64, qint64 );
    void updateDownloadProgressAirspace( qint64, qint64 );
};

#endif // __SETTINGSDIALOG_H__
