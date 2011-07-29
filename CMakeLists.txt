cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include (version.cmake)

message (STATUS "*** Building xsubInsertion ${xsubInsertion_VERSION} ***")

PROJECT(xsubInsertion)

find_library(libfreetype.so REQUIRED)

configure_file (config.h.cmake config.h)

add_subdirectory (src) 

set (CPACK_GENERATOR "TGZ")
set (CPACK_PACKAGE_VERSION_MAJOR ${xsubInsertion_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${xsubInsertion_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${xsubInsertion_MAJOR})
set (CPACK_SOURCE_GENERATOR "TBZ2")
set (CPACK_SOURCE_PACKAGE_FILE_NAME xsubInsertion-${xsubInsertion_VERSION})
set (CPACK_SOURCE_IGNORE_FILES "~$" ".bz2$" ".gz$")
include (CPack)
