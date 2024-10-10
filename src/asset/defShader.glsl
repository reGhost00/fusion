#shader vertex
#version 330 core
#extension GL_ARB_bindless_texture : require
layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 textureCoord;
out vec4 rgb;
void main()
{
   rgb = color;
   gl_Position = position;
}



    static const char* const defFragmentShaders[] = {
        "#version 330 core\n"
        "in vec4 rgb;\n"
        "out vec4 color;\n"
        "void main()\n"
        "{\n"
        "   color = rgb;\n"
        "}\0"
    };