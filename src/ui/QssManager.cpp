#include "QssManager.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

QssManager &QssManager::instance()
{
    static QssManager inst;
    return inst;
}

QssManager::QssManager(QObject *parent)
    : QObject(parent), m_currentTheme("default")
{
}

void QssManager::loadTheme(const QString &themeName)
{
    m_currentTheme = themeName;

    const QString themesDir =
        QApplication::applicationDirPath() + "/DATA/themes";
    const QString qssFile = themesDir + "/" + themeName + ".qss";

    QFile file(qssFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString styleSheet = file.readAll();
        qApp->setStyleSheet(styleSheet);
        file.close();
        qDebug() << "[QssManager] Loaded theme:" << themeName;
    }
    else
    {
        qDebug() << "[QssManager] Theme not found:" << qssFile
                 << "- using default style";
        qApp->setStyleSheet("");
    }
}

QString QssManager::currentTheme() const { return m_currentTheme; }

QStringList QssManager::availableThemes() const
{
    const QString themesDir =
        QApplication::applicationDirPath() + "/DATA/themes";
    QDir dir(themesDir);
    QStringList themes;
    for (const QFileInfo &fi : dir.entryInfoList({"*.qss"}, QDir::Files))
        themes << fi.completeBaseName();
    return themes;
}