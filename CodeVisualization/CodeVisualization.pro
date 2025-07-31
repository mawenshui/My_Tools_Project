QT += core widgets charts printsupport concurrent network svg

CONFIG += c++11

# 项目信息
TARGET = CodeVisualization
VERSION = 1.1.3
DESTDIR = $$PWD/bin

# 输出目录
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 源文件
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    src/core/analyzer/code_analyzer.cpp \
    src/core/models/analysis_result.cpp \
    src/core/models/file_statistics.cpp \
    src/core/export/csv_exporter.cpp \
    src/core/export/markdown_exporter.cpp \
    src/ui/widgets/project_selection_widget.cpp \
    src/ui/widgets/statistics_widget.cpp \
    src/ui/widgets/chart_widget.cpp \
    src/ui/widgets/advanced_config_dialog.cpp \
    src/ui/widgets/html_preview_dialog.cpp \
    src/ui/widgets/chart_selection_dialog.cpp \
    src/ui/dialogs/csv_export_dialog.cpp \
    src/ui/dialogs/markdown_export_dialog.cpp \
    src/ui/theme/theme_manager.cpp \
    src/core/config/config_manager.cpp

HEADERS += \
    mainwindow.h \
    src/core/analyzer/code_analyzer.h \
    src/core/models/analysis_result.h \
    src/core/models/file_statistics.h \
    src/core/export/csv_exporter.h \
    src/core/export/markdown_exporter.h \
    src/ui/widgets/project_selection_widget.h \
    src/ui/widgets/statistics_widget.h \
    src/ui/widgets/chart_widget.h \
    src/ui/widgets/advanced_config_dialog.h \
    src/ui/widgets/html_preview_dialog.h \
    src/ui/widgets/chart_selection_dialog.h \
    src/ui/dialogs/csv_export_dialog.h \
    src/ui/dialogs/markdown_export_dialog.h \
    src/ui/theme/theme_manager.h \
    src/core/config/config_manager.h

FORMS += \
    mainwindow.ui

# 包含路径
INCLUDEPATH += src

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
