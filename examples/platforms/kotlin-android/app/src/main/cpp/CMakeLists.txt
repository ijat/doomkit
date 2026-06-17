# =============================================================================
#  CMakeLists.txt  --  Builds the DOOM engine + JNI bridge into one .so.
# -----------------------------------------------------------------------------
#  Android Studio's NDK integration invokes this. It compiles:
#    * our JNI bridge (native-doom.cpp),
#    * the reusable key-queue helper from this package,
#    * the upstream DOOM engine sources (which you drop into engine/).
#  GPLv2. See LICENSE.
# =============================================================================
cmake_minimum_required(VERSION 3.22)
project(doom)

# Where you copied the upstream doomgeneric engine .c/.h files and this
# package's include/ + src/. Adjust these two paths to your checkout.
set(ENGINE_DIR  ${CMAKE_SOURCE_DIR}/engine)              # upstream engine sources
set(PKG_DIR     ${CMAKE_SOURCE_DIR}/../../../../../../..) # doomkit root

# Engine sources: every .c EXCEPT the desktop platform files (doomgeneric_*.c),
# because native-doom.cpp provides the DG_* symbols itself.
file(GLOB ENGINE_SRC ${ENGINE_DIR}/*.c)
list(FILTER ENGINE_SRC EXCLUDE REGEX "doomgeneric_.*\\.c$")

add_library(doom SHARED
    native-doom.cpp
    ${PKG_DIR}/src/dg_keyqueue.c       # reuse the package's tested ring buffer
    ${ENGINE_SRC}
)

target_include_directories(doom PRIVATE
    ${ENGINE_DIR}
    ${PKG_DIR}/include
)

# Android system libs we use: native window + logging.
find_library(log-lib log)
find_library(android-lib android)
target_link_libraries(doom ${log-lib} ${android-lib})
