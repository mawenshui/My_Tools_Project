#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <QObject>
#include <QApplication>
#include <QSettings>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>

/**
 * @brief 主题管理器类
 * 
 * 负责管理应用程序的主题设置，包括亮色主题、暗色主题等
 * 提供主题切换、保存和加载功能
 */
class ThemeManager : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief 主题类型枚举
     */
    enum ThemeType {
        LightTheme,     ///< 亮色主题
        DarkTheme,      ///< 暗色主题
        SystemTheme     ///< 跟随系统主题
    };
    Q_ENUM(ThemeType)
    
    /**
     * @brief 获取主题管理器单例
     * @return 主题管理器实例
     */
    static ThemeManager* instance();
    
    /**
     * @brief 析构函数
     */
    ~ThemeManager();
    
    /**
     * @brief 设置当前主题
     * @param theme 主题类型
     */
    void setTheme(ThemeType theme);
    
    /**
     * @brief 获取当前主题
     * @return 当前主题类型
     */
    ThemeType getCurrentTheme() const;
    
    /**
     * @brief 获取主题名称
     * @param theme 主题类型
     * @return 主题名称
     */
    QString getThemeName(ThemeType theme) const;
    
    /**
     * @brief 加载保存的主题设置
     */
    void loadThemeSettings();
    
    /**
     * @brief 保存主题设置
     */
    void saveThemeSettings();
    
    /**
     * @brief 获取主题样式表
     * @param theme 主题类型
     * @return 样式表字符串
     */
    QString getThemeStyleSheet(ThemeType theme) const;
    
signals:
    /**
     * @brief 主题变化信号
     * @param theme 新的主题类型
     */
    void themeChanged(ThemeType theme);
    
private:
    /**
     * @brief 构造函数（私有，单例模式）
     * @param parent 父对象
     */
    explicit ThemeManager(QObject *parent = nullptr);
    
    /**
     * @brief 应用亮色主题
     */
    void applyLightTheme();
    
    /**
     * @brief 应用暗色主题
     */
    void applyDarkTheme();
    
    /**
     * @brief 应用系统主题
     */
    void applySystemTheme();
    
    /**
     * @brief 创建亮色主题调色板
     * @return 亮色主题调色板
     */
    QPalette createLightPalette() const;
    
    /**
     * @brief 创建暗色主题调色板
     * @return 暗色主题调色板
     */
    QPalette createDarkPalette() const;
    
    /**
     * @brief 获取亮色主题样式表
     * @return 亮色主题样式表
     */
    QString getLightThemeStyleSheet() const;
    
    /**
     * @brief 获取暗色主题样式表
     * @return 暗色主题样式表
     */
    QString getDarkThemeStyleSheet() const;
    
private:
    static ThemeManager* s_instance;        ///< 单例实例
    ThemeType m_currentTheme;               ///< 当前主题
};

#endif // THEME_MANAGER_H