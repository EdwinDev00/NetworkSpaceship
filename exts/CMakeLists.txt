#--------------------------------------------------------------------------
# exts
#--------------------------------------------------------------------------

ADD_LIBRARY(exts INTERFACE)

ADD_SUBDIRECTORY(flatbuffers)

ADD_SUBDIRECTORY(glm)
ADD_SUBDIRECTORY(enet)
SET_TARGET_PROPERTIES(enet PROPERTIES FOLDER "exts/enet")
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE enet/include)
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE flatbuffers/include)

if(WIN32)
	SET(SOLOUD_BACKEND_WINMM ON)
else()
	SET(SOLOUD_BACKEND_ALSA ON)
endif()
SET(SOLOUD_STATIC ON)

ADD_SUBDIRECTORY(soloud/contrib)
SET_TARGET_PROPERTIES(soloud PROPERTIES FOLDER "exts/soloud")
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE soloud/include)

ADD_SUBDIRECTORY(glew)
SET_TARGET_PROPERTIES(glew PROPERTIES FOLDER "exts/glew")
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE glew/include)

SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
SET(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
SET(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")		  

ADD_SUBDIRECTORY(glfw)
SET_TARGET_PROPERTIES(glfw PROPERTIES FOLDER "exts/glfw")
#TARGET_INCLUDE_DIRECTORIES(exts INTERFACE glfw/include)
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glew/include)

ADD_LIBRARY(imgui STATIC 
			imgui/imconfig.h
			imgui/imgui.cpp
			imgui/imgui.h
			#imgui/imgui_demo.cpp
			imgui/imgui_draw.cpp 
			imgui/imgui_impl_glfw.h
			imgui/imgui_impl_glfw.cpp
			imgui/imgui_impl_opengl3.h
			imgui/imgui_impl_opengl3.cpp
			imgui/imgui_impl_opengl3_loader.h
			imgui/imgui_internal.h
			imgui/imgui_tables.cpp
			imgui/imgui_widgets.cpp
			imgui/imstb_rectpack.h
			imgui/imstb_textedit.h
			imgui/imstb_truetype.h
			)
			

TARGET_LINK_LIBRARIES(imgui PUBLIC glfw)
TARGET_INCLUDE_DIRECTORIES(imgui PUBLIC imgui)
SET_TARGET_PROPERTIES(imgui PROPERTIES FOLDER "exts/imgui")

TARGET_LINK_LIBRARIES(exts INTERFACE imgui)
