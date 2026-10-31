#--------------------- Install config -------------------------#

IF(WIN32)
install(TARGETS 
        dlEngine

        RUNTIME DESTINATION bin
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

# zlib ships as zlib1.dll or z.dll depending on the vcpkg baseline, so missing DLLs are skipped
install(FILES
        ${COMPILE_OUTPUT_FOLDER}/$<CONFIG>/spdlog.dll
        ${COMPILE_OUTPUT_FOLDER}/$<CONFIG>/fmt.dll
        ${COMPILE_OUTPUT_FOLDER}/$<CONFIG>/glfw3.dll
        ${COMPILE_OUTPUT_FOLDER}/$<CONFIG>/assimp-vc145-mt.dll
        ${COMPILE_OUTPUT_FOLDER}/$<CONFIG>/pugixml.dll
        ${COMPILE_OUTPUT_FOLDER}/$<CONFIG>/zlib1.dll
        ${COMPILE_OUTPUT_FOLDER}/$<CONFIG>/z.dll
        ${COMPILE_OUTPUT_FOLDER}/$<CONFIG>/poly2tri.dll
        ${COMPILE_OUTPUT_FOLDER}/$<CONFIG>/minizip.dll
        ${COMPILE_OUTPUT_FOLDER}/$<CONFIG>/kubazip.dll

        DESTINATION bin
        OPTIONAL
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

ELSEIF(UNIX)
install(TARGETS
        dlEngine

        LIBRARY DESTINATION bin
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

ENDIF()

install(TARGETS dlSandbox 
        RUNTIME DESTINATION bin
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

# runtime/ is generated and not in git; keybinds.ini only exists after the app saved it, defaults are used otherwise
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/runtime/config/keybinds.ini
        DESTINATION ./config/
        OPTIONAL)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/source/dlSandbox/path.ini
        DESTINATION .)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/version.txt
        DESTINATION .)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/runtime/shader DESTINATION .

        PATTERN "cache" EXCLUDE
        PATTERN ${CMAKE_CURRENT_SOURCE_DIR}/runtime/shader

        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)

install(DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime/asset
        DESTINATION .
        OPTIONAL
        )
