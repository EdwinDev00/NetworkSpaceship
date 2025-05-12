
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	if(NOT GLM_QUIET)
		message("GLM: Clang - ${CMAKE_CXX_COMPILER_ID} compiler")
	endif()

	add_compile_options(-Werror -Weverything)
	add_compile_options(-Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-c++11-long-long -Wno-padded -Wno-gnu-anonymous-struct -Wno-nested-anon-types)
	add_compile_options(-Wno-undefined-reinterpret-cast -Wno-sign-conversion -Wno-unused-variable -Wno-missing-prototypes -Wno-unreachable-code -Wno-missing-variable-declarations -Wno-sign-compare -Wno-global-constructors -Wno-unused-macros -Wno-format-nonliteral)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	if(NOT GLM_QUIET)
		message("GLM: GCC - ${CMAKE_CXX_COMPILER_ID} compiler")
	endif()

	add_compile_options(-O2)
	add_compile_options(-Wno-long-long)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
	if(NOT GLM_QUIET)
		message("GLM: Intel - ${CMAKE_CXX_COMPILER_ID} compiler")
	endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	if(NOT GLM_QUIET)
		message("GLM: Visual C++ - ${CMAKE_CXX_COMPILER_ID} compiler")
	endif()

	add_compile_options(/W4 /WX)
	add_compile_options(/wd4309 /wd4324 /wd4389 /wd4127 /wd4267 /wd4146 /wd4201 /wd4464 /wd4514 /wd4701 /wd4820 /wd4365)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()


set(GLM_VERSION "0.9.9")
option(BUILD_GLM_SHARED_LIBS "Build shared library" OFF)
option(BUILD_GLM_STATIC_LIBS "Build static library" ON)

file(GLOB ROOT_SOURCE *.cpp)
file(GLOB ROOT_INLINE *.inl)
file(GLOB ROOT_HEADER *.hpp)
file(GLOB ROOT_TEXT ../*.txt)
file(GLOB ROOT_MD ../*.md)
file(GLOB ROOT_NAT ../util/glm.natvis)

file(GLOB_RECURSE CORE_SOURCE ./detail/*.cpp)
file(GLOB_RECURSE CORE_INLINE ./detail/*.inl)
file(GLOB_RECURSE CORE_HEADER ./detail/*.hpp)

file(GLOB_RECURSE EXT_SOURCE ./ext/*.cpp)
file(GLOB_RECURSE EXT_INLINE ./ext/*.inl)
file(GLOB_RECURSE EXT_HEADER ./ext/*.hpp)

file(GLOB_RECURSE GTC_SOURCE ./gtc/*.cpp)
file(GLOB_RECURSE GTC_INLINE ./gtc/*.inl)
file(GLOB_RECURSE GTC_HEADER ./gtc/*.hpp)

file(GLOB_RECURSE GTX_SOURCE ./gtx/*.cpp)
file(GLOB_RECURSE GTX_INLINE ./gtx/*.inl)

file(GLOB_RECURSE GTX_HEADER ./gtx/*.hpp)

file(GLOB_RECURSE SIMD_SOURCE ./simd/*.cpp)
file(GLOB_RECURSE SIMD_INLINE ./simd/*.inl)
file(GLOB_RECURSE SIMD_HEADER ./simd/*.h)

source_group("Text Files" FILES ${ROOT_TEXT} ${ROOT_MD})
source_group("Core Files" FILES ${CORE_SOURCE})
source_group("Core Files" FILES ${CORE_INLINE})
source_group("Core Files" FILES ${CORE_HEADER})
source_group("EXT Files" FILES ${EXT_SOURCE})
source_group("EXT Files" FILES ${EXT_INLINE})
source_group("EXT Files" FILES ${EXT_HEADER})
source_group("GTC Files" FILES ${GTC_SOURCE})
source_group("GTC Files" FILES ${GTC_INLINE})
source_group("GTC Files" FILES ${GTC_HEADER})
source_group("GTX Files" FILES ${GTX_SOURCE})
source_group("GTX Files" FILES ${GTX_INLINE})
source_group("GTX Files" FILES ${GTX_HEADER})
source_group("SIMD Files" FILES ${SIMD_SOURCE})
source_group("SIMD Files" FILES ${SIMD_INLINE})
source_group("SIMD Files" FILES ${SIMD_HEADER})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

if(BUILD_GLM_STATIC_LIBS)
add_library(glm_static STATIC ${ROOT_TEXT} ${ROOT_MD} ${ROOT_NAT}
	${ROOT_SOURCE}    ${ROOT_INLINE}    ${ROOT_HEADER}
	${CORE_SOURCE}    ${CORE_INLINE}    ${CORE_HEADER}
	${EXT_SOURCE}     ${EXT_INLINE}     ${EXT_HEADER}
	${GTC_SOURCE}     ${GTC_INLINE}     ${GTC_HEADER}
	${GTX_SOURCE}     ${GTX_INLINE}     ${GTX_HEADER}
	${SIMD_SOURCE}    ${SIMD_INLINE}    ${SIMD_HEADER})
	
	SET_TARGET_PROPERTIES(glm_static PROPERTIES FOLDER "exts/glm")
    TARGET_INCLUDE_DIRECTORIES(glm_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

endif()

if(BUILD_GLM_SHARED_LIBS)
add_library(glm_shared SHARED ${ROOT_TEXT} ${ROOT_MD} ${ROOT_NAT}
	${ROOT_SOURCE}    ${ROOT_INLINE}    ${ROOT_HEADER}
	${CORE_SOURCE}    ${CORE_INLINE}    ${CORE_HEADER}
	${EXT_SOURCE}     ${EXT_INLINE}     ${EXT_HEADER}
	${GTC_SOURCE}     ${GTC_INLINE}     ${GTC_HEADER}
	${GTX_SOURCE}     ${GTX_INLINE}     ${GTX_HEADER}
	${SIMD_SOURCE}    ${SIMD_INLINE}    ${SIMD_HEADER})

	SET_TARGET_PROPERTIES(glm_shared PROPERTIES FOLDER "exts/glm")
endif()

