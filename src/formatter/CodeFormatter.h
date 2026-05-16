#pragma once

#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

/**
 * @brief 代码格式化风格
 */
enum class FormatStyle
{
    Allman,     // 大括号独立一行
    Java,       // Java 风格（K&R 变体）
    Stroustrup, // Stroustrup 风格
    Whitesmith, // Whitesmith 风格
    Google,     // Google C++ Style
    Chromium,   // Chromium Style
    Mozilla,    // Mozilla Style
    WebKit,     // WebKit Style
    Microsoft   // Microsoft Style
};

/**
 * @brief 代码格式化模块
 *
 * 封装 clang-format 调用，提供多种风格化选项。
 * 支持 Allman, Java, Stroustrup, Google 等主流风格。
 */
class CodeFormatter : public QObject
{
    Q_OBJECT

  public:
    explicit CodeFormatter(QObject *parent = nullptr);
    ~CodeFormatter() override;

    /**
     * @brief 设置 clang-format 可执行文件路径
     * @param path clang-format 路径（如未设置则使用 PATH 中的）
     */
    void setClangFormatPath(const QString &path);

    /**
     * @brief 获取当前格式化风格
     */
    FormatStyle formatStyle() const;

    /**
     * @brief 获取风格名称列表（供菜单使用）
     */
    static QStringList availableStyles();

    /**
     * @brief 获取当前风格的可读名称
     */
    QString currentStyleName() const;

  public slots:
    /**
     * @brief 设置格式化风格
     * @param style 目标风格
     */
    void setFormatStyle(FormatStyle style);

    /**
     * @brief 通过名称设置风格（用于菜单联动）
     * @param styleName 如 "Allman", "Java", "Google" 等
     */
    void setFormatStyleByName(const QString &styleName);

    /**
     * @brief 格式化代码文本
     * @param code 原始代码
     * @return QJsonObject { "success": bool, "formatted": QString, "error":
     * QString }
     */
    QJsonObject formatCode(const QString &code);

    /**
     * @brief 格式化文件
     * @param filePath 文件路径
     * @return 是否成功
     */
    QJsonObject formatFile(const QString &filePath);

  signals:
    /**
     * @brief 格式化完成信号
     * @param filePath 被格式化的文件路径（内存格式化时为空）
     * @param formatted 格式化后的代码
     */
    void formattingCompleted(const QString &filePath, const QString &formatted);

    /**
     * @brief 格式化失败信号
     * @param error 错误信息
     */
    void formattingError(const QString &error);

  private:
    /**
     * @brief 将风格枚举转换为 clang-format --style 参数
     */
    QString styleToString(FormatStyle style) const;

    /**
     * @brief 检查 clang-format 是否可用
     */
    bool isClangFormatAvailable() const;

    /**
     * @brief 查找 clang-format 的路径
     */
    QString findClangFormat() const;

    // ────────── 成员 ──────────
    FormatStyle m_style = FormatStyle::Allman;
    QString m_clangFormatPath;
};