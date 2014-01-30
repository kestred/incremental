# Filename: Interrogate.cmake
#
# Description: This file contains macros and functions that are used to invoke
#   interrogate, to generate wrappers for Python and/or other languages.
#
# Functions:
#   target_interrogate(target [ALL] [source1 [source2 ...]])
#   add_python_module(module [lib1 [lib2 ...]])
#

set(IGATE_FLAGS ${INTERROGATE_OPTIONS} -DCPPPARSER -D__cplusplus -Dvolatile -Dmutable)

if(WIN32)
  list(APPEND IGATE_FLAGS -longlong __int64 -D_X86_ -D__STDC__=1 -DWIN32_VC -D "_declspec(param)=" -D "__declspec(param)=" -D_near -D_far -D__near -D__far -D_WIN32 -D__stdcall -DWIN32)
endif()
if(INTERROGATE_VERBOSE)
  list(APPEND IGATE_FLAGS "-v")
endif()

set(IMOD_FLAGS ${INTERROGATE_MODULE_OPTIONS})

if(INTERROGATE_PYTHON_INTERFACE AND PYTHON_NATIVE)
  list(APPEND IGATE_FLAGS -python-native)
  list(APPEND IMOD_FLAGS -python-native)
endif()

# This is a list of regexes that are applied to every filename. If one of the
# regexes matches, that file will not be passed to Interrogate.
set(INTERROGATE_EXCLUDE_REGEXES
  ".*\\.I$"
  ".*\\.N$"
  ".*_src\\..*")

#
# Function: target_interrogate(target [ALL] [source1 [source2 ...]])
# Currently, this adds the resultant TARGET_igate.cxx to the target by linking
# it as a library. This is only temporary until the codebase is cleaned up a
# bit more to reduce the dependency on interrogate.
# If ALL is specified, all of the sources from the associated
# target are added.
#
function(target_interrogate target)
  if(HAVE_PYTHON AND HAVE_INTERROGATE)
    set(sources)
    set(want_all OFF)

    # Find any .N files that would normally be picked up by interrogate.
    # We let CMake add these as dependencies too, to allow rebuilding
    # the wrappers when the .N files have been modified.
    set(deps)
    foreach(arg ${ARGV})
      if(arg STREQUAL "ALL")
        set(want_all ON)
      else()
        list(APPEND sources "${source}")
      endif()
    endforeach()

    # If ALL was specified, pull in all sources from the target.
    if(want_all)
      get_target_property(target_sources "${target}" SOURCES)
      list(APPEND sources ${target_sources})
    endif()

    list(REMOVE_DUPLICATES sources)

    # Also remove the regex-blacklisted filenames from sources:
    foreach(source ${sources})
      foreach(regex ${INTERROGATE_EXCLUDE_REGEXES})
        if("${source}" MATCHES "${regex}")
          list(REMOVE_ITEM sources "${source}")
        endif()
      endforeach(regex)
    endforeach(source)

    # Go through the sources to determine the full name,
    # and also find out if there are any .N files to pick up.
    foreach(source ${sources})
      get_source_file_property(exclude "${source}" WRAP_EXCLUDE)

      if(NOT exclude)
        get_filename_component(basename "${source}" NAME_WE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${basename}.N")
          list(APPEND deps "${CMAKE_CURRENT_SOURCE_DIR}/${basename}.N")
        endif()

        # Add the full path to the source file itself.
        get_source_file_property(location "${source}" LOCATION)
        list(APPEND deps "${location}")
      endif()
    endforeach(source)

    # Interrogate also needs to know the include paths of the current module,
    # so we'll run over the include directories property and add those to the
    # interrogate command line.
    set(igate_includes)
    get_target_property(target_includes "${target}" INTERFACE_INCLUDE_DIRECTORIES)
    foreach(include_directory ${target_includes})
      set(igate_includes
        ${igate_includes} -I "${include_directory}")
    endforeach(include_directory)

    set(igate_database "${CMAKE_CURRENT_BINARY_DIR}/${target}.in")
    add_custom_command(
      OUTPUT "${target}_igate.cxx" "${igate_database}"
      COMMAND interrogate
        -od "${igate_database}"
        -oc "${target}_igate.cxx"
        -module $<TARGET_PROPERTY:${target},INTERROGATE_MODULE>
        -library ${target} ${IGATE_FLAGS}
        -srcdir "${CMAKE_CURRENT_SOURCE_DIR}"
        -S "${PROJECT_SOURCE_DIR}/dtool/src/parser-inc"
        -S "${PROJECT_BINARY_DIR}/include/parser-inc"
        -S "${PROJECT_BINARY_DIR}/include"
        -I '$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>'
        ${sources}
      DEPENDS interrogate ${deps}
      COMMENT "Interrogating ${target}"
    )

    # Now that we've interrogated, let's associate the interrogate sources to
    # the target:
    set(igate_sources "${target}_igate.cxx")
    # Also add all of the _ext sources:
    foreach(source ${sources})
      if("${source}" MATCHES ".*_ext.*")
        set(igate_sources ${igate_sources} "${source}")
      endif()
    endforeach(source)

    # Now record INTERROGATE_SOURCES and INTERROGATE_DATABASE to the target:
    set_target_properties("${target}" PROPERTIES INTERROGATE_SOURCES ${igate_sources})
    set_target_properties("${target}" PROPERTIES INTERROGATE_DATABASE "${igate_database}")
    # This gets set by add_python_module later on:
    set_target_properties("${target}" PROPERTIES INTERROGATE_MODULE "NONE")

    # HACK: This is REALLY ugly, but we can't add the _igate.cxx to the existing
    # target (or at least, when I tried, it ignored the additional file), so as
    # a (very!) temporary workaround, add another library and link it in.
    add_library("${target}_igate" ${igate_sources})
    get_target_property(target_libraries "${target}" INTERFACE_LINK_LIBRARIES)
    target_link_libraries("${target}_igate" ${target_libraries})
    target_link_libraries(${target} ${target}_igate)

  endif()
endfunction(target_interrogate)

#
# Function: add_python_module(module [lib1 [lib2 ...]])
# Uses interrogate to create a Python module.
#
function(add_python_module module)
  if(HAVE_PYTHON AND HAVE_INTERROGATE)
    set(targets ${ARGN})
    set(databases)

    foreach(target ${targets})
      # Add the target's .in (database) to what we're going to pass to
      # interrogate_module...
      get_target_property(target_database "${target}" INTERROGATE_DATABASE)
      list(APPEND databases "${target_database}")

      # Also set the target's INTERROGATE_MODULE property so that the target's
      # interrogate is invoked with our name as the module:
      set_target_properties("${target}" PROPERTIES INTERROGATE_MODULE "${module}")
    endforeach(target)

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${module}_module.cxx"
      COMMAND interrogate_module
        -oc "${CMAKE_CURRENT_BINARY_DIR}/${module}_module.cxx"
        -module ${module} -library ${module}
        ${IMOD_FLAGS} ${databases}
      DEPENDS interrogate_module ${databases}
    )

    add_library(${module} MODULE "${module}_module.cxx")
    target_link_libraries(${module} ${PYTHON_LIBRARIES})
    target_link_libraries(${module} p3interrogatedb)
    foreach(target ${targets})
      target_link_libraries(${module} ${target})

      # This is part of the aforementioned HACK:
      target_link_libraries(${module} ${target}_igate)
    endforeach(target)

    set_target_properties(${module} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/panda3d"
      PREFIX ""
    )
    if(WIN32 AND NOT CYGWIN)
      set_target_properties(${module} PROPERTIES SUFFIX ".pyd")
    endif()
  endif()
endfunction(add_python_module)
