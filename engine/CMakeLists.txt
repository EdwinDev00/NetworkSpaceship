#--------------------------------------------------------------------------
# engine
#--------------------------------------------------------------------------

ADD_LIBRARY(engine INTERFACE)
TARGET_INCLUDE_DIRECTORIES(engine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
TARGET_LINK_LIBRARIES(engine INTERFACE ${OPENGL_LIBS})
ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(render)
ADD_SUBDIRECTORY(input)
ADD_SUBDIRECTORY(physics)
ADD_SUBDIRECTORY(network)
TARGET_LINK_LIBRARIES(engine INTERFACE core render input physics network)

SET_TARGET_PROPERTIES(core PROPERTIES FOLDER "engine")
SET_TARGET_PROPERTIES(physics PROPERTIES FOLDER "engine")
SET_TARGET_PROPERTIES(input PROPERTIES FOLDER "engine")
SET_TARGET_PROPERTIES(render PROPERTIES FOLDER "engine")
SET_TARGET_PROPERTIES(network PROPERTIES FOLDER "engine")


