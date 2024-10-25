#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <mimalloc.h>
#include <stdio.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char* vertexShaderSource = "#version 460 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "layout (location = 1) in vec3 aColor;\n"
                                 "out vec3 oColor;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                 "   oColor = aColor;\n"
                                 "}\0";
const char* fragmentShaderSource = "#version 460 core\n"
                                   "in vec3 oColor;\n"
                                   "out vec4 fragColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   fragColor = vec4(oColor, 1.0f);\n"
                                   "}\n\0";

typedef struct _DrawData {
    uint32_t VAO, VBO, EBO;
    uint32_t numIndices;
    uint32_t shaderProgram;
} DrawData;

void init_shader(DrawData* drawData)
{
    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n");
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n");
    }

    // link shaders
    drawData->shaderProgram = glCreateProgram();
    glAttachShader(drawData->shaderProgram, vertexShader);
    glAttachShader(drawData->shaderProgram, fragmentShader);
    glLinkProgram(drawData->shaderProgram);
    // check for linking errors
    glGetProgramiv(drawData->shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(drawData->shaderProgram, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n");
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void init_vertices(DrawData* drawData)
{
    float vertices[] = {
        0.5f, 0.5f, 0.0f, // top right
        0.5f, -0.5f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f, // bottom left
        -0.5f, 0.5f, 0.0f // top left
    };

    unsigned int indices[] = {
        // note that we start from 0!
        0, 1, 3, // first Triangle
        1, 2, 3 // second Triangle
    };
    drawData->numIndices = sizeof(indices) / sizeof(indices[0]);
    glGenVertexArrays(1, &drawData->VAO);
    glGenBuffers(1, &drawData->VBO);
    glGenBuffers(1, &drawData->EBO);
    glBindVertexArray(drawData->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, drawData->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawData->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void init_vertices_dsa(DrawData* drawData)
{
    float vertices[] = {
        0.5f, 0.5f, 0.0f, 0.9f, 0.8f, 0.1f, // top right
        0.5f, -0.5f, 0.0f, 0.1f, 0.2f, 0.9f, // bottom right
        -0.5f, -0.5f, 0.0f, 0.1f, 0.9f, 0.8f, // bottom left
        -0.5f, 0.5f, 0.0f, 0.8f, 0.1f, 0.9f // top left
    };

    unsigned int indices[] = {
        // note that we start from 0!
        0, 1, 2, // first Triangle
        0, 2, 3 // second Triangle
    };
    drawData->numIndices = sizeof(indices) / sizeof(indices[0]);
    glCreateVertexArrays(1, &drawData->VAO);
    glCreateBuffers(1, &drawData->VBO);
    glCreateBuffers(1, &drawData->EBO);

    glNamedBufferData(drawData->VBO, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glNamedBufferData(drawData->EBO, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexArrayAttrib(drawData->VAO, 0);
    glVertexArrayAttribBinding(drawData->VAO, 0, 0);
    glVertexArrayAttribFormat(drawData->VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);

    glEnableVertexArrayAttrib(drawData->VAO, 1);
    glVertexArrayAttribBinding(drawData->VAO, 1, 0);
    glVertexArrayAttribFormat(drawData->VAO, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));

    glVertexArrayVertexBuffer(drawData->VAO, 0, drawData->VBO, 0, 6 * sizeof(float));
    glVertexArrayElementBuffer(drawData->VAO, drawData->EBO);
}

static uint32_t load_shader(const char* path, char** dest)
{
    FILE* file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    *dest = (char*)mi_malloc(size);
    fread(*dest, 1, size, file);

    fclose(file);
    return size;
}

static void init_shader_spirv(DrawData* drawData)
{
    printf("%s\n", __func__);
    char *vertexShaderSource, *fragmentShaderSource;
    const uint32_t vertexShaderSize = load_shader("D:/dev/C/fusion/test/gl1/vert.spv", &vertexShaderSource);
    const uint32_t fragmentShaderSize = load_shader("D:/dev/C/fusion/test/gl1/frag.spv", &fragmentShaderSource);

    // 创建着色器
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &vertShader, GL_SHADER_BINARY_FORMAT_SPIR_V, vertexShaderSource, vertexShaderSize);
    glSpecializeShader(vertShader, "main", 0, NULL, NULL);

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderBinary(1, &fragShader, GL_SHADER_BINARY_FORMAT_SPIR_V, fragmentShaderSource, fragmentShaderSize);
    glSpecializeShader(fragShader, "main", 0, NULL, NULL);

    // 检查编译错误
    int success;
    char infoLog[512];
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR: Vertex Shader Compilation Failed\n%s\n", infoLog);
    }

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR: Fragment Shader Compilation Failed\n%s\n", infoLog);
    }

    // 链接着色器程序
    drawData->shaderProgram = glCreateProgram();
    glAttachShader(drawData->shaderProgram, vertShader);
    glAttachShader(drawData->shaderProgram, fragShader);
    glLinkProgram(drawData->shaderProgram);

    // 检查链接错误
    glGetProgramiv(drawData->shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(drawData->shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "ERROR: Shader Program Linking Failed\n%s\n", infoLog);
    }

    // 删除着色器
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    mi_free(vertexShaderSource);
    mi_free(fragmentShaderSource);
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGL(glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    // build and compile our shader program
    // ------------------------------------
    DrawData drawData;
    init_shader_spirv(&drawData);
    init_vertices_dsa(&drawData);

    // uncomment this call to draw in wireframe polygons.
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw our first triangle
        glUseProgram(drawData.shaderProgram);
        glBindVertexArray(drawData.VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        // glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // glBindVertexArray(0); // no need to unbind it every time

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &drawData.VAO);
    glDeleteBuffers(1, &drawData.VBO);
    glDeleteBuffers(1, &drawData.EBO);
    glDeleteProgram(drawData.shaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}