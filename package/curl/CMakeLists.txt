cmake_minimum_required(VERSION 2.6)
find_library(HAVE_LIBCURL NAMES curl)
if(HAVE_LIBCURL)
	set(ExtraLibs ${ExtraLibs} ${HAVE_LIBCURL})
	set(PACKAGE_EXTRA_LIBRARY ${ExtraLibs})
	set(PACKAGE_SOURCE_CODE curl_glue.c)
	set(PACKAGE_SCRIPT_CODE curl_glue.k)
	add_konoha_package(curl)
endif(HAVE_LIBCURL)
