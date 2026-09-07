cmake_minimum_required(VERSION 3.16)
project(CashManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

set(ICON_FILE "app_icon.ico")

# التحقق من وجود ملف الأيقونة لمنع فشل البناء
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${ICON_FILE}")
    if(WIN32)
        set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc")
        file(WRITE "${RC_FILE}" "IDI_ICON1 ICON DISCARDABLE \"${CMAKE_CURRENT_SOURCE_DIR}/${ICON_FILE}\"\n")
        set(APP_SOURCES main.cpp ${RC_FILE})
    else()
        set(APP_SOURCES main.cpp)
    endif()
else()
    set(APP_SOURCES main.cpp)
endif()

add_executable(CashManager ${APP_SOURCES})

target_link_libraries(CashManager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

# إخفاء النافذة السوداء في ويندوز
set_target_properties(CashManager PROPERTIES WIN32_EXECUTABLE TRUE)
