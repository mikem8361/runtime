
macro(append_extra_system_libs NativeLibsExtra)
    if (CLR_CMAKE_TARGET_LINUX AND NOT CLR_CMAKE_TARGET_ANDROID)
        list(APPEND ${NativeLibsExtra} rt)
    elseif (CLR_CMAKE_TARGET_FREEBSD)
        list(APPEND ${NativeLibsExtra} pthread)
        find_library(INOTIFY_LIBRARY inotify HINTS ${CROSS_ROOTFS}/usr/local/lib)
        list(APPEND ${NativeLibsExtra} ${INOTIFY_LIBRARY})
    elseif (CLR_CMAKE_TARGET_SUNOS)
        list(APPEND ${NativeLibsExtra} socket)
    elseif (CLR_CMAKE_TARGET_HAIKU)
        list (APPEND ${NativeLibsExtra} network bsd)
    endif ()

    if (CLR_CMAKE_TARGET_APPLE)
        find_library(FOUNDATION Foundation REQUIRED)
        list(APPEND ${NativeLibsExtra} ${FOUNDATION})
    endif ()
endmacro()
