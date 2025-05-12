#--------------------------------------------------------------------------
# input
#--------------------------------------------------------------------------

SET(files_input
	gamepad.cc
	gamepad.h
	inputserver.cc
	inputserver.h
	key.cc
	key.h
	keyboard.h
	mouse.h
	)
SOURCE_GROUP("input" FILES ${files_input})

SET(files_pch ../config.h ../config.cc)
SOURCE_GROUP("pch" FILES ${files_pch})
ADD_LIBRARY(input STATIC ${files_input} ${files_pch})
TARGET_PCH(input ../)
ADD_DEPENDENCIES(input glew enet)
TARGET_LINK_LIBRARIES(input PUBLIC engine exts glew enet soloud)