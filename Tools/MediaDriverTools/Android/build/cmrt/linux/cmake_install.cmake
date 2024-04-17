# Install script for directory: /media1/celadon_u/hardware/intel/external/media/mod/cmrtlib/linux

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xmediax" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/media1/celadon_u/hardware/intel/external/media/mod/Tools/MediaDriverTools/Android/build/cmrt/linux/libigfxcmrt.so.7.2.0"
    "/media1/celadon_u/hardware/intel/external/media/mod/Tools/MediaDriverTools/Android/build/cmrt/linux/libigfxcmrt.so.7"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libigfxcmrt.so.7.2.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libigfxcmrt.so.7"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xmediax" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/media1/celadon_u/hardware/intel/external/media/mod/Tools/MediaDriverTools/Android/build/cmrt/linux/libigfxcmrt.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libigfxcmrt.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libigfxcmrt.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libigfxcmrt.so")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xmediax" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/igfxcmrt" TYPE FILE FILES
    "/media1/celadon_u/hardware/intel/external/media/mod/cmrtlib/linux/../agnostic/share/cm_rt.h"
    "/media1/celadon_u/hardware/intel/external/media/mod/cmrtlib/linux/../agnostic/share/cm_rt_g8.h"
    "/media1/celadon_u/hardware/intel/external/media/mod/cmrtlib/linux/../agnostic/share/cm_rt_g9.h"
    "/media1/celadon_u/hardware/intel/external/media/mod/cmrtlib/linux/../agnostic/share/cm_rt_g10.h"
    "/media1/celadon_u/hardware/intel/external/media/mod/cmrtlib/linux/../agnostic/share/cm_rt_g11.h"
    "/media1/celadon_u/hardware/intel/external/media/mod/cmrtlib/linux/../agnostic/share/cm_rt_g12_tgl.h"
    "/media1/celadon_u/hardware/intel/external/media/mod/cmrtlib/linux/../agnostic/share/cm_rt_g12_dg1.h"
    "/media1/celadon_u/hardware/intel/external/media/mod/cmrtlib/linux/../agnostic/share/cm_hw_vebox_cmd_g10.h"
    "/media1/celadon_u/hardware/intel/external/media/mod/cmrtlib/linux/share/cm_rt_def_os.h"
    "/media1/celadon_u/hardware/intel/external/media/mod/cmrtlib/linux/share/cm_rt_api_os.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xmediax" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/igfxcmrt" TYPE FILE FILES "/media1/celadon_u/hardware/intel/external/media/mod/Tools/MediaDriverTools/Android/build/cmrt/linux/cm_rt_extension.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xmediax" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/media1/celadon_u/hardware/intel/external/media/mod/Tools/MediaDriverTools/Android/build/cmrt/linux/igfxcmrt.pc")
endif()

