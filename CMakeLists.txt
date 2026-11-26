cmake_minimum_required(VERSION 3.21)
project(clgvpn VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Gui Widgets Quick QuickControls2 Network LinguistTools
)

qt_standard_project_setup(REQUIRES 6.5)

add_library(clgvpn_core STATIC
    src/core/util/stable_hash.cpp
    src/core/util/base64.cpp
    src/core/util/format.cpp
    src/core/util/app_exception.h

    src/core/model/vpn_node.cpp
    src/core/singbox/config_builder.cpp
    src/core/settings/app_settings.cpp

    src/core/storage/app_paths.cpp
    src/core/storage/settings_store.cpp
    src/core/storage/subscription_store.cpp

    src/core/subscription/subscription.cpp
    src/core/subscription/link_parser.cpp
    src/core/subscription/subscription_parser.cpp
    src/core/subscription/subscription_service.cpp
    src/core/subscription/subscription_manager.cpp
    src/core/subscription/auto_refresh.cpp

    src/core/engine/tunnel_state.h
    src/core/engine/privileges.cpp
    src/core/engine/kill_switch.cpp
    src/core/engine/ip_check.cpp
    src/core/engine/log_buffer.cpp
    src/core/engine/clash_api.cpp
    src/core/engine/core_install.cpp
    src/core/engine/desktop_engine.cpp
    src/core/engine/tunnel_controller.cpp

    src/core/app_controller.cpp
)

set_target_properties(clgvpn_core PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

target_include_directories(clgvpn_core PUBLIC src)
target_link_libraries(clgvpn_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
)

# --- Приложение ---
qt_add_executable(clgvpn
    src/main.cpp
)

qt_add_qml_module(clgvpn
    URI Clgvpn
    VERSION 1.0
    QML_FILES
        qml/Snackbar.qml
        qml/Main.qml
        qml/HomePage.qml
        qml/ServersPage.qml
        qml/SettingsPage.qml
        qml/AddSubscriptionDialog.qml
        qml/LogsDialog.qml
)

qt_add_translations(clgvpn
    TS_FILES
        l10n/app_en.ts
        l10n/app_ru.ts
    RESOURCE_PREFIX "/i18n"
)

target_link_libraries(clgvpn PRIVATE
    clgvpn_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
)

install(TARGETS clgvpn
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# --- Тесты ---
include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)

    qt_add_executable(util_test tests/util_test.cpp)
    target_link_libraries(util_test PRIVATE clgvpn_core Qt6::Test)
    add_test(NAME util_test COMMAND util_test)

    qt_add_executable(vpn_node_test tests/vpn_node_test.cpp)
    target_link_libraries(vpn_node_test PRIVATE clgvpn_core Qt6::Test)
    add_test(NAME vpn_node_test COMMAND vpn_node_test)

    qt_add_executable(config_builder_test tests/config_builder_test.cpp)
    target_link_libraries(config_builder_test PRIVATE clgvpn_core Qt6::Test)
    add_test(NAME config_builder_test COMMAND config_builder_test)

    qt_add_executable(storage_test
        tests/storage_test.cpp
        tests/storage_test.h
    )
    target_link_libraries(storage_test PRIVATE clgvpn_core Qt6::Test)
    add_test(NAME storage_test COMMAND storage_test)

    qt_add_executable(subscription_test
        tests/subscription_test.cpp
        tests/subscription_test.h
    )
    target_link_libraries(subscription_test PRIVATE clgvpn_core Qt6::Test)
    add_test(NAME subscription_test COMMAND subscription_test)
endif()
