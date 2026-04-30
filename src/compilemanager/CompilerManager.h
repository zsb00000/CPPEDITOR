#ifndef COMPILERMANAGER_H
#define COMPILERMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QStringList>

class QProcess;

class CompilerManager : public QObject
{
    Q_OBJECT

public:
    explicit CompilerManager(QObject *parent = nullptr);
    ~CompilerManager();

    // 启动时调用：加载已有编译器配置
    void setup();

    // 配置编译器：path, name, 额外flag，保存为JSON
    QJsonObject setcompile(const QString &compilerPath,
                           const QString &compilerName = QString(),
                           const QStringList &defaultFlags = {});

    // 执行编译，返回结果JSON
    QJsonObject compiling(const QString &sourceFile,
                          const QString &outputFile = QString(),
                          const QStringList &extraFlags = {}) const;

    // 运行可执行文件（自动添加 pause，Windows下）
    QJsonObject running(const QString &executablePath,
                        const QStringList &args = {},
                        bool pause = true) const;
  
    QJsonObject manage();

private:
    // 内部工具：生成统一响应格式
    static QJsonObject makeResponse(const QString &process,
                                    const QString &type,
                                    const QString &address,
                                    const QString &message);

    // 配置文件路径
    QString configFilePath() const;

    // 加载/保存配置
    void loadConfig();
    void saveConfig() const;

    // 成员变量
    QString m_compilerPath;        // 编译器可执行文件完整路径
    QString m_compilerName;        // 编译器标识（如 "GCC 13.2"）
    QStringList m_defaultFlags;    // 默认编译参数（如 -std=c++20）
};

#endif // COMPILERMANAGER_H