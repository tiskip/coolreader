# - Try to find the libunibreak
# Once done this will define
#
#  LIBUNIBREAK_FOUND - system has libunibreak
#  LIBUNIBREAK_INCLUDE_DIR - The include directory to use for the libunibreak headers
#  LIBUNIBREAK_LIBRARIES - Link these to use libunibreak
#  LIBUNIBREAK_DEFINITIONS - Compiler switches required for using libunibreak

# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

if (LIBUNIBREAK_LIBRARIES AND LIBUNIBREAK_INCLUDE_DIR)

  # in cache already
  set(LIBUNIBREAK_FOUND TRUE)

else (LIBUNIBREAK_LIBRARIES AND LIBUNIBREAK_INCLUDE_DIR)

  if (NOT MSVC)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    pkg_check_modules(PC_LIBUNIBREAK libunibreak)

    set(LIBUNIBREAK_DEFINITIONS ${PC_LIBUNIBREAK_CFLAGS_OTHER})
  endif (NOT MSVC)

  find_path(LIBUNIBREAK_INCLUDE_DIR linebreak.h
    PATHS
    ${PC_LIBUNIBREAK_INCLUDEDIR}
    ${PC_LIBUNIBREAK_INCLUDE_DIRS}
  )

  find_library(LIBUNIBREAK_LIBRARIES NAMES unibreak
    PATHS
    ${PC_LIBUNIBREAK_LIBDIR}
    ${PC_LIBUNIBREAK_LIBRARY_DIRS}
  )

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(libunibreak DEFAULT_MSG LIBUNIBREAK_LIBRARIES LIBUNIBREAK_INCLUDE_DIR)

  mark_as_advanced(LIBUNIBREAK_LIBRARIES LIBUNIBREAK_INCLUDE_DIR)

endif (LIBUNIBREAK_LIBRARIES AND LIBUNIBREAK_INCLUDE_DIR)
