# Filename: Interrogate.cmake
#
# Description: This file contains macros and functions that are used to invoke
#   interrogate, to generate wrappers for Python and/or other languages.
#
# Functions:
#   target_interrogate(target [ALL] [source1 [source2 ...]])
#   add_python_module(module [lib1 [lib2 ...]])
#

set(IGATE_FLAGS ${INTERROGATE_OPTIONS} -DCPPPARSER -D__cplusplus -Dvolatile -Dmutable -python-native)

# This is a list of regexes that are applied to every filename. If one of the
# regexes matches, that file will not be passed to Interrogate.
set(INTERROGATE_EXCLUDE_REGEXES
  ".*\\.I$"
  ".*\\.N$"
  ".*_src\\..*")

if(WIN32)
  list(APPEND IGATE_FLAGS -longlong __int64 -D_X86_ -D__STDC__=1 -DWIN32_VC -D "_declspec(param)=" -D "__declspec(param)=" -D_near -D_far -D__near -D__far -D_WIN32 -D__stdcall -DWIN32)
endif()
if(INTERROGATE_VERBOSE)
  list(APPEND IGATE_FLAGS "-v")
endif()

set(IMOD_FLAGS ${INTERROGATE_MODULE_OPTIONS} -python-native)


#
# Function: target_interrogate(target [ALL] [source1 [source2 ...]])
# NB. This doesn't actually invoke interrogate, but merely adds the
# sources to the list of scan sources associated with the target.
# Interrogate will be invoked when add_python_module is called.
# If ALL is specified, all of the sources from the associated
# target are added.
#
function(target_interrogate target)
  if(HAVE_PYTHON AND HAVE_INTERROGATE)
    set(sources)
    set(want_all OFF)
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

    # Now let's get everything's absolute path, so that it can be passed
    # through a property while still preserving the reference.
    set(absolute_sources)
    foreach(source ${sources})
      get_source_file_property(location "${source}" LOCATION)
      set(absolute_sources ${absolute_sources} ${location})
    endforeach(source)

    set_target_properties("${target}" PROPERTIES IGATE_SOURCES
      "${absolute_sources}")

    # CMake has no property for determining the source directory where the
    # target was originally added. interrogate_sources makes use of this
    # property (if it is set) in order to make all paths on the command-line
    # relative to it, thereby shortening the command-line even more.
    # Since this is not an Interrogate-specific property, it is not named with
    # an IGATE_ prefix.
    set_target_properties("${target}" PROPERTIES TARGET_SRCDIR
      "${CMAKE_CURRENT_SOURCE_DIR}")

    # HACK HACK HACK -- this is part of the below hack.
    target_link_libraries(${target} ${target}_igate)
  endif()
endfunction(target_interrogate)

#
# Function: interrogate_sources(target output database module)
#
# This function actually runs a component-level interrogation against 'target'.
# It generates the outfile.cxx (output) and dbfile.in (database) files, which
# can then be used during the interrogate_module step to produce language
# bindings.
#
# The target must first have had sources selected with target_interrogate.
# Failure to do so will result in an error.
#
function(interrogate_sources target output database module)
  if(HAVE_PYTHON AND HAVE_INTERROGATE)
    get_target_property(sources "${target}" IGATE_SOURCES)

    if(NOT sources)
      message(FATAL_ERROR
        "Cannot interrogate ${target} unless it's run through target_interrogate first!")
    endif()

    get_target_property(srcdir "${target}" TARGET_SRCDIR)
    if(NOT srcdir)
      # No TARGET_SRCDIR was set, so we'll do everything relative to our
      # current binary dir instead:
      set(srcdir "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    set(scan_sources)
    foreach(source ${sources})
      get_filename_component(source_basename "${source}" NAME)

      # Only certain sources should actually be scanned by Interrogate. The
      # rest are merely dependencies. This uses the exclusion regex above in
      # order to determine what files are okay:
      set(exclude OFF)
      foreach(regex ${INTERROGATE_EXCLUDE_REGEXES})
        if("${source_basename}" MATCHES "${regex}")
          set(exclude ON)
        endif()
      endforeach(regex)

      get_source_file_property(source_excluded ${source} WRAP_EXCLUDE)
      if(source_excluded)
        set(exclude ON)
      endif()

      if(NOT exclude)
        # This file is to be scanned by Interrogate. In order to avoid
        # cluttering up the command line, we should first make it relative:
        file(RELATIVE_PATH rel_source "${srcdir}" "${source}")
        list(APPEND scan_sources "${rel_source}")
      endif()
    endforeach(source)

    # Interrogate also needs the include paths, so we'll extract them from the
    # target:
    set(include_flags)
    get_target_property(include_dirs "${target}" INTERFACE_INCLUDE_DIRECTORIES)
    foreach(include_dir ${include_dirs})
      # To keep the command-line small, also make this relative:
      # Note that Interrogate does NOT handle -I paths relative to -srcdir, so
      # we make them relative to the directory where it's invoked.
      file(RELATIVE_PATH rel_include_dir "${CMAKE_CURRENT_BINARY_DIR}" "${include_dir}")
      list(APPEND include_flags "-I${rel_include_dir}")
    endforeach(include_dir)
    # The above must also be included when compiling the resulting _igate.cxx file:
    include_directories(${include_dirs})


    add_custom_command(
      OUTPUT "${output}" "${database}"
      COMMAND interrogate
        -oc "${output}"
        -od "${database}"
        -srcdir "${srcdir}"
        -module ${module} -library ${target} ${IGATE_FLAGS}
        -S "${PROJECT_BINARY_DIR}/include"
        -S "${PROJECT_SOURCE_DIR}/dtool/src/parser-inc"
        -S "${PROJECT_BINARY_DIR}/include/parser-inc"
        ${include_flags}
        ${scan_sources}
      DEPENDS interrogate ${sources}
      COMMENT "Interrogating ${target}"
    )
  endif()
endfunction(interrogate_sources)

#
# Function: add_python_module(module [lib1 [lib2 ...]])
# Uses interrogate to create a Python module.
#
function(add_python_module module)
  if(HAVE_PYTHON AND HAVE_INTERROGATE)
    set(targets ${ARGN})
    set(infiles)
    set(sources)
    set(HACKlinklibs)

    foreach(target ${targets})
      interrogate_sources(${target} "${target}_igate.cxx" "${target}.in" "${module}")
      list(APPEND infiles "${target}.in")
      #list(APPEND sources "${target}_igate.cxx")

      # HACK HACK HACK:
      # Currently, the codebase has dependencies on the Interrogate-generated
      # code when HAVE_PYTHON is enabled. rdb is working to remove this, but
      # until then, the generated code must somehow be made available to the
      # modules themselves. The workaround here is to put the _igate.cxx into
      # its own micro-library, which is linked both here on the module and
      # against the component library in question.
      add_library(${target}_igate ${target}_igate.cxx)
      list(APPEND HACKlinklibs "${target}_igate")

      get_target_property(target_links "${target}" INTERFACE_LINK_LIBRARIES)
      target_link_libraries(${target}_igate ${target_links})
    endforeach(target)

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${module}_module.cxx"
      COMMAND interrogate_module
        -oc "${CMAKE_CURRENT_BINARY_DIR}/${module}_module.cxx"
        -module ${module} -library ${module}
        ${IMOD_FLAGS} ${infiles}
      DEPENDS interrogate_module ${infiles}
      COMMENT "Generating module ${module}"
    )

    add_library(${module} MODULE "${module}_module.cxx" ${sources})
    target_link_libraries(${module}
      ${targets} ${PYTHON_LIBRARIES} p3interrogatedb)
    target_link_libraries(${module} ${HACKlinklibs})

    set_target_properties(${module} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/panda3d"
      PREFIX ""
    )
    if(WIN32 AND NOT CYGWIN)
      set_target_properties(${module} PROPERTIES SUFFIX ".pyd")
    endif()
  endif()
endfunction(add_python_module)
