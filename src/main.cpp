#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define GLFW_INCLUDE_ES3
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <vector>
#include <sstream>

GLFWwindow *g_window;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
ImGuiContext *imgui = 0;
bool show_demo_window = true;
bool show_another_window = true;
std::vector<std::string> Items;
ImGuiTextFilter Filter;

bool m_shouldOutputData = true;
bool m_shouldAutoScroll = true;
bool m_shouldScrollToBottom = true;
bool m_isRegistered = false;
static char buf1[64] = "";

void clearLog();
void sendCommand();
void debugMessageHandler(const std::string &msg);

EM_JS(int, canvas_get_width, (), {
    return Module.canvas.width;
});

EM_JS(int, canvas_get_height, (), {
    return Module.canvas.height;
});

EM_JS(void, resizeCanvas, (), {
    js_resizeCanvas();
});

void loop()
{
    int width = canvas_get_width();
    int height = canvas_get_height();

    glfwSetWindowSize(g_window, width, height);

    ImGui::SetCurrentContext(imgui);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        std::stringstream ss;
        ss << "Application average " << 1000.0f / ImGui::GetIO().Framerate << "ms/frame (" << ImGui::GetIO().Framerate << " FPS)";
        debugMessageHandler(ss.str());
    }

    {

        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("Server Console");

        if (ImGui::Button("Clear"))
        {
            clearLog();
        }

        ImGui::SameLine();

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &m_shouldAutoScroll);
            ImGui::EndPopup();
        }

        // Options, Filter
        if (ImGui::Button("Options"))
        {
            ImGui::OpenPopup("Options");
        }

        ImGui::Separator();

        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear"))
            {
                clearLog();
            }
            ImGui::EndPopup();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

        for (unsigned int i = 0; i < Items.size(); i++)
        {
            const char *item = Items[i].c_str();
            ImGui::TextUnformatted(item);
        }

        if (m_shouldScrollToBottom || (m_shouldAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        {
            ImGui::SetScrollHere(1.0f);
        }

        m_shouldScrollToBottom = false;

        ImGui::PopStyleVar();
        ImGui::EndChild();


        ImGui::SetKeyboardFocusHere(0);        
        ImGui::InputText("", buf1, 64);
        ImGui::SameLine();


        if (ImGui::Button("Send") || ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Enter]))
        {
            sendCommand();
        }

        ImGui::End();
    }

    ImGui::Render();

    int display_w, display_h;
    glfwMakeContextCurrent(g_window);
    glfwGetFramebufferSize(g_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwMakeContextCurrent(g_window);
}

void clearLog()
{
    Items.clear();
}

void sendCommand()
{
    Items.push_back(buf1);
    *buf1 = 0;
}

void debugMessageHandler(const std::string &msg)
{
    Items.push_back(msg);
}

int init()
{
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }

    //glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

    // Open a window and create its OpenGL context
    int canvasWidth = 800;
    int canvasHeight = 600;
    g_window = glfwCreateWindow(canvasWidth, canvasHeight, "WebGui Demo", NULL, NULL);
    if (g_window == NULL)
    {
        fprintf(stderr, "Failed to open GLFW window.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(g_window); // Initialize GLEW

    // Create game objects
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplGlfw_InitForOpenGL(g_window, false);
    ImGui_ImplOpenGL3_Init();

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    io.Fonts->AddFontDefault();

    imgui = ImGui::GetCurrentContext();

    // Cursor callbacks
    glfwSetMouseButtonCallback(g_window, ImGui_ImplGlfw_MouseButtonCallback);
    glfwSetScrollCallback(g_window, ImGui_ImplGlfw_ScrollCallback);
    glfwSetKeyCallback(g_window, ImGui_ImplGlfw_KeyCallback);
    glfwSetCharCallback(g_window, ImGui_ImplGlfw_CharCallback);

    resizeCanvas();

    return 0;
}

void quit()
{
    glfwTerminate();
}

extern "C" int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (init() != 0)
        return 1;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop, 0, 1);
#endif

    quit();

    return 0;
}
