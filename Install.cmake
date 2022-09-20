#--------------------- Install config -------------------------#
IF(WIN32)
install(TARGETS 
          dlEngine

        RUNTIME DESTINATION bin
        PERMISSIONS OWNER_WRITE)

install(FILES
          ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}-x64/${CMAKE_BUILD_TYPE}/spdlog.dll
          ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}-x64/${CMAKE_BUILD_TYPE}/fmt.dll
          ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}-x64/${CMAKE_BUILD_TYPE}/glfw3.dll

        DESTINATION bin
        PERMISSIONS OWNER_WRITE)

ELSEIF(UNIX)
install(TARGETS
          dlEngine

        LIBRARY DESTINATION bin
        PERMISSIONS OWNER_WRITE)

install(FILES
          ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}-x64/${CMAKE_BUILD_TYPE}/spdlog.so
          ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}-x64/${CMAKE_BUILD_TYPE}/fmt.so
          ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}-x64/${CMAKE_BUILD_TYPE}/glfw3.so

        DESTINATION bin
        PERMISSIONS OWNER_WRITE)

ENDIF()

install(TARGETS dlSandbox 
        RUNTIME DESTINATION bin
        PERMISSIONS OWNER_WRITE)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/version.txt
        DESTINATION .)