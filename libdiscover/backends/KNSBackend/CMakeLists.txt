add_subdirectory(tests)

add_library(kns-backend MODULE
    KNSBackend.cpp
    KNSResource.cpp
    KNSReviews.cpp)

target_link_libraries(kns-backend Discover::Common KF5::ConfigCore KF5::Attica KF5::NewStuffCore KF5::WidgetsAddons)

install(TARGETS kns-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)
install(FILES custom/discover_ktexteditor_codesnippets_core.knsrc DESTINATION ${KDE_INSTALL_CONFDIR})
