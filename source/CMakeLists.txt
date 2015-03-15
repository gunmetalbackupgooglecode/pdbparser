
# Generate meta-header
configure_file(template-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/template-version.h)

# We do NOT want to deploy this header (at least not for binary packages)
#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/template/template-version.h DESTINATION ${INSTALL_INCLUDE}/template)

# Include directories
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/common")

# Libraries
set(IDE_FOLDER "")
add_subdirectory(symenglib)

# Applications
 set(IDE_FOLDER "Applications")
add_subdirectory(pdbparser-console)

# Tests
set(IDE_FOLDER "Tests")
add_subdirectory(tests)
