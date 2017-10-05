cmake_minimum_required (VERSION 2.8)
cmake_policy(VERSION 2.8)

if(UNIX AND NOT APPLE)
    set(FCGIFUNC_LINUX TRUE)
else(UNIX AND NOT APPLE)
    set(FCGIFUNC_MACOSX TRUE)
    set(FCGIFUNC_CMAKE_MACOSX_RPATH 1)
endif (UNIX AND NOT APPLE)

set(PROJ_FCGIFUNC ffunc)
project(ffunc C)

# Some tweak parameters
SET(PWD ${CMAKE_CURRENT_SOURCE_DIR})

IF (NOT DEFINED CMAKE_INSTALL_BINDIR)
SET(CMAKE_INSTALL_BINDIR /usr/local/bin)
ENDIF (NOT DEFINED CMAKE_INSTALL_BINDIR)

IF (NOT DEFINED CMAKE_INSTALL_LIBDIR)
SET(CMAKE_INSTALL_LIBDIR /usr/local/lib)
ENDIF (NOT DEFINED CMAKE_INSTALL_LIBDIR)

IF (NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
SET(CMAKE_INSTALL_INCLUDEDIR /usr/local/include/ffunc)
ENDIF(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)

MESSAGE(STATUS "ffunc LIBRARY DIRECTORY located at ${CMAKE_INSTALL_LIBDIR}")
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
#  gcc -fPIC -pie -o libffunc.so src/ffunc_core.c src/ffunc_pool.c src/ffunc_json.c src/ffunc_cli.c -Iinclude -Wl,-E -Wl,--whole-archive -lfcgi -ldl -Wl,--no-whole-archive

if(UNIX AND NOT APPLE)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pie -Wl,-E")
else(UNIX AND NOT APPLE)
add_definitions(-DFCGIFUNC_MACOSX=true)
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

SET (SRCS ${SRCDIR}/ffunc_core.c ${SRCDIR}/ffunc_pool.c ${SRCDIR}/ffunc_hash.c ${SRCDIR}/ffunc_map.c ${BIN_ARRAY_DIR}/binary_array.c ${ATOMIC_HASH_DIR}/atomic_hashtable.c ${ATOMIC_HASH_DIR}/atomic_hashtable_n.c ${SRCDIR}/ffunc_buf.c ${SRCDIR}/ffunc_cli.c)
SET (HDRS ${INCLUDES}/ffunc_core.h ${INCLUDES}/ffunc_pool.h ${INCLUDES}/ffunc_hash.h ${INCLUDES}/ffunc_map.h ${BIN_ARRAY_DIR}/binary_array.h ${ATOMIC_HASH_DIR}/atomic_hashtable.h ${ATOMIC_HASH_DIR}/atomic_hashtable_n.h ${INCLUDES}/ffunc_buf.h)

add_library(${PROJ_FCGIFUNC}.static STATIC ${HDRS} ${SRCS})
set_target_properties(${PROJ_FCGIFUNC}.static PROPERTIES OUTPUT_NAME ${PROJ_FCGIFUNC})

add_library(${PROJ_FCGIFUNC} SHARED ${HDRS} ${SRCS})

link_directories(/usr/local/lib /usr/lib)

# add_executable(ffunc ${SRCDIR}/ffunc_cli.c)

# target_link_libraries(ffunc ${PROJ_FCGIFUNC})
# target_link_libraries(ffunc fcgi dl)

if(UNIX AND NOT APPLE)
# target_link_libraries(${PROJ_FCGIFUNC} ${LIB_DL}) #static link to fcgi
target_link_libraries(${PROJ_FCGIFUNC} fcgi dl pthread)  #dynamic link to -ldl
# target_link_libraries(${PROJ_FCGIFUNC}.static ${LIB_DL}) #static link 
target_link_libraries(${PROJ_FCGIFUNC}.static fcgi dl pthread) #dynamic link to -ldl
else(UNIX AND NOT APPLE)
# target_link_libraries(${PROJ_FCGIFUNC} ${LIB_DL}) #static link to fcgi
target_link_libraries(${PROJ_FCGIFUNC} fcgi dl pthread)  #dynamic link to -ldl
# target_link_libraries(${PROJ_FCGIFUNC}.static ${LIB_DL}) #static link 
target_link_libraries(${PROJ_FCGIFUNC}.static fcgi dl pthread) #dynamic link to -ldl
endif(UNIX AND NOT APPLE)


install(TARGETS ${PROJ_FCGIFUNC} EXPORT ${PROJ_FCGIFUNC}
		ARCHIVE DESTINATION lib${LIB_SUFFIX}
		LIBRARY DESTINATION lib${LIB_SUFFIX}
		# RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		)
install(TARGETS ${PROJ_FCGIFUNC}.static EXPORT ${PROJ_FCGIFUNC}
		ARCHIVE DESTINATION lib${LIB_SUFFIX}
		LIBRARY DESTINATION lib${LIB_SUFFIX}
		# RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		)
install(FILES ${INCLUDES}/ffunc_core.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/ffunc_hash.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/ffunc_map.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/ffunc_buf.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${INCLUDES}/ffunc_pool.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${BIN_ARRAY_DIR}/binary_array.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${ATOMIC_HASH_DIR}/atomic_hashtable.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${ATOMIC_HASH_DIR}/atomic_hashtable_n.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# install(TARGETS ffunc EXPORT ffunc DESTINATION ${CMAKE_INSTALL_BINDIR})
install(CODE "execute_process(COMMAND ldconfig)")

# IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
# 	add_definitions(-D__DEBUG__)
# 	SET(CMAKE_C_FLAGS "-Wall -O0 -fPIC ${CMAKE_C_FLAGS}")
# 	SET(CMAKE_CXX_FLAGS "-Wall -O0 -fPIC ${CMAKE_CXX_FLAGS}")
# ELSE (${CMAKE_BUILD_TYPE} MATCHES "Debug")
# 	SET(CMAKE_C_FLAGS "-Wall -O2 -fPIC ${CMAKE_C_FLAGS}")
# 	SET(CMAKE_CXX_FLAGS "-Wall -O2 -fPIC ${CMAKE_CXX_FLAGS}")
# ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")

