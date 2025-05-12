#--------------------------------------------------------------------------
# network
#--------------------------------------------------------------------------

SET(files_network
	network.h
	network.cc
	client.h
	client.cc
	server.h
	server.cc
	proto.h
	timer.h
	)
SOURCE_GROUP("network" FILES ${files_network})

SET(files_pch ../config.h ../config.cc)
SOURCE_GROUP("pch" FILES ${files_pch})
ADD_LIBRARY(network STATIC ${files_network} ${files_pch})
TARGET_PCH(network ../)
ADD_DEPENDENCIES(network glew enet)
TARGET_LINK_LIBRARIES(network PUBLIC engine exts glew enet soloud)