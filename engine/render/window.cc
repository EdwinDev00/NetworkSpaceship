//------------------------------------------------------------------------------
// window.cc
// (C) 2015-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "window.h"
#include <imgui.h>
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include "input/inputserver.h"

namespace Display
{


#ifdef __WIN32__
#define APICALLTYPE __stdcall
#else
#define APICALLTYPE
#endif
//------------------------------------------------------------------------------
/**
*/
static void GLAPIENTRY
GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	std::string msg("[OPENGL DEBUG MESSAGE] ");

	// print error severity
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_LOW:
		msg.append("<Low severity> ");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		msg.append("<Medium severity> ");
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		msg.append("<High severity> ");
		break;
	}

	// append message to output
	msg.append(message);

	// print message
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		printf("Error: %s\n", msg.c_str());
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		printf("Performance issue: %s\n", msg.c_str());
		break;
	default:		// Portability, Deprecated, Other
		break;
	}
}

int32 Window::WindowCount = 0;
//------------------------------------------------------------------------------
/**
*/
Window::Window() :
	window(nullptr),
	width(1024),
	height(768),
	title("gscept Lab Environment")
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Window::~Window()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
Window::StaticKeyPressCallback(GLFWwindow* win, int32 key, int32 scancode, int32 action, int32 mods)
{
	Window* window = (Window*)glfwGetWindowUserPointer(win);
	if (ImGui::IsAnyItemHovered())
	{
		ImGui_ImplGlfw_KeyCallback(win, key, scancode, action, mods);
	}
	else if (nullptr != window->keyPressCallback)
		window->keyPressCallback(key, scancode, action, mods);
	
	Input::InputHandler::HandleKeyEvent(key, scancode, action, mods);
}

//------------------------------------------------------------------------------
/**
*/
void
Window::StaticMousePressCallback(GLFWwindow* win, int32 button, int32 action, int32 mods)
{
	Window* window = (Window*)glfwGetWindowUserPointer(win);
	
	ImGui_ImplGlfw_MouseButtonCallback(win, button, action, mods);
	
	if (nullptr != window->mousePressCallback)
		window->mousePressCallback(button, action, mods);

	Input::InputHandler::HandleMousePressEvent(button, action, mods);
}

//------------------------------------------------------------------------------
/**
*/
void
Window::StaticMouseMoveCallback(GLFWwindow* win, float64 x, float64 y)
{
	Window* window = (Window*)glfwGetWindowUserPointer(win);
	if (nullptr != window->mouseMoveCallback)
		window->mouseMoveCallback(x, y);
	Input::InputHandler::HandleMouseMoveEvent(x, y);
}

//------------------------------------------------------------------------------
/**
*/
void
Window::StaticMouseEnterLeaveCallback(GLFWwindow* win, int32 mode)
{
	Window* window = (Window*)glfwGetWindowUserPointer(win);
	if (nullptr != window->mouseLeaveEnterCallback) window->mouseLeaveEnterCallback(mode == 0);
}

//------------------------------------------------------------------------------
/**
*/
void
Window::StaticMouseScrollCallback(GLFWwindow* win, float64 x, float64 y)
{
	Window* window = (Window*)glfwGetWindowUserPointer(win);
	if (ImGui::IsAnyItemHovered())
	{
		ImGui_ImplGlfw_ScrollCallback(win, x, y);
	}
	else if (nullptr != window->mouseScrollCallback) window->mouseScrollCallback(x, y);
}

//------------------------------------------------------------------------------
/**
*/
void
Window::StaticWindowResizeCallback(GLFWwindow* win, int32 x, int32 y)
{
    Window* window = (Window*)glfwGetWindowUserPointer(win);
    window->width = x;
    window->height = y;
    window->Resize();
    if (nullptr != window->windowResizeCallback)
        window->windowResizeCallback(x, y);
}

//------------------------------------------------------------------------------
/**
*/
void
Window::StaticCloseCallback(GLFWwindow* window)
{
	// FIXME: should be more graceful...
	std::exit(0);
}

//------------------------------------------------------------------------------
/**
*/
void
Window::StaticFocusCallback(GLFWwindow* window, int focus)
{
	// empty for now
}

//------------------------------------------------------------------------------
/**
*/
void
Window::StaticCharCallback(GLFWwindow* window, unsigned int key)
{
	if (ImGui::IsAnyItemHovered())
	{
		ImGui_ImplGlfw_CharCallback(window, key);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
Window::StaticDropCallback(GLFWwindow* window, int files, const char** args)
{
	// empty for now
}


//------------------------------------------------------------------------------
/**
*/
void
Window::Resize()
{
	if (nullptr != this->window)
	{
		glfwSetWindowSize(this->window, this->width, this->height);

		// setup viewport
		glViewport(0, 0, this->width, this->height);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
Window::Retitle()
{
	if (nullptr != this->window) glfwSetWindowTitle(this->window, this->title.c_str());
}

//------------------------------------------------------------------------------
/**
*/
bool
Window::Open()
{
	if (Window::WindowCount == 0)
	{
		if (!glfwInit()) return false;
	}

	// setup window
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
	//glfwWindowHint(GLFW_SAMPLES, 4);

	// open window
	this->window = glfwCreateWindow(this->width, this->height, this->title.c_str(), nullptr, nullptr);
	glfwMakeContextCurrent(this->window);

	if (nullptr != this->window && WindowCount == 0)
	{
		GLenum res = glewInit();
		assert(res == GLEW_OK);

		const GLubyte* vendor = glGetString(GL_VENDOR);
		const GLubyte* renderer = glGetString(GL_RENDERER);
		printf("GPU Vendor: %s\n", vendor);
		printf("GPU Render Device: %s\n", renderer);

		if (!(GLEW_VERSION_4_0))
		{
			printf("[WARNING]: OpenGL 4.0+ is not supported on this hardware!\n");
			glfwDestroyWindow(this->window);
			this->window = nullptr;
			return false;
		}

		// setup debug callback
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(GLDebugCallback, NULL);
		GLuint unusedIds;
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);

		// setup stuff
		//glEnable(GL_FRAMEBUFFER_SRGB);
		glEnable(GL_MULTISAMPLE);

		// disable vsync
		glfwSwapInterval(0);

		// setup viewport
		glViewport(0, 0, this->width, this->height);
	}


	glfwSetWindowUserPointer(this->window, this);
	glfwSetKeyCallback(this->window, Window::StaticKeyPressCallback);
	glfwSetMouseButtonCallback(this->window, Window::StaticMousePressCallback);
	glfwSetCursorPosCallback(this->window, Window::StaticMouseMoveCallback);
	glfwSetCursorEnterCallback(this->window, Window::StaticMouseEnterLeaveCallback);
	glfwSetScrollCallback(this->window, Window::StaticMouseScrollCallback);
	glfwSetWindowSizeCallback(this->window, Window::StaticWindowResizeCallback);
	glfwSetWindowCloseCallback(window, Window::StaticCloseCallback);
	glfwSetWindowFocusCallback(window, Window::StaticFocusCallback);
	glfwSetCharCallback(window, Window::StaticCharCallback);
	glfwSetDropCallback(window, Window::StaticDropCallback);


	// setup imgui implementation
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = { (float)width, (float)height };
	io.DeltaTime = 1 / 60.0f;
	//io.PixelCenterOffset = 0.0f;
	//io.FontTexUvForWhite = ImVec2(1, 1);
	//io.RenderDrawListsFn = ImguiDrawFunction;

	ImGui_ImplGlfw_InitForOpenGL(this->window, false);
	ImGui_ImplOpenGL3_Init();

	// load default font
	ImFontConfig config;
	config.OversampleH = 3;
	config.OversampleV = 1;
#if _WIN32
	ImFont* font = io.Fonts->AddFontFromFileTTF("c:/windows/fonts/tahoma.ttf", 14, &config);
#else
	ImFont* font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 18, &config);
#endif

	unsigned char* buffer;
	int width, height, channels;
	io.Fonts->GetTexDataAsRGBA32(&buffer, &width, &height, &channels);

	glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	Input::InputHandler::Create();

	// increase window count and return result
	Window::WindowCount++;

	return this->window != nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
Window::Close()
{
	if (nullptr != this->window) glfwDestroyWindow(this->window);
	this->window = nullptr;
	Window::WindowCount--;
	if (Window::WindowCount == 0)
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		glfwTerminate();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
Window::MakeCurrent()
{
	glfwMakeContextCurrent(this->window);
}

//------------------------------------------------------------------------------
/**
*/
void
Window::Update()
{
	Input::InputHandler::BeginFrame();
	glfwPollEvents();	
}

//------------------------------------------------------------------------------
/**
*/
void
Window::SwapBuffers()
{
	if (this->window)
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
		if (nullptr != this->uiFunc)
			this->uiFunc();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(this->window);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
Window::GetMousePos(float64& x, float64& y)
{
	glfwGetCursorPos(this->window, &x, &y);
}

} // namespace Display