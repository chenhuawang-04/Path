add_library(ToolPath STATIC)

target_sources(ToolPath
        PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES
        CurrentExecutable.cppm
)

set_target_properties(ToolPath PROPERTIES
        EXPORT_NAME Path
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
)

add_library(Tool::Path ALIAS ToolPath)

if(PROJECT_IS_TOP_LEVEL)
    include(GNUInstallDirs)

    install(TARGETS ToolPath
            EXPORT ToolPathTargets
            FILE_SET CXX_MODULES DESTINATION "${CMAKE_INSTALL_LIBDIR}/cxx/modules"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

    install(EXPORT ToolPathTargets
            NAMESPACE Tool::
            FILE ToolPathTargets.cmake
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ToolPath"
    )
endif()