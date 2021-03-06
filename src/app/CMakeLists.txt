include_directories("../")

set(APP_SRC
  "main.cc"
  "config.cc"
  "daemon.cc"
  "instance.cc")

# Linux/OSX/BSD/DragonFly
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
  list(APPEND APP_SRC "daemon_linux.cc")
else()
  list(APPEND APP_SRC "daemon_win32.cc")
  list(APPEND APP_SRC "win32_service.cc")
endif()

if(WITH_BINARY)
  add_executable(${APP_NAME} ${APP_SRC})
  target_link_libraries(
    ${APP_NAME}
    ${CLIENT_NAME} ${CORE_NAME}
    ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  set_target_properties(${APP_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
  if(WITH_STATIC)
    if(NOT MINGW)
      set_target_properties(${APP_NAME} PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++")
    endif()
    if(CLANG AND NOT (ARM OR ANDROID) )
      set_target_properties(${APP_NAME} PROPERTIES LINK_FLAGS "-lstdc++")
      if(FREEBSD)
        set_target_properties(${APP_NAME} PROPERTIES LINK_FLAGS "-lm -lc++")
      endif()
    endif()
    # Note: On FreeBSD 10.3-RELEASE-p4 and 11, GCC 4.9.4+/Clang 3.6+ apparently started to *silently* drop -static-libgcc -static-libstdc++ during linkage so, we add -static here
    if(FREEBSD OR MINGW)
      set_target_properties(${APP_NAME} PROPERTIES LINK_FLAGS "-static")
    endif()
  endif()
  if(MINGW)
    target_link_libraries(${APP_NAME} wsock32 iphlpapi ws2_32)
  endif()
  if(WITH_HARDENING AND CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    set_target_properties(${APP_NAME} PROPERTIES LINK_FLAGS "-z relro -z now")
  endif()
  # FindBoost pulls pthread for thread which is broken for static linking
  # (at least on Ubuntu 15.04)
  list(GET Boost_LIBRARIES -1 LAST_Boost_LIBRARIES)
  if(${LAST_Boost_LIBRARIES} MATCHES ".*pthread.*")
    list(REMOVE_AT Boost_LIBRARIES -1)
  endif()
  if(MSVC)
    install(FILES
      $<TARGET_PDB_FILE:${APP_NAME}> DESTINATION "bin" CONFIGURATIONS DEBUG)
  endif()
endif()

# vim: noai:ts=2:sw=2
