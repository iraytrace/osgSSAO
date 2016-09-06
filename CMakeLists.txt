project( ssao )
cmake_minimum_required(VERSION 3.2)
message(STATUS ${CMAKE_CURRENT_SOURCE_DIR})

# Allow all sub CMakeLists to use our custom distributed CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_SOURCE_DIR}/misc/CMake"
  "${CMAKE_SOURCE_DIR}/misc/CMake/compiler"
)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# target build locations

find_package(OpenSceneGraph 3.4.0 COMPONENTS
			    osg osgDB osgViewer osgGA osgUtil OpenThreads )
if (OPENSCENEGRAPH_FOUND)
  include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS})
else (OPENSCENEGRAPH_FOUND)
  message(STATUS "----- You probably want to get OpenSceneGraph set up properly ------")
endif (OPENSCENEGRAPH_FOUND)

# if cmake cannot find Qt you can set Qt5_DIR or uncomment next line
# set(Qt5_DIR "C:/Qt/5.5/msvc2013_64/lib/cmake/Qt5" CACHE PATH "Qt cmake dir")
find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL REQUIRED)

# make all sources part of the build
file(GLOB product_SRCS
    "${CMAKE_CURRENT_LIST_DIR}/*cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*h" )

# Make all *.ui files part of the build
file(GLOB product_UIS "${CMAKE_CURRENT_LIST_DIR}/*ui")

# Make all resource (.qrc) part of the build
file(GLOB product_RCS "${CMAKE_CURRENT_LIST_DIR}/*.qrc")

# necessary to pick up uic headers
include_directories(${PROJECT_BINARY_DIR})

include_directories("${CMAKE_SOURCE_DIR}/src/lib/applib")

# Qt headers
include_directories(
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Gui_INCLUDE_DIRS}
  ${Qt5Widgets_INCLUDE_DIRS}
  ${Qt5OpenGL_INCLUDE_DIRS}
  )

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt5_wrap_ui(UIS ${product_UIS})
qt5_add_resources( RCS ${product_RCS})

# the list of dependencies is needed both for linking
# and for the install step.  So make the list once
# so we are sure to use the same list in both places
set(product_LIBS
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::OpenGL
    ${OPENSCENEGRAPH_LIBRARIES}
)

set(EXEC_NAME "ssao")
add_executable(${EXEC_NAME} ${product_SRCS} ${UIS} ${RCS} )
target_link_libraries(${EXEC_NAME} ${product_LIBS})

install(TARGETS ${EXEC_NAME}
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime )

#
# gather up dependencies for installation. This works as follows:
#
# INSTALL(CODE      CMake code to have build system run at install time
# "		    begin content of install-time CMake code
# include(BundleUtilities)	  load definition of fixup_bundle
# fixup_bundle( executable pluginpaths librarypaths )
#
# executable: executable to be examined for library dependencies.
#   ---------- WARNING -------------------------------------------------------
#   CPack runs CMake on the code provided when you make the PACKAGE target
#   (eg. % make PACKAGE).  IT USES A DIFFERENT INSTALL DIRECTORY THAN THE ONE
#   YOU PROVIDE AT CMAKE TIME.  Therefore you want to defer evaluation of the
#   variable CMAKE_INSTALL_PREFIX to CPack (or make PACKAGE) time to get the
#   right value.  On the other hand ${EXEC_NAME} wants to be evaluated at
#   at the original CMake time.
#
# pluginpaths:  It is ok to evaluate $ENV{PATH} at the original CMake time.  
#   You probably want to create a list you can append other paths to.  
#   Converting to CMAKE path separators seems to be a good idea to prevent
#   Windows back-slashes from being mis-interpreted as escape chars.
#
# librarypaths:  It is ok to evaluate $ENV{PATH} at the original CMake time.  
#   You probably want to create a list you can append other paths to.  
#   Converting to CMAKE path separators seems to be a good idea to prevent
#   Windows back-slashes from being mis-interpreted as escape chars.
#   
file(TO_CMAKE_PATH "$ENV{PATH}" DIRS) # directories to search for prerequisites
# optional:  list(append DIRS "anotherDir")

INSTALL(CODE "FILE(COPY \"${OSG_INCLUDE_DIR}/../bin/osgPlugins-3.4.0\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
	FILE(GLOB QTPLUGINS \"${Qt5_DIR}/../../../plugins/*\")
	FILE(COPY \${QTPLUGINS} DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
	FILE(COPY \"${Qt5_DIR}/../../../bin/libEGL.dll\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
	file(GLOB_RECURSE VSLPLUGINS \"\${CMAKE_INSTALL_PREFIX}/bin/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
	include(BundleUtilities)
	fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/bin/${EXEC_NAME}${CMAKE_EXECUTABLE_SUFFIX}\" \"\${VSLPLUGINS}\" \"${DIRS}\")
	" COMPONENT Runtime)
