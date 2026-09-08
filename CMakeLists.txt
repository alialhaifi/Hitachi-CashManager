cmake_minimum_required(VERSION 3.16)
project(CashManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 1. استدعاء مكتبات Qt6 المطلوبة بما فيها مكتبة Network
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

set(ICON_FILE "app_icon.ico")

# 2. التحقق من وجود ملف الأيقونة وتضمينه لمنع فشل البناء
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

# 3. إنشاء الملف التنفيذي
add_executable(CashManager ${APP_SOURCES})

# 4. ربط مكتبات Qt6 بالتطبيق (تتضمن Qt6::Network)
target_link_libraries(CashManager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

# 5. إخفاء الشاشة السوداء (Console) عند تشغيل البرنامج في ويندوز
set_target_properties(CashManager PROPERTIES WIN32_EXECUTABLE TRUE)
