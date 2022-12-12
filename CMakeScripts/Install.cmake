#--------------------- Install config -------------------------#
if(NOT DEFINED INSTALL_CONFIG)
        set(INSTALL_CONFIG "Release")
endif()

IF(WIN32)
install(TARGETS 
        dlEngine

        RUNTIME DESTINATION bin
        PERMISSIONS OWNER_WRITE)

install(FILES
        ${COMPILE_OUTPUT_FOLDER}/${INSTALL_CONFIG}/spdlog.dll
        ${COMPILE_OUTPUT_FOLDER}/${INSTALL_CONFIG}/fmt.dll
        ${COMPILE_OUTPUT_FOLDER}/${INSTALL_CONFIG}/glfw3.dll

        DESTINATION bin
        PERMISSIONS OWNER_WRITE)

ELSEIF(UNIX)
install(TARGETS
        dlEngine

        LIBRARY DESTINATION bin
        PERMISSIONS OWNER_WRITE)

install(FILES
        ${COMPILE_OUTPUT_FOLDER}/spdlog.so
        ${COMPILE_OUTPUT_FOLDER}/fmt.so
        ${COMPILE_OUTPUT_FOLDER}/glfw3.so

        DESTINATION bin
        PERMISSIONS OWNER_WRITE)

ENDIF()

install(TARGETS dlSandbox 
        RUNTIME DESTINATION bin
        PERMISSIONS OWNER_WRITE)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/version.txt
        DESTINATION .)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION .

        PATTERN "cache" EXCLUDE
        PATTERN ${CMAKE_CURRENT_SOURCE_DIR}/shaders

        PERMISSIONS OWNER_WRITE
)

install(DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        DESTINATION .
        )