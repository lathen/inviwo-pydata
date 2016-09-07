add_subdirectory(ext/pybind11)
pybind11_add_module(inviwo_pydata pydata.cpp)
# .PYD file extension on Windows
set_target_properties(inviwo_pydata PROPERTIES SUFFIX ".pyd")

#--------------------------------------------------------------------
# Inviwo PyData Module
ivw_module(PyData)

#--------------------------------------------------------------------
# Add header files
set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/imagesourcebuffer.h
)
ivw_group("Header Files" ${HEADER_FILES})

#--------------------------------------------------------------------
# Add source files
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/imagesourcebuffer.cpp
)
ivw_group("Source Files" ${SOURCE_FILES})

#--------------------------------------------------------------------
# Add Unittests
set(TEST_FILES
	#${CMAKE_CURRENT_SOURCE_DIR}/tests/pydata-test.cpp
)
ivw_add_unittest(${TEST_FILES})

#--------------------------------------------------------------------
# Create module
ivw_create_module(${SOURCE_FILES} ${HEADER_FILES})
