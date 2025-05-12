#--------------------------------------------------------------------------
# spacegame project
#--------------------------------------------------------------------------

PROJECT(spacegame)
FILE(GLOB project_headers code/*.h)
FILE(GLOB project_sources code/*.cc)

SET(files_project ${project_headers} ${project_sources})
SOURCE_GROUP("spacegame" FILES ${files_project})

ADD_EXECUTABLE(spacegame ${files_project})
TARGET_LINK_LIBRARIES(spacegame core render)
ADD_DEPENDENCIES(spacegame core render)

IF(MSVC)
    set_property(TARGET spacegame PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
ENDIF()