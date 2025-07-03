cmake_minimum_required(VERSION 3.16)
project(NoteBook LANGUAGES CXX)

set(CMAKE_AUTOMOC   ON)
set(CMAKE_AUTOUIC   ON)
set(CMAKE_AUTORCC   ON)

set(CMAKE_CXX_STANDARD     17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    LinguistTools
)

message(STATUS "Using Qt6 at: ${Qt6_DIR}")

set(SRC
    main.cpp
    widget.cpp
    shortsetting.cpp
    SmoothSwitch.cpp
    MarkdownHighlighter.cpp
    RestartDialog.cpp
    DailyQuoteManager.cpp
    TranslationManager.cpp
    SeparatorDelegate.cpp
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
    WIN32
    ${SRC}
    ${UI}
    ${RES_QRC}
    ${QM_FILES}
    appico.rc
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}

)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network

)
