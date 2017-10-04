cmake_minimum_required (VERSION 2.8)
cmake_policy(VERSION 2.8)

if(UNIX AND NOT APPLE)
    set(NGXCH_LINUX TRUE)
else(UNIX AND NOT APPLE)
    set(NGXCH_MACOSX TRUE)
    set(NGXCH_CMAKE_MACOSX_RPATH 1)
endif (UNIX AND NOT APPLE)

set(PROJ_NGXCH ngxch)
project(ngxch C)

# Some tweak parameters
SET(PWD ${CMAKE_CURRENT_SOURCE_DIR})

IF (NOT DEFINED CMAKE_INSTALL_BINDIR)
SET(CMAKE_INSTALL_BINDIR /usr/local/bin)
ENDIF (NOT DEFINED CMAKE_INSTALL_BINDIR)

IF (NOT DEFINED CMAKE_INSTALL_LIBDIR)
SET(CMAKE_INSTALL_LIBDIR /usr/local/lib)
ENDIF (NOT DEFINED CMAKE_INSTALL_LIBDIR)

IF (NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
SET(CMAKE_INSTALL_INCLUDEDIR /usr/local/include/ngxch)
ENDIF(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)

MESSAGE(STATUS "ngxch LIBRARY DIRECTORY located at ${CMAKE_INSTALL_LIBDIR}")
message(STATUS "Current SOURCES DIRECTORY located at ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Current Binary DIRECTORY located at ${CMAKE_CURRENT_BINARY_DIR}")

SET(LIBPATH /usr/lib /usr/local/lib)

include_directories(binary_array atomic_hashtable include /usr/local/include)

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

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wstrict-prototypes -Wmissing-prototypes")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wshadow -Wpointer-arith -Wcast-qual")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wsign-compare -Iinclude -std=c11 -pedantic")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

# Not need to export dynamic symbol out, -rdyanmic only for user application
# SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-rdynamic") # LDFLAGS=
#  gcc -fPIC -pie -o libngxch.so src/ch_core.c src/ch_pool.c src/ch_json.c src/ch_cli.c -Iinclude -Wl,-E -Wl,--whole-archive -lfcgi -ldl -Wl,--no-whole-archive

if(UNIX AND NOT APPLE)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pie -Wl,-E")
else(UNIX AND NOT APPLE)
add_definitions(-DNGXCH_MACOSX=true)
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

SET (ATOMIC_HASH_DIR atomic_hashtable)
SET (BIN_ARRAY_DIR binary_array)

SET (SRCS ${SRCDIR}/ch_core.c ${SRCDIR}/ch_pool.c ${SRCDIR}/ch_hash.c ${SRCDIR}/ch_map.c ${BIN_ARRAY_DIR}/binary_array.c ${ATOMIC_HASH_DIR}/atomic_hashtable.c ${ATOMIC_HASH_DIR}/atomic_hashtable_n.c ${SRCDIR}/ch_buf.c ${SRCDIR}/ch_cli.c)
SET (HDRS ${INCLUDES}/ch_core.h ${INCLUDES}/ch_pool.h ${INCLUDES}/ch_hash.h ${INCLUDES}/ch_map.h ${BIN_ARRAY_DIR}/binary_array.h ${ATOMIC_HASH_DIR}/atomic_hashtable.h ${ATOMIC_HASH_DIR}/atomic_hashtable_n.h ${INCLUDES}/ch_buf.h)

add_library(${PROJ_NGXCH}.static STATIC ${HDRS} ${SRCS})
set_target_properties(${PROJ_NGXCH}.static PROPERTIES OUTPUT_NAME ${PROJ_NGXCH})

add_library(${PROJ_NGXCH} SHARED ${HDRS} ${SRCS})

link_directories(/usr/local/lib /usr/lib)

# add_executable(ngxch ${SRCDIR}/ch_cli.c)

# target_link_libraries(ngxch ${PROJ_NGXCH})
# target_link_libraries(ngxch fcgi dl)

if(UNIX AND NOT APPLE)
# target_link_libraries(${PROJ_NGXCH} ${LIB_DL}) #static link to fcgi
target_link_libraries(${PROJ_NGXCH} fcgi dl pthread)  #dynamic link to -ldl
# target_link_libraries(${PROJ_NGXCH}.static ${LIB_DL}) #static link 
target_link_libraries(${PROJ_NGXCH}.static fcgi dl pthread) #dynamic link to -ldl
else(UNIX AND NOT APPLE)
# target_link_libraries(${PROJ_NGXCH} ${LIB_DL}) #static link to fcgi
target_link_libraries(${PROJ_NGXCH} fcgi dl pthread)  #dynamic link to -ldl
# target_link_libraries(${PROJ_NGXCH}.static ${LIB_DL}) #static link 
target_link_libraries(${PROJ_NGXCH}.static fcgi dl pthread) #dynamic link to -ldl
endif(UNIX AND NOT APPLE)


install(TARGETS ${PROJ_NGXCH} EXPORT ${PROJ_NGXCH}
		ARCHIVE DESTINATION lib${LIB_SUFFIX}
		LIBRARY DESTINATION lib${LIB_SUFFIX}
		# RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		)
install(TARGETS ${PROJ_NGXCH}.static EXPORT ${PROJ_NGXCH}
		ARCHIVE DESTINATION lib${LIB_SUFFIX}
		LIBRARY DESTINATION lib${LIB_SUFFIX}
		# RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		)
install(FILES ${INCLUDES}/ch_core.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/ch_hash.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/ch_map.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/ch_buf.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/ch_pool.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${BIN_ARRAY_DIR}/binary_array.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${ATOMIC_HASH_DIR}/atomic_hashtable.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${ATOMIC_HASH_DIR}/atomic_hashtable_n.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# install(TARGETS ngxch EXPORT ngxch DESTINATION ${CMAKE_INSTALL_BINDIR})
install(CODE "execute_process(COMMAND ldconfig)")

# IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
# 	add_definitions(-D__DEBUG__)
# 	SET(CMAKE_C_FLAGS "-Wall -O0 -fPIC ${CMAKE_C_FLAGS}")
# 	SET(CMAKE_CXX_FLAGS "-Wall -O0 -fPIC ${CMAKE_CXX_FLAGS}")
# ELSE (${CMAKE_BUILD_TYPE} MATCHES "Debug")
# 	SET(CMAKE_C_FLAGS "-Wall -O2 -fPIC ${CMAKE_C_FLAGS}")
# 	SET(CMAKE_CXX_FLAGS "-Wall -O2 -fPIC ${CMAKE_CXX_FLAGS}")
# ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")

