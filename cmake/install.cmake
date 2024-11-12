install(TARGETS event_dispatcher EXPORT event_dispatcher_targets FILE_SET public_headers)
install(EXPORT event_dispatcher_targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/event_dispatcher)

include(CMakePackageConfigHelpers)

set(SYSCONFIG_INSTALL_DIR ${CMAKE_INSTALL_SYSCONFDIR}/event_dispatcher
  CACHE PATH "Location of configuration files" )
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}
  CACHE PATH "Location of header files" )

configure_package_config_file(cmake/event_dispatcher-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/event_dispatcherConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/event_dispatcher
  PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/event_dispatcher-configVersion.cmake
  VERSION ${PROJECT_VERSION} 
  COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/event_dispatcher-config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/event_dispatcher-configVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/event_dispatcher)
