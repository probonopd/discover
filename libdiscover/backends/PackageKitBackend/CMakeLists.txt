find_package(KF5Archive REQUIRED)

add_subdirectory(runservice)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-paths.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-paths.h)

add_library(packagekit-backend MODULE PackageKitBackend.cpp
    PackageKitResource.cpp
    AppPackageKitResource.cpp
    PKTransaction.cpp
    PackageKitUpdater.cpp
    PackageKitMessages.cpp
    PackageKitSourcesBackend.cpp
    LocalFilePKResource.cpp
)
target_link_libraries(packagekit-backend PRIVATE Discover::Common Qt5::Core PK::packagekitqt5 KF5::ConfigGui KF5::KIOCore KF5::Archive AppStreamQt)

install(TARGETS packagekit-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)

#notifier
add_library(DiscoverPackageKitNotifier MODULE PackageKitNotifier.cpp)
target_link_libraries(DiscoverPackageKitNotifier PRIVATE PK::packagekitqt5 Discover::Notifiers)
set_target_properties(DiscoverPackageKitNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS DiscoverPackageKitNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/discover-notifier)

install(FILES packagekit-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)
add_subdirectory(categoryimages)
