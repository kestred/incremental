# Filename: Interrogate.cmake
#
# Description: This file contains macros and functions that are used to invoke
#   interrogate, to generate wrappers for Python and/or other languages.
#
# Functions:
#   target_interrogate(target [ALL] [source1 [source2 ...]])
#   add_python_module(module [lib1 [lib2 ...]])
#

set(IGATE_FLAGS -DCPPPARSER -D__cplusplus -Dvolatile -Dmutable -python-native)

# In addition, Interrogate needs to know if this is a 64-bit build:
include(CheckTypeSize)
check_type_size(long CMAKE_SIZEOF_LONG)
if(CMAKE_SIZEOF_LONG EQUAL 8)
  list(APPEND IGATE_FLAGS "-D_LP64")
endif()


# This is a list of regexes that are applied to every filename. If one of the
# regexes matches, that file will not be passed to Interrogate.
set(INTERROGATE_EXCLUDE_REGEXES
  ".*\\.I$"
  ".*\\.N$"
  ".*\\.lxx$"
  ".*\\.yxx$"
  ".*_src\\..*")

if(WIN32)
  list(APPEND IGATE_FLAGS -longlong __int64 -D_X86_ -D__STDC__=1 -DWIN32_VC -D "_declspec(param)=" -D "__declspec(param)=" -D_near -D_far -D__near -D__far -D_WIN32 -D__stdcall -DWIN32)
endif()
if(INTERROGATE_VERBOSE)
  list(APPEND IGATE_FLAGS "-v")
endif()

set(IMOD_FLAGS -python-native)


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
    foreach(arg ${ARGN})
      if(arg STREQUAL "ALL")
        set(want_all ON)
      else()
        list(APPEND sources "${arg}")
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
    set(nfiles)
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

        # Also see if this file has a .N counterpart, which has directives
        # specific for Interrogate. If there is a .N file, we add it as a dep,
        # so that CMake will rerun Interrogate if the .N files are modified:
        get_filename_component(source_path "${source}" PATH)
        get_filename_component(source_name_we "${source}" NAME_WE)
        set(nfile "${source_path}/${source_name_we}.N")
        if(EXISTS "${nfile}")
          list(APPEND nfiles "${nfile}")
        endif()
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

    # Get the compiler definition flags. These must be passed to Interrogate
    # in the same way that they are passed to the compiler so that Interrogate
    # will preprocess each file in the same way.
    set(define_flags)
    get_target_property(target_defines "${target}" COMPILE_DEFINITIONS)
    if(target_defines)
      foreach(target_define ${target_defines})
        list(APPEND define_flags "-D${target_define}")
        # And add the same definition when we compile the _igate.cxx file:
        add_definitions("-D${target_define}")
      endforeach(target_define)
    endif()
    # If this is a release build that has NDEBUG defined, we need that too:
    string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type)
    if("${CMAKE_CXX_FLAGS_${build_type}}" MATCHES ".*NDEBUG.*")
      list(APPEND define_flags "-DNDEBUG")
    endif()

    # CMake offers no way to directly depend on the composite files from here,
    # because the composite files are created in a different directory from
    # where CMake itself is run. Therefore, we need to depend on the
    # TARGET_composite target, if it exists.
    if(TARGET ${target}_composite)
      set(sources ${target}_composite ${sources})
    endif()

    add_custom_command(
      OUTPUT "${output}" "${database}"
      COMMAND interrogate
        -oc "${output}"
        -od "${database}"
        -srcdir "${srcdir}"
        -module ${module} -library ${target}
        ${INTERROGATE_OPTIONS}
        ${IGATE_FLAGS}
        ${define_flags}
        -S "${PROJECT_BINARY_DIR}/include"
        -S "${PROJECT_SOURCE_DIR}/dtool/src/parser-inc"
        -S "${PROJECT_BINARY_DIR}/include/parser-inc"
        ${include_flags}
        ${scan_sources}
      DEPENDS interrogate ${sources} ${nfiles}
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

      get_target_property(target_links "${target}" LINK_LIBRARIES)
      target_link_libraries(${target}_igate ${target_links})
    endforeach(target)

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${module}_module.cxx"
      COMMAND interrogate_module
        -oc "${CMAKE_CURRENT_BINARY_DIR}/${module}_module.cxx"
        -module ${module} -library ${module}
        ${INTERROGATE_MODULE_OPTIONS}
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
