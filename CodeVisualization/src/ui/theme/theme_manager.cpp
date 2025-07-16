#include "theme_manager.h"
#include <QDebug>
#include <QStyle>

// 静态成员初始化
ThemeManager* ThemeManager::s_instance = nullptr;

ThemeManager* ThemeManager::instance()
{
    if (!s_instance) {
        s_instance = new ThemeManager();
    }
    return s_instance;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
    , m_currentTheme(LightTheme)
{
    loadThemeSettings();
}

ThemeManager::~ThemeManager()
{
    saveThemeSettings();
}

void ThemeManager::setTheme(ThemeType theme)
{
    if (m_currentTheme == theme) {
        return;
    }
    
    m_currentTheme = theme;
    
    switch (theme) {
    case LightTheme:
        applyLightTheme();
        break;
    case DarkTheme:
        applyDarkTheme();
        break;
    case SystemTheme:
        applySystemTheme();
        break;
    }
    
    saveThemeSettings();
    emit themeChanged(theme);
}

ThemeManager::ThemeType ThemeManager::getCurrentTheme() const
{
    return m_currentTheme;
}

QString ThemeManager::getThemeName(ThemeType theme) const
{
    switch (theme) {
    case LightTheme:
        return tr("亮色主题");
    case DarkTheme:
        return tr("暗色主题");
    case SystemTheme:
        return tr("跟随系统");
    default:
        return tr("未知主题");
    }
}

void ThemeManager::loadThemeSettings()
{
    QSettings settings;
    int themeValue = settings.value("theme/currentTheme", static_cast<int>(LightTheme)).toInt();
    m_currentTheme = static_cast<ThemeType>(themeValue);
    
    // 应用加载的主题
    setTheme(m_currentTheme);
}

void ThemeManager::saveThemeSettings()
{
    QSettings settings;
    settings.setValue("theme/currentTheme", static_cast<int>(m_currentTheme));
}

QString ThemeManager::getThemeStyleSheet(ThemeType theme) const
{
    switch (theme) {
    case LightTheme:
        return getLightThemeStyleSheet();
    case DarkTheme:
        return getDarkThemeStyleSheet();
    case SystemTheme:
        // 系统主题使用默认样式
        return QString();
    default:
        return QString();
    }
}

void ThemeManager::applyLightTheme()
{
    QPalette palette = createLightPalette();
    qApp->setPalette(palette);
    qApp->setStyleSheet(getLightThemeStyleSheet());
}

void ThemeManager::applyDarkTheme()
{
    QPalette palette = createDarkPalette();
    qApp->setPalette(palette);
    qApp->setStyleSheet(getDarkThemeStyleSheet());
}

void ThemeManager::applySystemTheme()
{
    // 恢复系统默认主题
    qApp->setPalette(qApp->style()->standardPalette());
    qApp->setStyleSheet(QString());
}

QPalette ThemeManager::createLightPalette() const
{
    QPalette palette;
    
    // 基础颜色
    palette.setColor(QPalette::Window, QColor(240, 240, 240));
    palette.setColor(QPalette::WindowText, QColor(0, 0, 0));
    palette.setColor(QPalette::Base, QColor(255, 255, 255));
    palette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    palette.setColor(QPalette::Text, QColor(0, 0, 0));
    palette.setColor(QPalette::Button, QColor(240, 240, 240));
    palette.setColor(QPalette::ButtonText, QColor(0, 0, 0));
    palette.setColor(QPalette::BrightText, QColor(255, 0, 0));
    palette.setColor(QPalette::Link, QColor(0, 0, 255));
    palette.setColor(QPalette::Highlight, QColor(0, 120, 215));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    
    return palette;
}

QPalette ThemeManager::createDarkPalette() const
{
    QPalette palette;
    
    // 基础颜色
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, QColor(255, 255, 255));
    palette.setColor(QPalette::Base, QColor(25, 25, 25));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::Text, QColor(255, 255, 255));
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
    palette.setColor(QPalette::BrightText, QColor(255, 0, 0));
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
    
    return palette;
}

QString ThemeManager::getLightThemeStyleSheet() const
{
    return QString(
        "QMainWindow {"
        "    background-color: #f0f0f0;"
        "}"
        "QMenuBar {"
        "    background-color: #ffffff;"
        "    border-bottom: 1px solid #d0d0d0;"
        "}"
        "QMenuBar::item {"
        "    background-color: transparent;"
        "    padding: 4px 8px;"
        "}"
        "QMenuBar::item:selected {"
        "    background-color: #e0e0e0;"
        "}"
        "QToolBar {"
        "    background-color: #ffffff;"
        "    border: 1px solid #d0d0d0;"
        "    spacing: 2px;"
        "}"
        "QStatusBar {"
        "    background-color: #ffffff;"
        "    border-top: 1px solid #d0d0d0;"
        "}"
        "QProgressBar {"
        "    border: 1px solid #d0d0d0;"
        "    border-radius: 3px;"
        "    text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #0078d4;"
        "    border-radius: 2px;"
        "}"
        "QSplitter::handle {"
        "    background-color: #d0d0d0;"
        "}"
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #d0d0d0;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px 0 5px;"
        "}"
    );
}

QString ThemeManager::getDarkThemeStyleSheet() const
{
    return QString(
        "QMainWindow {"
        "    background-color: #353535;"
        "    color: #ffffff;"
        "}"
        "QMenuBar {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "    border-bottom: 1px solid #555555;"
        "}"
        "QMenuBar::item {"
        "    background-color: transparent;"
        "    padding: 4px 8px;"
        "}"
        "QMenuBar::item:selected {"
        "    background-color: #404040;"
        "}"
        "QMenu {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "    border: 1px solid #555555;"
        "}"
        "QMenu::item:selected {"
        "    background-color: #404040;"
        "}"
        "QToolBar {"
        "    background-color: #2b2b2b;"
        "    border: 1px solid #555555;"
        "    spacing: 2px;"
        "}"
        "QToolButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    padding: 4px;"
        "}"
        "QToolButton:hover {"
        "    background-color: #404040;"
        "    border-radius: 3px;"
        "}"
        "QStatusBar {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "    border-top: 1px solid #555555;"
        "}"
        "QProgressBar {"
        "    border: 1px solid #555555;"
        "    border-radius: 3px;"
        "    text-align: center;"
        "    color: #ffffff;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #2a82da;"
        "    border-radius: 2px;"
        "}"
        "QSplitter::handle {"
        "    background-color: #555555;"
        "}"
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #555555;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    color: #ffffff;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px 0 5px;"
        "}"
        "QWidget {"
        "    background-color: #353535;"
        "    color: #ffffff;"
        "}"
        "QLineEdit, QTextEdit, QPlainTextEdit {"
        "    background-color: #191919;"
        "    border: 1px solid #555555;"
        "    border-radius: 3px;"
        "    padding: 2px;"
        "    color: #ffffff;"
        "}"
        "QComboBox {"
        "    background-color: #191919;"
        "    border: 1px solid #555555;"
        "    border-radius: 3px;"
        "    padding: 2px;"
        "    color: #ffffff;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "}"
        "QComboBox::down-arrow {"
        "    image: none;"
        "    border-left: 5px solid transparent;"
        "    border-right: 5px solid transparent;"
        "    border-top: 5px solid #ffffff;"
        "}"
        "QPushButton {"
        "    background-color: #404040;"
        "    border: 1px solid #555555;"
        "    border-radius: 3px;"
        "    padding: 5px 10px;"
        "    color: #ffffff;"
        "}"
        "QPushButton:hover {"
        "    background-color: #505050;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #303030;"
        "}"
        "QLabel {"
        "    color: #ffffff;"
        "}"
        "QCheckBox {"
        "    color: #ffffff;"
        "}"
        "QRadioButton {"
        "    color: #ffffff;"
        "}"
        "QTabWidget::pane {"
        "    border: 1px solid #555555;"
        "    background-color: #353535;"
        "}"
        "QTabBar::tab {"
        "    background-color: #2b2b2b;"
        "    border: 1px solid #555555;"
        "    padding: 5px 10px;"
        "    color: #ffffff;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: #353535;"
        "    border-bottom: none;"
        "}"
    );
}