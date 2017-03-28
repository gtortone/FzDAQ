AC_INIT([FzDAQ], [1.5])
AC_CONFIG_SRCDIR([src/main/FzDAQ-mt.cpp])
ac_default_prefix=/opt/FzDAQ

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE

AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

AC_LANG(C++)
CXXFLAGS="-O3 -std=c++0x -Wall"
#to enable debug - run with "gdb --args ./FzController --cfg config/local-daq-topology.cfg"
#CXXFLAGS="-O3 -std=c++0x -Wall -g" 

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

m4_include([aclocal/ax_boost_base.m4])
m4_include([aclocal/ax_boost_program_options.m4])
m4_include([aclocal/ax_boost_system.m4])
m4_include([aclocal/ax_boost_thread.m4])
m4_include([aclocal/log4cpp.m4])

# mandatory libs

AX_BOOST_BASE([1.55])

CXXFLAGS="$CXXFLAGS $BOOST_CPPFLAGS"
LDFLAGS="$LDFLAGS $BOOST_LDFLAGS" 

AX_BOOST_THREAD
if test x"$ax_cv_boost_thread" != x"yes"; then
  AC_MSG_ERROR([unable to find Boost::Thread library])
else
  LIBS="$BOOST_THREAD_LIB $LIBS"
fi

AX_BOOST_SYSTEM
if test x"$ax_cv_boost_system" != x"yes"; then
  AC_MSG_ERROR([unable to find Boost::System library])
else
  LIBS="$BOOST_SYSTEM_LIB $LIBS"
fi

AX_BOOST_PROGRAM_OPTIONS
if test x"$ax_cv_boost_program_options" != x"yes"; then
  AC_MSG_ERROR([unable to find Boost::ProgramOptions library])
else
  LIBS="$BOOST_PROGRAM_OPTIONS_LIB $LIBS"
fi

AM_PATH_LOG4CPP([], [], [
  AC_MSG_ERROR([unable to find log4cpp library])
])
LIBS="$LOG4CPP_LIBS $LIBS"

PKG_CHECK_MODULES([configpp], [libconfig++] ,[], [
  AC_MSG_ERROR([unable to find libconfig++ library])
])
LIBS="$configpp_LIBS $LIBS"

PKG_CHECK_MODULES([protobuf], [protobuf] ,[], [
  AC_MSG_ERROR([unable to find Google protobuf library]) 
])
LIBS="$protobuf_LIBS $LIBS"

PKG_CHECK_MODULES([zmq], [libzmq] ,[], [
  AC_MSG_ERROR([unable to find ZMQ library]) 
])
LIBS="$zmq_LIBS $LIBS"

# optional features

AC_ARG_ENABLE([usb],
    AS_HELP_STRING([--enable-usb], [enable USB acquisition channel]))

AS_IF([test "x$enable_usb" = "xyes"], [
  PKG_CHECK_MODULES([usb], [libusb-1.0] ,[], [
    AC_MSG_ERROR([unable to find libusb library])
  ])
  AC_DEFINE([USB_ENABLED])
  LIBS="$usb_LIBS $LIBS"
])

AC_ARG_ENABLE([epics],
    AS_HELP_STRING([--enable-epics], [enable EPICS run control interface]))
AS_IF([test "x$enable_epics" = "xyes"], [
  PKG_CHECK_MODULES([epics], [epics-base] ,[], [
    AC_MSG_ERROR([unable to find EPICS library])
  ])
  AC_DEFINE([EPICS_ENABLED])
  LIBS="$epics_LIBS $LIBS"
  CXXFLAGS="$CXXFLAGS $epics_CFLAGS"

  AC_SEARCH_LIBS([dbNameToAddr], [dbCore])
  AC_SEARCH_LIBS([ca_context_create], [ca])
  AC_SEARCH_LIBS([epicsThreadSleep], [Com])
  AC_SEARCH_LIBS([pvar_func_arrInitialize], [dbRecStd])
])

AC_ARG_ENABLE([weblog],
    AS_HELP_STRING([--enable-weblog], [enable WebLog message interface]))
AS_IF([test "x$enable_weblog" = "xyes"], [
  PKG_CHECK_MODULES([weblog], [libcurl] ,[], [
    AC_MSG_ERROR([unable to find curl library])
  ])
  AC_DEFINE([WEBLOG_ENABLED])
  LIBS="$weblog_LIBS $LIBS"
])

AC_ARG_ENABLE([amqlog],
    AS_HELP_STRING([--enable-amqlog], [enable ActiveMQ log message interface]))
AS_IF([test "x$enable_amqlog" = "xyes"], [
  PKG_CHECK_MODULES([amqlog], [activemq-cpp] ,[], [
    AC_MSG_ERROR([unable to find ActiveMQ library])
  ])
  AC_DEFINE([AMQLOG_ENABLED])
  LIBS="$amqlog_LIBS $LIBS"
  CXXFLAGS="$CXXFLAGS $amqlog_CFLAGS"
])

AC_OUTPUT(Makefile)
