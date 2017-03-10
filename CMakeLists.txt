cmake_minimum_required (VERSION 2.8)
cmake_policy(VERSION 2.8)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
else(UNIX AND NOT APPLE)
    set(MACOSX TRUE)
    set(CMAKE_MACOSX_RPATH 1)
endif (UNIX AND NOT APPLE)

set(PROJ_FEEDY feedy)
project(restfcgi C)

# Some tweak parameters
SET(PWD ${CMAKE_CURRENT_SOURCE_DIR})

IF (NOT DEFINED CMAKE_INSTALL_BINDIR)
SET(CMAKE_INSTALL_BINDIR /usr/local/bin)
ENDIF (NOT DEFINED CMAKE_INSTALL_BINDIR)

IF (NOT DEFINED CMAKE_INSTALL_LIBDIR)
SET(CMAKE_INSTALL_LIBDIR /usr/local/lib)
ENDIF (NOT DEFINED CMAKE_INSTALL_LIBDIR)

IF (NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
SET(CMAKE_INSTALL_INCLUDEDIR /usr/local/include/restfcgi)
ENDIF(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)

MESSAGE(STATUS "restfcgi LIBRARY DIRECTORY located at ${CMAKE_INSTALL_LIBDIR}")
message(STATUS "Current SOURCES DIRECTORY located at ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Current Binary DIRECTORY located at ${CMAKE_CURRENT_BINARY_DIR}")

SET(LIBPATH /usr/lib /usr/local/lib)

include_directories(include)

add_definitions(-D_XOPEN_SOURCE)


# add_definitions(-DFDYMEMDETECT=true)

# for static libary find first priority
# find_library(FCGI_LIB NAMES libfcgi.a PATHS ${LIBPATH})
# for dynamic library find second priority
find_library(FCGI_LIB NAMES fcgi PATHS ${LIBPATH})
if(FCGI_LIB)
	message(STATUS "fcgi found at ${FCGI_LIB}. fcgi support is compiled in.")
    add_definitions(-DFCGI=true)
else(FCGI_LIB)
	message( FATAL_ERROR "fcgi not found. Please install fcgi")
endif(FCGI_LIB)

find_library(THREAD_LIB NAMES pthread PATHS ${LIBPATH})
if(THREAD_LIB)	
    add_definitions(-DTHREAD=true)
    add_definitions(-DSYNC=true)
else(THREAD_LIB)
	message( FATAL_ERROR "pthread not found. Please install pthread")
endif(THREAD_LIB)

# find_library(CJSON_LIB NAMES libcjson.a PATHS ${LIBPATH})
find_library(CJSON_LIB NAMES cjson PATHS ${LIBPATH})
if(CJSON_LIB)
	message(STATUS "cjson found. JSON support is compiled in.")
	add_definitions(-DCJSON=true)
else(CJSON_LIB)
	message( FATAL_ERROR "cjson not found, json is not supporting, please install cjson")
endif(CJSON_LIB)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wstrict-prototypes -Wmissing-prototypes")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wshadow -Wpointer-arith -Wcast-qual")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wsign-compare -Iinclude -std=c11 -pedantic")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
# Not need to export dynamic symbol out, -rdyanmic only for user application
# SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-rdynamic") # LDFLAGS=
#  gcc -fPIC -pie -o libfeedy.so src/feedy.c src/fdy_pool.c src/fdy_json.c src/fdy_cli.c -Iinclude -Wl,-E -Wl,--whole-archive -lfcgi -ldl -lcjson -Wl,--no-whole-archive

if(UNIX AND NOT APPLE)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pie -Wl,-E")
endif(UNIX AND NOT APPLE)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

SET (INCLUDES include)
SET (SRCDIR src)

SET (SRCS ${SRCDIR}/feedy.c ${SRCDIR}/fdy_pool.c ${SRCDIR}/fdy_json.c ${SRCDIR}/fdy_hash.c ${SRCDIR}/fdy_LFHashTable.c ${SRCDIR}/fdy_array.c ${SRCDIR}/fdy_sarray.c ${SRCDIR}/fdy_buf.c ${SRCDIR}/fdy_cli.c)
SET (HDRS ${INCLUDES}/feedy.h ${INCLUDES}/fdy_pool.h ${INCLUDES}/fdy_json.h ${INCLUDES}/fdy_hash.h ${INCLUDES}/fdy_LFHashTable.h ${INCLUDES}/fdy_array.h ${INCLUDES}/fdy_sarray.h ${INCLUDES}/fdy_buf.h)

add_library(${PROJ_FEEDY}.static STATIC ${HDRS} ${SRCS})
set_target_properties(${PROJ_FEEDY}.static PROPERTIES OUTPUT_NAME ${PROJ_FEEDY})

add_library(${PROJ_FEEDY} SHARED ${HDRS} ${SRCS})

link_directories(/usr/local/lib /usr/lib)

# add_executable(restfcgi ${SRCDIR}/fdy_cli.c)

# target_link_libraries(restfcgi ${PROJ_FEEDY})
# target_link_libraries(restfcgi fcgi dl)

if(UNIX AND NOT APPLE)
# target_link_libraries(${PROJ_FEEDY} ${LIB_DL}) #static link to fcgi
target_link_libraries(${PROJ_FEEDY} fcgi dl pthread atomic)  #dynamic link to -ldl
# target_link_libraries(${PROJ_FEEDY}.static ${LIB_DL}) #static link 
target_link_libraries(${PROJ_FEEDY}.static fcgi dl pthread atomic) #dynamic link to -ldl
else(UNIX AND NOT APPLE)
# target_link_libraries(${PROJ_FEEDY} ${LIB_DL}) #static link to fcgi
target_link_libraries(${PROJ_FEEDY} fcgi dl pthread)  #dynamic link to -ldl
# target_link_libraries(${PROJ_FEEDY}.static ${LIB_DL}) #static link 
target_link_libraries(${PROJ_FEEDY}.static fcgi dl pthread) #dynamic link to -ldl
endif(UNIX AND NOT APPLE)	

if(CJSON_LIB)
# target_link_libraries(restfcgi cjson)
target_link_libraries(${PROJ_FEEDY} cjson)
target_link_libraries(${PROJ_FEEDY}.static cjson)
endif(CJSON_LIB)

install(TARGETS ${PROJ_FEEDY} EXPORT ${PROJ_FEEDY}
		ARCHIVE DESTINATION lib${LIB_SUFFIX}
		LIBRARY DESTINATION lib${LIB_SUFFIX}
		# RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		)
install(TARGETS ${PROJ_FEEDY}.static EXPORT ${PROJ_FEEDY}
		ARCHIVE DESTINATION lib${LIB_SUFFIX}
		LIBRARY DESTINATION lib${LIB_SUFFIX}
		# RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		)
install(FILES ${INCLUDES}/feedy.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/fdy_json.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/fdy_hash.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/fdy_LFHashTable.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/fdy_array.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/fdy_sarray.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/fdy_buf.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/fdy_pool.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# install(TARGETS restfcgi EXPORT restfcgi DESTINATION ${CMAKE_INSTALL_BINDIR})
install(CODE "execute_process(COMMAND ldconfig)")

# IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
# 	add_definitions(-D__DEBUG__)
# 	SET(CMAKE_C_FLAGS "-Wall -O0 -fPIC ${CMAKE_C_FLAGS}")
# 	SET(CMAKE_CXX_FLAGS "-Wall -O0 -fPIC ${CMAKE_CXX_FLAGS}")
# ELSE (${CMAKE_BUILD_TYPE} MATCHES "Debug")
# 	SET(CMAKE_C_FLAGS "-Wall -O2 -fPIC ${CMAKE_C_FLAGS}")
# 	SET(CMAKE_CXX_FLAGS "-Wall -O2 -fPIC ${CMAKE_CXX_FLAGS}")
# ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")

