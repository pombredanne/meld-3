CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

PROJECT (meld)

INCLUDE(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
IF(COMPILER_SUPPORTS_CXX11)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ELSE(COMPILER_SUPPORTS_CXX11)
    MESSAGE(STATUS "${CMAKE_CXX_COMPILER} does not support C++11.")
ENDIF(COMPILER_SUPPORTS_CXX11)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

ADD_DEFINITIONS(-DPREFIX="${CMAKE_INSTALL_PREFIX}")

FIND_PACKAGE(Boost COMPONENTS system filesystem REQUIRED)
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(GTKMM gtkmm-3.0 REQUIRED)
LINK_DIRECTORIES(${GTKMM_LIBRARY_DIRS})
INCLUDE_DIRECTORIES(${GTKMM_INCLUDE_DIRS})

PKG_CHECK_MODULES(GTKSOURCEVIEWMM gtksourceviewmm-3.0 REQUIRED)
LINK_DIRECTORIES(${GTKSOURCEVIEWMM_LIBRARY_DIRS})
INCLUDE_DIRECTORIES(${GTKSOURCEVIEWMM_INCLUDE_DIRS})

FIND_PACKAGE(GTest)

FILE(GLOB util_sources meld/util/*.cpp )
FILE(GLOB ui_sources meld/ui/*.cpp )
FILE(GLOB sources meld/*.cpp )
FILE(GLOB vc_sources meld/vc/*.cpp )

ADD_EXECUTABLE(meld
    difflib/src/difflib.h
    ${util_sources}
    ${ui_sources}
    ${sources}
    ${vc_sources}
)
TARGET_LINK_LIBRARIES(meld
    ${GTKMM_LIBRARIES}
    ${GTKSOURCEVIEWMM_LIBRARIES}
    boost_regex
    boost_system
    boost_filesystem
)

IF (GTEST_FOUND)
    ENABLE_TESTING()

    ADD_EXECUTABLE(difflibtest tests/difflibtest.cpp)
    TARGET_LINK_LIBRARIES(difflibtest gtest_main gtest boost_regex)
    ADD_TEST(NAME difflibtest WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND difflibtest)

    ADD_EXECUTABLE(filesystemtest tests/filesystemtest.cpp)
    TARGET_LINK_LIBRARIES(filesystemtest gtest_main gtest boost_filesystem boost_system)
    ADD_TEST(NAME filesystemtest WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND filesystemtest)

    ADD_EXECUTABLE(tasktest tests/tasktest.cpp meld/task.cpp meld/util/compat.cpp)
    TARGET_LINK_LIBRARIES(tasktest gtest_main gtest boost_regex boost_system boost_filesystem ${GTKMM_LIBRARIES})
    ADD_TEST(NAME tasktest WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND tasktest)

    ADD_EXECUTABLE(sigcparamtest tests/sigcparamtest.cpp)
    TARGET_LINK_LIBRARIES(sigcparamtest gtest_main gtest ${GTKMM_LIBRARIES})
    ADD_TEST(NAME sigcparamtest WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND sigcparamtest)

ENDIF (GTEST_FOUND)



install(
    FILES
        data/ui/application.ui
        data/ui/dirdiff.ui
        data/ui/EditableList.ui
        data/ui/filediff.ui
        data/ui/findbar.ui
        data/ui/meldapp.ui
        data/ui/patch-dialog.ui
        data/ui/preferences.ui
        data/ui/tab-placeholder.ui
        data/ui/vcview.ui
    DESTINATION
        "share/meld/ui/"
)
