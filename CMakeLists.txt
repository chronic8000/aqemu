# AQEMU CMake Project

# Authors:
# Andrey Rijov (AQEMU developer)
# Timothy Redaelli (Fix for build in a different folder)
# Tobias Gläßer (Qt5 port)

CMAKE_MINIMUM_REQUIRED( VERSION 3.5 FATAL_ERROR )
PROJECT( AQEMU CXX )

# Host platforms we ship and document. macOS guests under QEMU are fine;
# building AQEMU itself on macOS is not supported.
IF( APPLE )
	MESSAGE( FATAL_ERROR
		"AQEMU host builds are only supported on Linux (incl. Raspberry Pi) and Windows. "
		"macOS is not a supported build target. You can still run macOS as a QEMU guest from Linux/Windows." )
ENDIF()

# Available options
OPTION( DEBUG "Build with debug information" )
OPTION( WITHOUT_EMBEDDED_DISPLAY "Build without support Embedded VNC Display" )
OPTION( UPDATE_TRANSLATIONS "Update source translation files (WARNING: make clean will delete the source .ts files!)" )
OPTION( INSTALL_MAN "Install MAN page file" "ON" )
OPTION( PI5_OPTIMIZATIONS "Enable compiler and linker optimizations for Raspberry Pi 5" OFF )
OPTION( AQEMU_BUNDLE_QEMU "Prefer/copy QEMU from AQEMU_QEMU_PREFIX into the build tree" OFF )
OPTION( AQEMU_WITH_SPICE_GTK "Build embedded SPICE client via spice-client-glib (Linux/Pi/Windows)" OFF )
SET( AQEMU_QEMU_PREFIX "${CMAKE_SOURCE_DIR}/third_party/qemu-install" CACHE PATH "Prefix of a built QEMU bundle (bin/qemu-system-*)" )

# Debug mode
IF( DEBUG )
	# Verbose compiler output
	SET( CMAKE_VERBOSE_MAKEFILE ON )
	
	# GCC
	ADD_DEFINITIONS( -Wall -g )
ENDIF( DEBUG )

# Raspberry Pi 5 Optimizations
IF( PI5_OPTIMIZATIONS )
	# Optimize for Cortex-A76 and enable high-level optimizations
	ADD_COMPILE_OPTIONS( -O3 -mcpu=cortex-a76 -mtune=cortex-a76 )
	# Align ELF load segments to 64KB page boundaries to support 16KB kernels on Pi 5
	SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,max-page-size=65536" )
	MESSAGE( STATUS "Raspberry Pi 5 optimizations enabled (-mcpu=cortex-a76, -z max-page-size=65536)" )
ENDIF( PI5_OPTIMIZATIONS )

# For finding VNC libs
SET( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/resources/cmake/modules )

# Files
SET( aqemu_headers
    src/No_Boot_Device.h
    src/Highlighted_Label.h
    src/Service.h
    src/Run_Guard.h
    src/QEMU_Docopt.h
    src/IP_Edit.h
    src/Dialog_Button_Box.h
	src/docopt/docopt.h
    	src/Settings_Widget.h
    	src/QDom.h
    	src/tinyxml2.h
	src/About_Window.h
	src/Add_New_Device_Window.h
	src/Add_Port_Window.h
	src/Add_USB_Port_Window.h
	src/Advanced_Settings_Window.h
	src/Boot_Device_Window.h
	src/Convert_HDD_Image_Window.h
	src/Copy_VM_Window.h
	src/Create_HDD_Image_Window.h
	src/Create_Template_Window.h
	src/Delete_VM_Files_Window.h
	src/Device_Manager_Widget.h
	src/Edit_Emulator_Version_Window.h
	src/Emulator_Control_Window.h
	src/Error_Log_Window.h
	src/First_Start_Wizard.h
	src/Folder_Sharing_Widget.h
	src/HDD_Image_Info.h
	src/Main_Window.h
	src/Monitor_Window.h
	src/Network_Widget.h
	src/Old_Network_Widget.h
	src/Ports_Tab_Widget.h
	src/Properties_Window.h
	src/Emulator_Options_Window.h
	src/Select_Icon_Window.h
	src/SMP_Settings_Window.h
	src/Snapshot_Properties_Window.h
	src/Snapshots_Window.h
	src/SPICE_Settings_Widget.h
	src/System_Info.h
	src/Utils.h
	src/VM.h
	src/VM_Devices.h
	src/VM_Wizard_Window.h
	src/VNC_Password_Window.h
	src/QMP_Client.h
	src/Embedded_Display/Guest_Display_View.h
	src/Embedded_Display/Spice_View.h
	src/VM_Session_Widget.h
)

SET( aqemu_sources
    src/No_Boot_Device.cpp
    src/Highlighted_Label.cpp
    src/Service.cpp
    src/Run_Guard.cpp
    src/QEMU_Docopt.cpp
    src/IP_Edit.cpp
    src/Dialog_Button_Box.cpp
	src/docopt/docopt.cpp
	src/Settings_Widget.cpp
    	src/QDom.cpp
    	src/tinyxml2.cpp
	src/About_Window.cpp
	src/Add_New_Device_Window.cpp
	src/Add_Port_Window.cpp
	src/Add_USB_Port_Window.cpp
	src/Advanced_Settings_Window.cpp
	src/Boot_Device_Window.cpp
	src/Convert_HDD_Image_Window.cpp
	src/Copy_VM_Window.cpp
	src/Create_HDD_Image_Window.cpp
	src/Create_Template_Window.cpp
	src/Delete_VM_Files_Window.cpp
	src/Device_Manager_Widget.cpp
	src/Edit_Emulator_Version_Window.cpp
	src/Emulator_Control_Window.cpp
	src/Error_Log_Window.cpp
	src/First_Start_Wizard.cpp
	src/Folder_Sharing_Widget.cpp
	src/HDD_Image_Info.cpp
	src/main.cpp
	src/Main_Window.cpp
	src/Monitor_Window.cpp
	src/Network_Widget.cpp
	src/Old_Network_Widget.cpp
	src/Ports_Tab_Widget.cpp
	src/Properties_Window.cpp
	src/Emulator_Options_Window.cpp
	src/Select_Icon_Window.cpp
	src/SMP_Settings_Window.cpp
	src/Snapshot_Properties_Window.cpp
	src/Snapshots_Window.cpp
	src/SPICE_Settings_Widget.cpp
	src/System_Info.cpp
	src/Utils.cpp
	src/VM.cpp
	src/VM_Devices.cpp
	src/VM_Wizard_Window.cpp
	src/VNC_Password_Window.cpp
	src/QMP_Client.cpp
	src/Embedded_Display/Guest_Display_View.cpp
	src/Embedded_Display/Spice_View.cpp
	src/VM_Session_Widget.cpp
)

SET( aqemu_ui
    src/No_Boot_Device.ui
	src/About_Window.ui
	src/Add_New_Device_Window.ui
	src/Add_Port_Window.ui
	src/Add_USB_Port_Window.ui
	src/Advanced_Settings_Window.ui
	src/Boot_Device_Window.ui
	src/Convert_HDD_Image_Window.ui
	src/Copy_VM_Window.ui
	src/Create_HDD_Image_Window.ui
	src/Create_Template_Window.ui
	src/Delete_VM_Files_Window.ui
	src/Device_Manager_Widget.ui
	src/Edit_Emulator_Version_Window.ui
	src/Emulator_Control_Window.ui
	src/Error_Log_Window.ui
	src/First_Start_Wizard.ui
	src/Folder_Sharing_Widget.ui
	src/Main_Window.ui
	src/Monitor_Window.ui
	src/Network_Widget.ui
	src/Old_Network_Widget.ui
	src/Ports_Tab_Widget.ui
	src/Properties_Window.ui
	src/Emulator_Options_Window.ui
	src/Select_Icon_Window.ui
	src/Settings_Window.ui
	src/SMP_Settings_Window.ui
	src/Snapshot_Properties_Window.ui
	src/Snapshots_Window.ui
	src/SPICE_Settings_Widget.ui
	src/VM_Wizard_Window.ui
	src/VNC_Password_Window.ui
)

SET( aqemu_qrc 
    resources/shared_images.qrc
    resources/icons.qrc
)

SET( aqemu_man_page
    resources/docs/aqemu.1 )

SET( translations 
     translations/German.ts
     transtations/Russian.ts
     translations/Ukrainian.ts
     translations/French.ts
     translations/English.ts )

SET( files_to_translate ${files_to_translate} ${aqemu_headers} ${aqemu_sources} ${aqemu_ui} )

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to run uic automatically when needed.
SET(CMAKE_AUTOUIC ON)
# Instruct CMake to run rcc automatically when needed.
SET(CMAKE_AUTORCC ON)

# Check Qt
FIND_PACKAGE( Qt5Core )
FIND_PACKAGE( Qt5Widgets )
FIND_PACKAGE( Qt5Network )
FIND_PACKAGE( Qt5Test )
FIND_PACKAGE( Qt5PrintSupport )
FIND_PACKAGE( Qt5DBus )

# Embedded VNC display
IF( NOT WITHOUT_EMBEDDED_DISPLAY )
	# Prefer WinLibs-built static LibVNC (no GnuTLS) over MSYS2 packages
	IF( EXISTS "${CMAKE_SOURCE_DIR}/third_party/libvnc-install/include/rfb/rfbclient.h" )
		SET( LIBVNCCLIENT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/libvnc-install/include" CACHE PATH "" FORCE )
		SET( LIBVNCSERVER_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/libvnc-install/include" CACHE PATH "" FORCE )
		SET( LIBVNCCLIENT_LIBRARIES "${CMAKE_SOURCE_DIR}/third_party/libvnc-install/lib/libvncclient.a" CACHE FILEPATH "" FORCE )
		SET( LIBVNCSERVER_LIBRARIES "${CMAKE_SOURCE_DIR}/third_party/libvnc-install/lib/libvncserver.a" CACHE FILEPATH "" FORCE )
		SET( LIBVNCSERVER_FOUND TRUE CACHE INTERNAL "" FORCE )
		MESSAGE( STATUS "Using vendored LibVNCClient: ${LIBVNCCLIENT_LIBRARIES}" )
	ELSE()
		FIND_PACKAGE( LibVNCServer REQUIRED )
	ENDIF()
	
	SET( aqemu_headers ${aqemu_headers} src/Embedded_Display/Machine_View.h src/Embedded_Display/remoteview.h
										src/Embedded_Display/vncclientthread.h src/Embedded_Display/vncview.h )
	
	SET( aqemu_sources ${aqemu_sources} src/Embedded_Display/Machine_View.cpp src/Embedded_Display/remoteview.cpp
										src/Embedded_Display/vncclientthread.cpp src/Embedded_Display/vncview.cpp )
	
	ADD_DEFINITIONS( -DVNC_DISPLAY -DQTONLY )
ENDIF( NOT WITHOUT_EMBEDDED_DISPLAY )

# TODO: port from Qt4 to Qt5
# For translation
#IF( UPDATE_TRANSLATIONS )
#	QT5_CREATE_TRANSLATION( qm_files ${CMAKE_CURRENT_SOURCE_DIR} ${files_to_translate} ${translations} )
#ELSE( UPDATE_TRANSLATIONS )
#	QT5_ADD_TRANSLATION( qm_files ${translations} )
#ENDIF( UPDATE_TRANSLATIONS )

ADD_CUSTOM_TARGET( translations_target DEPENDS ${qm_files} )

IF( COMMAND CMAKE_POLICY )
	CMAKE_POLICY( SET CMP0003 NEW )
ENDIF( COMMAND CMAKE_POLICY )

ADD_EXECUTABLE( aqemu
	${aqemu_sources}
	${ui_headers}
	${moc_srcs}
	${qm_files}
    ${aqemu_qrc}
)

target_include_directories(aqemu PUBLIC src/ )

# Require C++-11 standard
SET_PROPERTY(TARGET aqemu PROPERTY CXX_STANDARD 11)
SET_PROPERTY(TARGET aqemu PROPERTY CXX_STANDARD_REQUIRED ON)

TARGET_LINK_LIBRARIES( aqemu
	Qt5::Widgets
    Qt5::Network
    Qt5::Test
    Qt5::PrintSupport
    Qt5::DBus
)

# Embedded VNC display
IF( NOT WITHOUT_EMBEDDED_DISPLAY )
	IF( EXISTS "${CMAKE_SOURCE_DIR}/third_party/libvnc-install/include/rfb/rfbclient.h" )
		TARGET_INCLUDE_DIRECTORIES( aqemu PRIVATE "${CMAKE_SOURCE_DIR}/third_party/libvnc-install/include" )
		IF( EXISTS "${CMAKE_SOURCE_DIR}/third_party/zlib-install/include/zlib.h" )
			TARGET_INCLUDE_DIRECTORIES( aqemu PRIVATE "${CMAKE_SOURCE_DIR}/third_party/zlib-install/include" )
		ENDIF()
	ELSEIF( EXISTS "${CMAKE_SOURCE_DIR}/third_party/libvnc_inc/rfb/rfbclient.h" )
		TARGET_INCLUDE_DIRECTORIES( aqemu PRIVATE "${CMAKE_SOURCE_DIR}/third_party/libvnc_inc" )
	ELSE()
		TARGET_INCLUDE_DIRECTORIES( aqemu PRIVATE ${LIBVNCCLIENT_INCLUDE_DIR} ${LIBVNCSERVER_INCLUDE_DIR} )
	ENDIF()
	TARGET_LINK_LIBRARIES( aqemu ${LIBVNCCLIENT_LIBRARIES} )
	# Static libvncclient needs zlib (and ws2_32 on Windows)
	IF( EXISTS "${CMAKE_SOURCE_DIR}/third_party/zlib-install/lib/libzlibstatic.a" )
		TARGET_LINK_LIBRARIES( aqemu "${CMAKE_SOURCE_DIR}/third_party/zlib-install/lib/libzlibstatic.a" )
	ENDIF()
	IF( WIN32 )
		TARGET_LINK_LIBRARIES( aqemu ws2_32 )
	ENDIF()
ENDIF( NOT WITHOUT_EMBEDDED_DISPLAY )

# Optional spice-client-glib (Qt paints frames; no GtkWidget embed)
# Windows (WinLibs UCRT): install MSYS2 ucrt64 packages and point pkg-config at them:
#   pacman -S mingw-w64-ucrt-x86_64-spice-gtk mingw-w64-ucrt-x86_64-pkgconf
#   set PKG_CONFIG_PATH=C:/msys64/ucrt64/lib/pkgconfig
IF( AQEMU_WITH_SPICE_GTK )
	FIND_PACKAGE( PkgConfig REQUIRED )
	# Prefer glib-only module; fall back to spice-client-gtk-3.0 (pulls glib as dep)
	PKG_CHECK_MODULES( SPICE_GLIB spice-client-glib-2.0 )
	IF( NOT SPICE_GLIB_FOUND )
		PKG_CHECK_MODULES( SPICE_GLIB REQUIRED spice-client-gtk-3.0 )
	ENDIF()
	TARGET_INCLUDE_DIRECTORIES( aqemu PRIVATE ${SPICE_GLIB_INCLUDE_DIRS} )
	TARGET_COMPILE_OPTIONS( aqemu PRIVATE ${SPICE_GLIB_CFLAGS_OTHER} )
	# Link by absolute path — do NOT add MSYS2 lib/ via LINK_DIRECTORIES
	# (that pulls the wrong mingw CRT when building with WinLibs).
	SET( _spice_libs_abs "" )
	FOREACH( _l ${SPICE_GLIB_LIBRARIES} )
		IF( IS_ABSOLUTE "${_l}" )
			LIST( APPEND _spice_libs_abs "${_l}" )
		ELSE()
			SET( _fl "_spice_find_${_l}" )
			FIND_LIBRARY( ${_fl} NAMES "${_l}" PATHS ${SPICE_GLIB_LIBRARY_DIRS} NO_DEFAULT_PATH )
			IF( ${_fl} )
				LIST( APPEND _spice_libs_abs "${${_fl}}" )
			ELSE()
				LIST( APPEND _spice_libs_abs "${_l}" )
			ENDIF()
		ENDIF()
	ENDFOREACH()
	TARGET_LINK_LIBRARIES( aqemu ${_spice_libs_abs} )
	# New name + legacy alias used by existing #ifdefs
	ADD_DEFINITIONS( -DAQEMU_HAVE_SPICE_GLIB -DAQEMU_HAVE_SPICE_GTK )
	MESSAGE( STATUS "SPICE client (spice-client-glib) enabled" )

	IF( WIN32 )
		# Resolve ucrt64 bin dir from pkg-config -L flags or default MSYS2 path
		SET( _spice_bin_dir "" )
		FOREACH( _libdir ${SPICE_GLIB_LIBRARY_DIRS} )
			GET_FILENAME_COMPONENT( _cand "${_libdir}/../bin" ABSOLUTE )
			IF( EXISTS "${_cand}" )
				SET( _spice_bin_dir "${_cand}" )
				BREAK()
			ENDIF()
		ENDFOREACH()
		IF( NOT _spice_bin_dir AND EXISTS "C:/msys64/ucrt64/bin" )
			SET( _spice_bin_dir "C:/msys64/ucrt64/bin" )
		ENDIF()
		IF( _spice_bin_dir )
			FILE( GLOB _spice_seed "${_spice_bin_dir}/libspice-client-glib-*.dll" )
			LIST( GET _spice_seed 0 _spice_seed_dll )
			GET_FILENAME_COMPONENT( _spice_seed_name "${_spice_seed_dll}" NAME )
			# Also seed QEMU's spice-server DLL (bundled QEMU is linked against it on Windows)
			SET( _spice_extra_seeds "${_spice_seed_name}" )
			IF( EXISTS "${_spice_bin_dir}/libspice-server-1.dll" )
				LIST( APPEND _spice_extra_seeds "libspice-server-1.dll" )
			ENDIF()
			FOREACH( _seed ${_spice_extra_seeds} )
				ADD_CUSTOM_COMMAND( TARGET aqemu POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E echo "Copying SPICE runtime DLLs (seed ${_seed}) from ${_spice_bin_dir}"
					COMMAND powershell -NoProfile -ExecutionPolicy Bypass
						-File "${CMAKE_SOURCE_DIR}/resources/cmake/CopySpiceRuntime.ps1"
						-BinDir "${_spice_bin_dir}"
						-DestDir "$<TARGET_FILE_DIR:aqemu>"
						-SeedDll "${_seed}"
					COMMENT "Bundle SPICE runtime DLLs (${_seed})"
				)
			ENDFOREACH()
			MESSAGE( STATUS "Will copy SPICE DLLs from ${_spice_bin_dir} next to aqemu" )
		ELSE()
			MESSAGE( WARNING "SPICE enabled but could not locate ucrt64 bin for DLL copy" )
		ENDIF()
	ENDIF( WIN32 )
ENDIF( AQEMU_WITH_SPICE_GTK )

# Bundle QEMU binaries next to aqemu when requested
IF( AQEMU_BUNDLE_QEMU )
	IF( EXISTS "${AQEMU_QEMU_PREFIX}/bin" )
		SET( _qemu_bin_dir "${AQEMU_QEMU_PREFIX}/bin" )
	ELSEIF( EXISTS "${AQEMU_QEMU_PREFIX}" )
		SET( _qemu_bin_dir "${AQEMU_QEMU_PREFIX}" )
	ELSE()
		MESSAGE( WARNING "AQEMU_BUNDLE_QEMU set but AQEMU_QEMU_PREFIX not found: ${AQEMU_QEMU_PREFIX}" )
		SET( _qemu_bin_dir "" )
	ENDIF()
	IF( _qemu_bin_dir )
		FILE( GLOB _qemu_bins
			"${_qemu_bin_dir}/qemu-system-*"
			"${_qemu_bin_dir}/qemu-img*"
		)
		FOREACH( _qb ${_qemu_bins} )
			ADD_CUSTOM_COMMAND( TARGET aqemu POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_qb}" "$<TARGET_FILE_DIR:aqemu>/"
				COMMENT "Bundle QEMU: ${_qb}"
			)
		ENDFOREACH()
		MESSAGE( STATUS "Will copy QEMU binaries from ${_qemu_bin_dir} next to aqemu" )
	ENDIF()
ENDIF( AQEMU_BUNDLE_QEMU )

# Install files
INSTALL( TARGETS aqemu DESTINATION bin )

INSTALL( FILES ${qm_files} DESTINATION share/aqemu )

FILE( GLOB os_icons "${CMAKE_CURRENT_SOURCE_DIR}/resources/os_icons/*.*" )
INSTALL( FILES ${os_icons} DESTINATION share/aqemu/os_icons )

FILE( GLOB os_templates "${CMAKE_CURRENT_SOURCE_DIR}/resources/os_templates/*.*")
INSTALL( FILES ${os_templates} DESTINATION share/aqemu/os_templates )

FILE( GLOB links "${CMAKE_CURRENT_SOURCE_DIR}/resources/docs/links.html" )
INSTALL( FILES ${links} DESTINATION share/aqemu/docs )

FILE( GLOB pixmaps "${CMAKE_CURRENT_SOURCE_DIR}/resources/menu_data/*.png" )
INSTALL( FILES ${pixmaps} DESTINATION share/pixmaps )

FILE( GLOB app_menu "${CMAKE_CURRENT_SOURCE_DIR}/resources/menu_data/aqemu.desktop" )
INSTALL( FILES ${app_menu} DESTINATION share/applications )

FILE( GLOB app_menu "${CMAKE_CURRENT_SOURCE_DIR}/resources/menu_data/aqemu.appdata.xml" )
INSTALL( FILES ${app_menu} DESTINATION share/appdata )

# Create docs .bz2 files
MACRO( create_docs _sources )
	FOREACH( curFile ${ARGN} )
		GET_FILENAME_COMPONENT( _in ${curFile} ABSOLUTE )
		GET_FILENAME_COMPONENT( _basename ${curFile} NAME )
		
		SET( _out ${CMAKE_CURRENT_SOURCE_DIR}/${_basename}.bz2 )
		
		ADD_CUSTOM_COMMAND( OUTPUT ${_out} COMMAND bzip2 ARGS -zkf9 ${_in} DEPENDS ${_in} )
		
		SET( ${_sources} ${${_sources}} ${_out} )
	ENDFOREACH( curFile )
ENDMACRO( create_docs )

IF( NOT WIN32 )
	CREATE_DOCS( docs_bz2 AUTHORS CHANGELOG README.md TODO )
	ADD_CUSTOM_TARGET( docs ALL DEPENDS ${docs_bz2} )
	INSTALL( FILES ${docs_bz2} DESTINATION share/doc/aqemu )
ENDIF( NOT WIN32 )

# Compress and install MAN page file
IF( NOT MAN_PAGE_COMPRESSOR )
	MESSAGE( STATUS "MAN page will be installed is not packed." )
ELSEIF( MAN_PAGE_COMPRESSOR STREQUAL "gzip" )
	SET( COMPRESSOR_SUFFIX gz )
ELSEIF( MAN_PAGE_COMPRESSOR STREQUAL "bzip2" )
	SET( COMPRESSOR_SUFFIX bz2 )
ELSEIF( MAN_PAGE_COMPRESSOR STREQUAL "lzma" )
	SET( COMPRESSOR_SUFFIX lzma )
ELSE( NOT MAN_PAGE_COMPRESSOR )
	MESSAGE( STATUS "${MAN_PAGE_COMPRESSOR} not supported!" )
	MESSAGE( FATAL_ERROR "Correct values: gzip, bzip2, lzma" )
ENDIF( NOT MAN_PAGE_COMPRESSOR )

# Compress MAN file?
IF( INSTALL_MAN )
	IF( COMPRESSOR_SUFFIX )
		# Find gzip, bzip or lzma
		FIND_PROGRAM( COMPRESSOR_BIN NAMES ${MAN_PAGE_COMPRESSOR} )
		
		IF( ${COMPRESSOR_BIN} MATCHES COMPRESSOR_BIN-NOTFOUND )
			MESSAGE( STATUS "${MAN_PAGE_COMPRESSOR} - NOT FOUND!" )
			MESSAGE( FATAL_ERROR "Please install ${MAN_PAGE_COMPRESSOR} package!" )
		ENDIF( ${COMPRESSOR_BIN} MATCHES COMPRESSOR_BIN-NOTFOUND )
		
		# Compress
		EXECUTE_PROCESS( OUTPUT_FILE ${aqemu_man_page}.${COMPRESSOR_SUFFIX}
						COMMAND "${COMPRESSOR_BIN}" -9
						INPUT_FILE ${aqemu_man_page}
						WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} )
		
		# Install MAN
		INSTALL( FILES ${CMAKE_SOURCE_DIR}/${aqemu_man_page}.${COMPRESSOR_SUFFIX} DESTINATION share/man/man1 )
	ELSE( COMPRESSOR_SUFFIX )
		MESSAGE( STATUS "MAN_PAGE_COMPRESSOR flag not set. Available values: gzip, bzip2, lzma" )
		MESSAGE( STATUS "Will be installed not compressed MAN pages" )
		
	ENDIF( COMPRESSOR_SUFFIX )
ENDIF( INSTALL_MAN )

# CPack Packaging Configuration
SET( CPACK_GENERATOR "DEB" )
SET( CPACK_PACKAGE_NAME "aqemu" )
SET( CPACK_PACKAGE_VERSION "0.9.8" )
SET( CPACK_PACKAGE_CONTACT "chronic8000" )
SET( CPACK_PACKAGE_DESCRIPTION_SUMMARY "AQEMU is a graphical user interface for the QEMU emulator." )
SET( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON )
INCLUDE( CPack )


