cmake_minimum_required(VERSION 3.19)
project(Project LANGUAGES CXX)
find_package( VTK REQUIRED )

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(Project
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    ModelPart.cpp ModelPart.h ModelPartList.cpp ModelPartList.h
    icons/fileopen.png icons/filesave.png icons/help.png icons/print.png icons/startVR.png icons/stopVR.png
    icons.qrc
    OptionDialog.ui
    OptionDialog.h
    OptionDialog.cpp
VRRenderThread.cpp
VRRenderThread.h
)

target_link_libraries(Project
    PRIVATE
        Qt::Core
        Qt::Widgets
        ${VTK_LIBRARIES}
)

vtk_module_autoinit(
    TARGETS Project
    MODULES ${VTK_LIBRARIES}
)

include(GNUInstallDirs)

install(TARGETS Project
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Project
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

set(CMAKE_RCC ON)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    ModelPart.cpp
    ModelPart.h
    ModelPartList.cpp
    ModelPartList.h
    icons.qrc
    optiondialog.cpp
    optiondialog.h
    optiondialog.ui
)

add_custom_target( VRBindings )
add_custom_command(TARGET Project POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/vrbindings
    $<TARGET_FILE_DIR:Project>/vrbindings)

add_custom_command(TARGET Project POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/skybox
    $<TARGET_FILE_DIR:Project>/skybox)
if(WIN32)
	set(CPACK_GENERATOR "NSIS")
else()
	set(CPACK_GENERATOR "ZIP")
endif()

set(CPACK_PACKAGE_NAME "Group 11 VR Work")
set(CPACK_PACKAGE_VENDOR "University of Nottingham")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "VR Visualization App")
set(CPACK_PACKAGE_FILE_NAME "Group11VRWork-1.0.0")

include(CPack)