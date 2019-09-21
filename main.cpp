#include <QApplication>
#include <QMainWindow>
#include "mainwindow.h"

#include <QFile>
#include <QDir>
#include <QScopedPointer>
#include <QTextStream>
#include <QDateTime>
#include <QLoggingCategory>

QScopedPointer<QFile>   m_logFile;

// Реализация обработчика
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QTextStream out(m_logFile.data());
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ");
    switch ( type ) {
        case QtInfoMsg:     out << "INF "; break;
        case QtDebugMsg:    out << "DBG "; break;
        case QtWarningMsg:  out << "WRN "; break;
        case QtCriticalMsg: out << "CRT "; break;
        case QtFatalMsg:    out << "FTL "; break;
    }

    out << msg << endl;
    out.flush();
}


int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName( ORGANIZATION_NAME );
    QCoreApplication::setOrganizationDomain( ORGANIZATION_DOMAIN );
    QCoreApplication::setApplicationName( APPLICATION_NAME );

    QApplication a(argc, argv);

    QString logDirPath = a.applicationDirPath() + "/logs";
    QDir logDir;
    if ( !logDir.exists(logDirPath) )
        logDir.mkpath(logDirPath);

    QString logFilePath;
    logFilePath = logDirPath + "/File" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm") + ".txt";
    m_logFile.reset(new QFile(logFilePath));
    m_logFile.data()->open(QFile::Append | QFile::Text);
    // Устанавливаем обработчик (перенаправление отладки)
    qInstallMessageHandler(messageHandler);

    MainWindow w;
    w.show();
    return a.exec();
}
