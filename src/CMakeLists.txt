if(UNIX)
  find_library(LIB_M m DOC "Path to libm")
  if(NOT LIB_M)
    message(FATAL_ERROR "Cannot build daysim without libm.  Please set LIB_M")
  endif()
else()
  set(LIB_M )
endif()

include_directories(common)
# create a macro to generate
macro(create_version_file version_file)
  add_custom_command(
    OUTPUT "${version_file}"
    COMMAND "${CMAKE_COMMAND}"
    -DVERSION_OUT_FILE="${version_file}"
    -DVERSION_IN_FILE="${daysim_SOURCE_DIR}/src/rt/VERSION"
    -DVERSION_GOLD="${daysim_SOURCE_DIR}/src/rt/Version.c"
    -P "${daysim_SOURCE_DIR}/src/rt/create_version.cmake"
    )
endmacro()
add_subdirectory(common)
add_subdirectory(cal)
add_subdirectory(cv)
add_subdirectory(gen)
add_subdirectory(hd)
add_subdirectory(meta)
add_subdirectory(ot)
add_subdirectory(px)
add_subdirectory(rt)
add_subdirectory(util)
add_subdirectory(daysim)
if(NOT BUILD_HEADLESS)
  # only use qtrvu if qt5 is found
  if(Qt5Widgets_DIR)
    add_subdirectory(qtrvu)
  endif()
endif()
