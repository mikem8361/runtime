project(createdump)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(BEFORE ${VM_DIR})
if(CLR_CMAKE_HOST_OSX)
  include_directories(${CLR_DIR}/src/inc/llvm)
endif(CLR_CMAKE_HOST_OSX)

remove_definitions(-DUNICODE)
remove_definitions(-D_UNICODE)

if(CLR_CMAKE_HOST_WIN32)

    set(CREATEDUMP_LIB_SOURCES
        createdump.cpp
        createdumpwindows.cpp
        createdump.rc
    )

    set(CREATEDUMP_LIBRARIES
        kernel32.lib
        ${STATIC_MT_CRT_LIB}
        advapi32.lib
        version.lib
        dbghelp.lib
    )

    set(CREATEDUMP_DEPENDENCIES
        mscordaccore
    )

else(CLR_CMAKE_HOST_WIN32)

    include(configure.cmake)

    # Set the RPATH of createdump so that it can find dependencies without needing to set LD_LIBRARY_PATH
    # For more information: http://www.cmake.org/Wiki/CMake_RPATH_handling.
    if (CORECLR_SET_RPATH)
      set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
      if(CLR_CMAKE_HOST_OSX)
        set(CMAKE_INSTALL_RPATH "@loader_path")
      else()
        set(CMAKE_INSTALL_RPATH "\$ORIGIN")
      endif(CLR_CMAKE_HOST_OSX)
    endif (CORECLR_SET_RPATH)

    add_definitions(-DPAL_STDCPP_COMPAT)

    set(CREATEDUMP_LIB_SOURCES
        createdump.cpp
        createdumpunix.cpp
        crashinfo.cpp
        threadinfo.cpp
        datatarget.cpp
        dumpwriter.cpp
    )

    set(CREATEDUMP_LIBRARIES
        # share the PAL in the dac module
        mscordaccore
    )

    set(CREATEDUMP_DEPENDENCIES
        mscordaccore
    )

    if(CLR_CMAKE_HOST_OSX)
        list(APPEND CREATEDUMP_LIB_SOURCES
            crashinfomac.cpp
            threadinfomac.cpp
        )
    else()
        list(APPEND CREATEDUMP_LIB_SOURCES
            crashinfounix.cpp
            threadinfounix.cpp
        )
        set(CREATEDUMP_SOURCES
            ${PAL_REDEFINES_FILE}
        )
        list(APPEND CREATEDUMP_DEPENDENCIES
            pal_redefines_file
        )
    endif(CLR_CMAKE_HOST_OSX)

endif(CLR_CMAKE_HOST_WIN32)

    # Build the createdump common library

    _add_library(createdump_lib
        ${CREATEDUMP_LIB_SOURCES}
    )

    target_link_libraries(createdump_lib
        corguids
        dbgutil
    )

    # Build the createdump executable

    _add_executable(createdump
        main.cpp
        ${CREATEDUMP_SOURCES}
    )

    target_link_libraries(createdump
        createdump_lib
        ${CREATEDUMP_LIBRARIES}
    )

    add_dependencies(createdump ${CREATEDUMP_DEPENDENCIES})

install_clr(TARGETS createdump ADDITIONAL_DESTINATION sharedFramework)
