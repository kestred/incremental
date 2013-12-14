#
# LocalSetup.cmake
#
# This file contains further instructions to set up the DTOOL package
# when using CMake. In particular, it creates the dtool_config.h
# file based on the user's selected configure variables.
#

include(CheckCXXSourceCompiles)
include(CheckCSourceRuns)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckTypeSize)
include(TestBigEndian)
include(TestForSTDNamespace)

# Define if we have libjpeg installed.
check_include_file_cxx(jpegint.h PHAVE_JPEGINT_H)

# Define to build video-for-linux.
check_include_file_cxx(linux/videodev2.h HAVE_VIDEO4LINUX)

# Check if this is a big-endian system.
test_big_endian(WORDS_BIGENDIAN)

# Check if the compiler supports namespaces.
set(HAVE_NAMESPACE ${CMAKE_STD_NAMESPACE})

# Define if fstream::open() accepts a third parameter for umask.
#TODO make test case
#$[cdefine HAVE_OPEN_MASK]

# Define if we have lockf().
#TODO make test case
set(HAVE_LOCKF 1)

# Check if we have a wchar_t type.
check_type_size(wchar_t HAVE_WCHAR_T)

# Check if we have a wstring type.
check_cxx_source_compiles("
#include <string>
std::wstring str;
int main(int argc, char *argv[]) { return 0; }
" HAVE_WSTRING)

# Define if the C++ compiler supports the typename keyword.
#TODO make test case (I had one but it broke)
set(HAVE_TYPENAME 1)

# Define if we can trust the compiler not to insert extra bytes in
# structs between base structs and derived structs.
check_c_source_runs("
struct A { int a; };
struct B : public A { int b; };
int main(int argc, char *argv[]) {
  struct B i;
  if ((size_t) &(i.b) == ((size_t) &(i.a)) + sizeof(struct A)) {
    return 0;
  } else {
    return 1;
  }
}" SIMPLE_STRUCT_POINTERS)

# Define if we have STL hash_map etc. available
#TODO make test case
#//$[cdefine HAVE_STL_HASH]

# Check if we have a gettimeofday() function.
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)

# Define if gettimeofday() takes only one parameter.
check_cxx_source_compiles("
#include <sys/time.h>
int main(int argc, char *argv[]) {
  struct timeval tv;
  int result;
  result = gettimeofday(&tv);
  return 0;
}" GETTIMEOFDAY_ONE_PARAM)

# Check if we have getopt.
check_function_exists(getopt HAVE_GETOPT)
check_function_exists(getopt_long_only HAVE_GETOPT_LONG_ONLY)
check_include_file_cxx(getopt.h PHAVE_GETOPT_H)

# Define if you have ioctl(TIOCGWINSZ) to determine terminal width.
#XXX can we make a test case for this that isn't dependent on
# the current terminal?  It might also be useful for Cygwin users.
if(UNIX)
  set(IOCTL_TERMINAL_WIDTH 1)
endif()

# Do the system headers define a "streamsize" typedef?
check_cxx_source_compiles("
#include <ios>
std::streamsize ss;
int main(int argc, char *argv[]) { return 0; }
" HAVE_STREAMSIZE)

# Do the system headers define key ios typedefs like ios::openmode
# and ios::fmtflags?
#TODO make test case
set(HAVE_IOS_TYPEDEFS 1)

# Define if the C++ iostream library defines ios::binary.
#TODO make test case
#$[cdefine HAVE_IOS_BINARY]

# Can we safely call getenv() at static init time?
#TODO make test case? can we make a reliable one?
#$[cdefine STATIC_INIT_GETENV]

# Can we read the file /proc/self/[*] to determine our
# environment variables at static init time?
if(EXISTS "/proc/self/exe")
  set(HAVE_PROC_SELF_EXE 1)
endif()
if(EXISTS "/proc/self/maps")
  set(HAVE_PROC_SELF_MAPS 1)
endif()
if(EXISTS "/proc/self/environ")
  set(HAVE_PROC_SELF_ENVIRON 1)
endif()
if(EXISTS "/proc/self/cmdline")
  set(HAVE_PROC_SELF_CMDLINE 1)
endif()
if(EXISTS "/proc/curproc/file")
  set(HAVE_PROC_CURPROC_FILE 1)
endif()
if(EXISTS "/proc/curproc/map")
  set(HAVE_PROC_CURPROC_MAP 1)
endif()
if(EXISTS "/proc/curproc/cmdline")
  set(HAVE_PROC_CURPROC_CMDLINE 1)
endif()

# Do we have a global pair of argc/argv variables that we can read at
# static init time? Should we prototype them? What are they called?
#TODO make test case
#$[cdefine HAVE_GLOBAL_ARGV]
#$[cdefine PROTOTYPE_GLOBAL_ARGV]
#$[cdefine GLOBAL_ARGV]
#$[cdefine GLOBAL_ARGC]

# Do we have all these header files?
check_include_file_cxx(io.h PHAVE_IO_H)
check_include_file_cxx(iostream PHAVE_IOSTREAM)
check_include_file_cxx(malloc.h PHAVE_MALLOC_H)
check_include_file_cxx(sys/malloc.h PHAVE_SYS_MALLOC_H)
check_include_file_cxx(alloca.h PHAVE_ALLOCA_H)
check_include_file_cxx(locale.h PHAVE_LOCALE_H)
check_include_file_cxx(string.h PHAVE_STRING_H)
check_include_file_cxx(stdlib.h PHAVE_STDLIB_H)
check_include_file_cxx(limits.h PHAVE_LIMITS_H)
check_include_file_cxx(minmax.h PHAVE_MINMAX_H)
check_include_file_cxx(sstream PHAVE_SSTREAM)
check_include_file_cxx(new PHAVE_NEW)
check_include_file_cxx(sys/types.h PHAVE_SYS_TYPES_H)
check_include_file_cxx(sys/time.h PHAVE_SYS_TIME_H)
check_include_file_cxx(unistd.h PHAVE_UNISTD_H)
check_include_file_cxx(utime.h PHAVE_UTIME_H)
check_include_file_cxx(glob.h PHAVE_GLOB_H)
check_include_file_cxx(dirent.h PHAVE_DIRENT_H)
check_include_file_cxx(drfftw.h PHAVE_DRFFTW_H)
check_include_file_cxx(sys/soundcard.h PHAVE_SYS_SOUNDCARD_H)
check_include_file_cxx(ucontext.h PHAVE_UCONTEXT_H) #TODO doesn't work on OSX, use sys/ucontext.h
check_include_file_cxx(linux/input.h PHAVE_LINUX_INPUT_H)
check_include_file_cxx(stdint.h PHAVE_STDINT_H)
check_include_file_cxx(typeinfo HAVE_RTTI)

# Do we have Posix threads?
#set(HAVE_POSIX_THREADS ${CMAKE_USE_PTHREADS_INIT})

#/* Define if needed to have 64-bit file i/o */
#$[cdefine __USE_LARGEFILE64]

configure_file(dtool_config.h.cmake "${PROJECT_BINARY_DIR}/dtool_config.h")
include_directories("${PROJECT_BINARY_DIR}")
#install(FILES "${PROJECT_BINARY_DIR}/dtool_config.h" DESTINATION include/panda3d)
