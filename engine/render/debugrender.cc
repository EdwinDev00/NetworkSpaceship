//------------------------------------------------------------------------------
//  @file debugrender.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include <queue>
#include "debugrender.h"
#include "GL/glew.h"
#include "shaderresource.h"
#include "cameramanager.h"
#include "imgui.h"

namespace Debug
{

enum DebugShape
{
	LINE,
	SPHERE,
	BOX,
	CONE,
	CAPSULE,
	FRUSTUM,
	MESH,
	CIRCLE,
	NUM_DEBUG_SHAPES
};
struct RenderCommand
{
	DebugShape shape;
	char rendermode = RenderMode::Normal;
	float linewidth = 1.0f;
};

struct LineCommand : public RenderCommand
{
	glm::vec3 startpoint = glm::vec3(0.0f);
	glm::vec3 endpoint = glm::vec3(1.0f);
	glm::vec4 startcolor = glm::vec4(1.0f);
	glm::vec4 endcolor = glm::vec4(1.0f);
};

struct BoxCommand : public RenderCommand
{
	glm::mat4 transform = glm::mat4();
	glm::vec4 color = glm::vec4(1.0f);
};

struct TextCommand
{
	glm::vec4 point;
	glm::vec4 color;
	std::string text;
};

static std::queue<RenderCommand*> cmds;
static std::queue<TextCommand> textcmds;
static GLuint shaders[NUM_DEBUG_SHAPES];
static GLuint vao[NUM_DEBUG_SHAPES];
static GLuint ib[NUM_DEBUG_SHAPES];
static GLuint vbo[NUM_DEBUG_SHAPES];

void DrawDebugText(const char* text, glm::vec3 point, const glm::vec4 color)
{
	TextCommand cmd;
	cmd.color = color;
	cmd.text = text;
	cmd.point = glm::vec4(point, 1.0f);
	textcmds.push(cmd);
}

void DrawLine(const glm::vec3& startPoint, const glm::vec3& endPoint, const float lineWidth, const glm::vec4& startColor, const glm::vec4& endColor, const RenderMode& renderModes)
{
	LineCommand* cmd = new LineCommand();
	cmd->shape = DebugShape::LINE;
	cmd->startpoint = startPoint;
	cmd->endpoint = endPoint;
	cmd->linewidth = lineWidth;
	cmd->rendermode = renderModes;
	cmd->startcolor = startColor;
	cmd->endcolor = endColor;
	cmds.push(cmd);
}

void DrawBox(const glm::vec3& position, const glm::quat& rotation, const float scale, const glm::vec4& color, const RenderMode renderModes, const float lineWidth)
{

	glm::mat4 transform = glm::scale(glm::vec3(scale)) * (glm::mat4)rotation;
	glm::translate(transform, position);
	
	BoxCommand* cmd = new BoxCommand();
	cmd->shape = DebugShape::BOX;
	cmd->transform = transform;
	cmd->linewidth = lineWidth;
	cmd->color = color;
	cmd->rendermode = renderModes;
	cmds.push(cmd);
}

void DrawBox(const glm::vec3& position, const glm::quat& rotation, const float width, const float height, const float length, const glm::vec4& color, const RenderMode renderModes, const float lineWidth)
{
	glm::mat4 transform = glm::scale(glm::vec3(width, height, length)) * (glm::mat4)rotation;
	glm::translate(transform, position);
	
	BoxCommand* cmd = new BoxCommand();
	cmd->shape = DebugShape::BOX;
	cmd->transform = transform;
	cmd->linewidth = lineWidth;
	cmd->color = color;
	cmd->rendermode = renderModes;
	cmds.push(cmd);
}

void DrawBox(const glm::mat4& transform, const glm::vec4& color, const RenderMode renderModes, const float lineWidth)
{
	BoxCommand* cmd = new BoxCommand();
	cmd->shape = DebugShape::BOX;
	cmd->transform = transform;
	cmd->linewidth = lineWidth;
	cmd->color = color;
	cmd->rendermode = renderModes;
	cmds.push(cmd);
}

void SetupShaders()
{
	Render::ShaderResourceId const vsDebug = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::VERTEXSHADER, "shd/debug.vs");
	Render::ShaderResourceId const psDebug = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::FRAGMENTSHADER, "shd/debug.fs");
	Render::ShaderProgramId const progDebug = Render::ShaderResource::CompileShaderProgram({ vsDebug, psDebug });
	shaders[DebugShape::BOX] = Render::ShaderResource::GetProgramHandle(progDebug);

	Render::ShaderResourceId const vsLine = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::VERTEXSHADER, "shd/debug_lines.vs");
	Render::ShaderResourceId const psLine = Render::ShaderResource::LoadShader(Render::ShaderResource::ShaderType::FRAGMENTSHADER, "shd/debug_lines.fs");
	Render::ShaderProgramId const progLine = Render::ShaderResource::CompileShaderProgram({ vsLine, psLine });
	shaders[DebugShape::LINE] = Render::ShaderResource::GetProgramHandle(progLine);
}

void SetupLine()
{
	glGenVertexArrays(1, &vao[DebugShape::LINE]);
	glBindVertexArray(vao[DebugShape::LINE]);

	glGenBuffers(1, &vbo[DebugShape::LINE]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[DebugShape::LINE]);
	// buffer some dummy data. We use uniforms for setting the positions and colors of the lines.
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float), NULL, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat), NULL);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SetupBox()
{
	const int meshSize = 24;
	const GLfloat mesh[meshSize] =
	{
		0.5, -0.5, -0.5,
		0.5, -0.5, 0.5,
		-0.5, -0.5, 0.5,
		-0.5, -0.5, -0.5,
		0.5, 0.5, -0.5,
		0.5, 0.5, 0.5,
		-0.5, 0.5, 0.5,
		-0.5, 0.5, -0.5
	};
	const int indicesSize = 60;
	const int indices[indicesSize] =
	{
		// triangles
		0, 1, 2,
		3, 0, 2,
		4, 7, 6,
		5, 4, 6,
		0, 4, 5,
		1, 0, 5,
		1, 5, 6,
		2, 1, 6,
		2, 6, 7,
		3, 2, 7,
		4, 0, 3,
		7, 4, 3,

		// outlines (offset 36)
		0, 1,
		1, 2,
		2, 3,
		3, 0,
		4, 5,
		5, 6,
		6, 7,
		7, 4,
		0, 4,
		1, 5,
		2, 6,
		3, 7
	};

	glGenVertexArrays(1, &vao[DebugShape::BOX]);
	glBindVertexArray(vao[DebugShape::BOX]);

	glGenBuffers(1, &vbo[DebugShape::BOX]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[DebugShape::BOX]);
	glBufferData(GL_ARRAY_BUFFER, meshSize * sizeof(GLfloat), mesh, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, NULL);

	glGenBuffers(1, &ib[DebugShape::BOX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib[DebugShape::BOX]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize * sizeof(GLuint), indices, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//------------------------------------------------------------------------------
/**
	Setup debug rendering context
*/
void InitDebugRendering()
{
	SetupShaders();
	SetupLine();
	SetupBox();
}

void RenderLine(RenderCommand* command)
{
	LineCommand* lineCommand = (LineCommand*)command;

	glUseProgram(shaders[DebugShape::LINE]);

	if ((lineCommand->rendermode & RenderMode::AlwaysOnTop) == RenderMode::AlwaysOnTop)
	{
		glDepthFunc(GL_ALWAYS);
		glDepthRange(0.0f, 0.01f);
	}

	glPolygonMode(GL_FRONT, GL_LINE);
	glLineWidth(lineCommand->linewidth);

	glBindVertexArray(vao[DebugShape::LINE]);

	// This is so dumb, yet so much fun
	static GLuint v0pos = glGetUniformLocation(shaders[DebugShape::LINE], "v0pos");
	static GLuint v1pos = glGetUniformLocation(shaders[DebugShape::LINE], "v1pos");
	static GLuint v0color = glGetUniformLocation(shaders[DebugShape::LINE], "v0color");
	static GLuint v1color = glGetUniformLocation(shaders[DebugShape::LINE], "v1color");
	static GLuint viewProjection = glGetUniformLocation(shaders[DebugShape::LINE], "viewProjection");

	// Upload uniforms for positions and colors
	glUniform4fv(v0pos, 1, &lineCommand->startpoint[0]);
	glUniform4fv(v1pos, 1, &lineCommand->endpoint[0]);
	glUniform4fv(v0color, 1, &lineCommand->startcolor[0]);
	glUniform4fv(v1color, 1, &lineCommand->endcolor[0]);

	Render::Camera* const mainCamera = Render::CameraManager::GetCamera(CAMERA_MAIN);
	glUniformMatrix4fv(viewProjection, 1, GL_FALSE, &mainCamera->viewProjection[0][0]);

	glDrawArrays(GL_LINES, 0, 2);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glPolygonMode(GL_FRONT, GL_FILL);

	if ((lineCommand->rendermode & RenderMode::AlwaysOnTop) == RenderMode::AlwaysOnTop)
	{
		glDepthFunc(GL_LEQUAL);
		glDepthRange(0.0f, 1.0f);
	}
}

void RenderBox(RenderCommand* command)
{
	BoxCommand* cmd = (BoxCommand*)command;

	glUseProgram(shaders[DebugShape::BOX]);

	glBindVertexArray(vao[DebugShape::BOX]);

	GLuint loc = glGetUniformLocation(shaders[DebugShape::BOX], "color");
	glUniform4fv(loc, 1, &cmd->color.x);

	static GLuint model = glGetUniformLocation(shaders[DebugShape::BOX], "model");
	static GLuint viewProjection = glGetUniformLocation(shaders[DebugShape::BOX], "viewProjection");
	Render::Camera* const mainCamera = Render::CameraManager::GetCamera(CAMERA_MAIN);
	glUniformMatrix4fv(model, 1, GL_FALSE, &cmd->transform[0][0]);
	glUniformMatrix4fv(viewProjection, 1, GL_FALSE, &mainCamera->viewProjection[0][0]);

	if ((cmd->rendermode & RenderMode::AlwaysOnTop) == RenderMode::AlwaysOnTop)
	{
		glDepthFunc(GL_ALWAYS);
		glDepthRange(0.0f, 0.01f);
	}

	if ((cmd->rendermode & RenderMode::WireFrame) == RenderMode::WireFrame)
	{
		glPolygonMode(GL_FRONT, GL_LINE);
		glLineWidth(cmd->linewidth);

		glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, (void*)(36 * sizeof(GLuint)));
		glPolygonMode(GL_FRONT, GL_FILL);
	}
	else
	{
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);
	}

	if ((cmd->rendermode & RenderMode::AlwaysOnTop) == RenderMode::AlwaysOnTop)
	{
		glDepthFunc(GL_LEQUAL);
		glDepthRange(0.0f, 1.0f);
	}

	glBindVertexArray(0);
}

void DispatchDebugDrawing()
{
	while (!cmds.empty())
	{
		RenderCommand* currentCommand = cmds.front();
		cmds.pop();
		switch (currentCommand->shape)
		{
		case DebugShape::LINE:
		{
			RenderLine(currentCommand);
			break;
		}
		case DebugShape::BOX:
		{
			RenderBox(currentCommand);
			break;
		}
		default:
		{
			n_assert2(false, "Debug::RenderShape not fully implemented!\n");
			break;
		}
		} // switch

		delete currentCommand;
	}
}

void DispatchDebugTextDrawing()
{
	if (textcmds.empty())
		return;

	static bool open = true;
	ImGui::SetNextWindowPos({ 0,0 }, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize, ImGuiCond_Always);
	ImGui::Begin("TEXT_RENDERING", &open, 
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoInputs|
		ImGuiWindowFlags_NoNav
	);

	Render::Camera* const cam = Render::CameraManager::GetCamera(CAMERA_MAIN);

	while (!textcmds.empty())
	{
		TextCommand& cmd = textcmds.front();

		// transform point into screenspace
		cmd.point.w = 1.0f;
		glm::vec4 ndc = cam->viewProjection * cmd.point;
		ndc /= ndc.w;

		if (ndc.z <= 1) // only render in front of camera
		{
			glm::vec2 cursorPos = { ndc.x, ndc.y };
			cursorPos += glm::vec2(1.0f, 1.0f);
			cursorPos *= 0.5f;
			cursorPos.y = 1.0f - cursorPos.y;
			cursorPos.x *= ImGui::GetWindowWidth();
			cursorPos.y *= ImGui::GetWindowHeight();
			// center text
			cursorPos.x -= ImGui::CalcTextSize(cmd.text.c_str()).x / 2.0f;

			ImGui::SetCursorPos({ cursorPos.x, cursorPos.y });
		
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(cmd.color.x, cmd.color.y, cmd.color.z, cmd.color.w));
			ImGui::TextUnformatted(cmd.text.c_str());
			ImGui::PopStyleColor();
		}
		textcmds.pop();
	}
	ImGui::End();
}

} // namespace Debug
