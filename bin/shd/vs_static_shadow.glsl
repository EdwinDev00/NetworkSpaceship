#version 430
layout(location=0) in vec3 in_Position;
layout(location=3) in vec2 in_TexCoord_0;

layout(location=3) out vec2 out_TexCoords;

uniform mat4 ViewProjection;
uniform mat4 Model;

invariant gl_Position;

void main()
{
	out_TexCoords = in_TexCoord_0;
	// BUG: this must be calculated EXACTLY the same way as in our vs_static shader, otherwise, we get zbuffer fighting since the write to gl_Position is not invariant.
	// 	    check out https://stackoverflow.com/a/46920273
	vec4 wPos = Model * vec4(in_Position, 1.0f);
	gl_Position = ViewProjection * wPos;
}
