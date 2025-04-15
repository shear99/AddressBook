cmake_minimum_required(VERSION 3.16)

project(AddressBook VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ✅ Widgets + Network 모듈 추가
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(PROJECT_SOURCES
    main.cpp
    widget.cpp
    widget.h
    widget.ui

    addressbookmodel.cpp
    addressbookmodel.h
    detailpagewidget.h
    detailpagewidget.cpp
    addressentry.h
    util.h
    mainpagewidget.h
    mainpagewidget.cpp
    mainpagewidget.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(AddressBook
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        .env
        envloader.h
        resources.qrc
        heartdelegate.h heartdelegate.cpp
        loadingdialog.h
        detailpagewidget.ui
        fontupdate.h
        fontupdate.cpp
    )
else()
    if(ANDROID)
        add_library(AddressBook SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(AddressBook
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# ✅ Qt::Network 모듈도 링크
target_link_libraries(AddressBook
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
)

# macOS/iOS용 번들 설정
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.AddressBook)
endif()
set_target_properties(AddressBook PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# 설치 설정
include(GNUInstallDirs)
install(TARGETS AddressBook
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(AddressBook)
endif()
