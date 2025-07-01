cmake_minimum_required(VERSION 3.16)
project(NoteBook LANGUAGES CXX)

set(CMAKE_AUTOMOC   ON)
set(CMAKE_AUTOUIC   ON)
set(CMAKE_AUTORCC   ON)

set(CMAKE_CXX_STANDARD     17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network LinguistTools)

message(STATUS "Using Qt6 at: ${Qt6_DIR}")

set(SRC
    main.cpp
    widget.cpp
    shortsetting.cpp
    SmoothSwitch.cpp
    MarkdownHighlighter.cpp
    RestartDialog.cpp
    DailyQuoteManager.cpp
)

set(HDR
    widget.h
    shortsetting.h
    SmoothSwitch.h
    MarkdownHighlighter.h
    RestartDialog.h
    config.h
    DailyQuoteManager.h
)

set(UI
    widget.ui
    shortsetting.ui
)

set(RES_QRC
    res.qrc
    translations.qrc
)

set(TS_FILES
    ${CMAKE_SOURCE_DIR}/i18n/app_zh_CN.ts
    ${CMAKE_SOURCE_DIR}/i18n/app_en.ts
    ${CMAKE_SOURCE_DIR}/i18n/app_ja.ts
)

qt6_add_translation(QM_FILES ${TS_FILES})

add_executable(${PROJECT_NAME}
    ${SRC}
    ${HDR}
    ${UI}
    ${RES_QRC}
    ${QM_FILES}
    TranslationManager.h
    TranslationManager.cpp
    SeparatorDelegate.cpp
    SeparatorDelegate.h
    DailyQuoteManager.h

    appico.rc
)

# ——— 包含项目根目录到 include path ———————————————
# 这样在编译 ui_*.h 时能找到 SmoothSwitch.h 等头
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ——— 链接 Qt 库 ——————————————————————————————
target_link_libraries(NoteBook PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)
