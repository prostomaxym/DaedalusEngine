#--------------------- Package config -------------------------#
message("-- Daedalus version: ${DAEDALUS_VERSION}")

include(InstallRequiredSystemLibraries)

set(CPACK_OUTPUT_FILE_PREFIX  "${CMAKE_CURRENT_SOURCE_DIR}/runtime")
set(CPACK_NSIS_MODIFY_PATH "on")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/runtime")
set(CPACK_PACKAGE_EXECUTABLES "dlSandbox;Daedalus Sandbox")
set(CPACK_CREATE_DESKTOP_LINKS dlSandbox)

if(WIN32)
	# Protect retained settings from CPack's package-owned file deletion list.
	set(CPACK_NSIS_DEFINES [[
Var DaedalusPreservedConfig

Function un.onUninstSuccess
	StrCmp $DaedalusPreservedConfig "" daedalus_restore_done
	ClearErrors
	Rename "$DaedalusPreservedConfig" "$INSTDIR\config"
	IfErrors 0 daedalus_restore_done
	MessageBox MB_OK|MB_ICONEXCLAMATION "Your settings remain at $DaedalusPreservedConfig. Restore that folder to $INSTDIR\config manually." /SD IDOK
daedalus_restore_done:
FunctionEnd
]])

	set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS [[
	StrCpy $DaedalusPreservedConfig ""
	IfFileExists "$INSTDIR\config\*.*" 0 daedalus_cleanup_generated
	MessageBox MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 "Remove settings and user data? This permanently deletes only the config folder. Choose No to keep your settings." /SD IDNO IDYES daedalus_remove_config

	ClearErrors
	GetTempFileName $DaedalusPreservedConfig "$INSTDIR"
	IfErrors daedalus_preserve_failed
	Delete "$DaedalusPreservedConfig"
	IfErrors daedalus_preserve_failed
	Rename "$INSTDIR\config" "$DaedalusPreservedConfig"
	IfErrors daedalus_preserve_failed daedalus_cleanup_generated

daedalus_preserve_failed:
	MessageBox MB_OK|MB_ICONSTOP "Unable to preserve your config folder. Uninstallation has been stopped; your settings have not been deleted." /SD IDOK
	Abort

daedalus_remove_config:
	RMDir /r "$INSTDIR\config"

daedalus_cleanup_generated:
	RMDir /r "$INSTDIR\logs"
	RMDir /r "$INSTDIR\shader\cache"
]])
endif()

set(CPACK_PACKAGE_NAME "Daedalus Development Sandbox")
set(CPACK_PACKAGE_VENDOR "Maksym Yershykov Dev")
set(CPACK_PACKAGE_DESCRIPTION "Development Kit for Daedalus Engine: Daedalus Engine, Sandbox Application and Daedalus Editor")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Development Kit for Daedalus Engine")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${DAEDALUS_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${DAEDALUS_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${DAEDALUS_VERSION_PATCH}")
set(CPACK_SOURCE_GENERATOR "TGZ")

set(CPACK_THREADS 0)
set(CPACK_VERBATIM_VARIABLES ON)
include(CPack)