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
        ${COMPILE_OUTPUT_FOLDER}/${INSTALL_CONFIG}/assimp-vc143-mt.dll
        ${COMPILE_OUTPUT_FOLDER}/${INSTALL_CONFIG}/pugixml.dll
        ${COMPILE_OUTPUT_FOLDER}/${INSTALL_CONFIG}/zlib1.dll

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
        ${COMPILE_OUTPUT_FOLDER}/${INSTALL_CONFIG}/assimp-vc143-mt.so
        ${COMPILE_OUTPUT_FOLDER}/${INSTALL_CONFIG}/pugixml.so
        ${COMPILE_OUTPUT_FOLDER}/${INSTALL_CONFIG}/zlib1.so

        DESTINATION bin
        PERMISSIONS OWNER_WRITE)

ENDIF()

install(TARGETS dlSandbox 
        RUNTIME DESTINATION bin
        PERMISSIONS OWNER_WRITE)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/version.txt
        DESTINATION .)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/runtime/shaders DESTINATION .

        PATTERN "cache" EXCLUDE
        PATTERN ${CMAKE_CURRENT_SOURCE_DIR}/runtime/shaders

        PERMISSIONS OWNER_WRITE
)

install(DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime/assets
        DESTINATION .
        )