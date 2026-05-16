#pragma once

#include <QObject>
#include <QString>

/**
 * @brief QSS 主题管理器（单例）
 *
 * Phase 7 将完整实现。此处提供最小接口供 main.cpp 和 ActionManager 使用。
 */
class QssManager : public QObject
{
    Q_OBJECT

  public:
    static QssManager &instance();

    void loadTheme(const QString &themeName);
    QString currentTheme() const;
    QStringList availableThemes() const;

  private:
    explicit QssManager(QObject *parent = nullptr);
    QString m_currentTheme;
};