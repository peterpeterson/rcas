//
// RCAS WebAssembly/ImGui UI
// Peter Peterson <peter@saborgato.com>
//
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/websocket.h>
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

std::vector<std::string> Chats;
static char m_chat[1024] = "";

std::vector<std::string> Items;
static char m_command[1024] = "";

bool m_shouldOutputData = true;
bool m_shouldAutoScroll = true;
bool m_shouldScrollToBottom = true;
bool m_resetFocusAfterEnter = false;
bool m_resetChatFocusAfterEnter = false;


void clearLog();
void sendCommand(const char* command);
void sendChat(const char* chat);
void logMessageHandler(const std::string& message);
void chatMessageHandler(const std::string& message);

EMSCRIPTEN_WEBSOCKET_T sendSocket;
EMSCRIPTEN_WEBSOCKET_T chatSocket;

EM_BOOL webSocketOpen(int eventType, const EmscriptenWebSocketOpenEvent *e, void *userData)
{
    (void) e;
	printf("open(eventType=%d, userData=%d)\n", eventType, (int)userData);
	return 0;
}

EM_BOOL webSocketClose(int eventType, const EmscriptenWebSocketCloseEvent *e, void *userData)
{
	printf("close(eventType=%d, wasClean=%d, code=%d, reason=%s, userData=%d)\n", eventType, e->wasClean, e->code, e->reason, (int)userData);
	return 0;
}

EM_BOOL webSocketError(int eventType, const EmscriptenWebSocketErrorEvent *e, void *userData)
{
    (void) e;
	printf("error(eventType=%d, userData=%d)\n", eventType, (int)userData);
	return 0;
}

EM_BOOL webSocketMessage(int eventType, const EmscriptenWebSocketMessageEvent *e, void *userData)
{
	printf("message(eventType=%d, userData=%d, data=%s, numBytes=%d, isText=%d)\n", eventType, (int)userData, e->data, e->numBytes, e->isText);
	if (e->isText)
    {
        std::stringstream ss;
        ss << e->data;
        logMessageHandler(ss.str());
    }
	
	return 0;
}

EM_BOOL chatWebSocketMessage(int eventType, const EmscriptenWebSocketMessageEvent *e, void *userData)
{
	printf("chat message(eventType=%d, userData=%d, data=%s, numBytes=%d, isText=%d)\n", eventType, (int)userData, e->data, e->numBytes, e->isText);
	if (e->isText)
    {
        std::stringstream ss;
        ss << e->data;
        chatMessageHandler(ss.str());
    }
	
	return 0;
}

EM_JS(int, canvas_get_width, (), {
    return Module.canvas.width;
});

EM_JS(int, canvas_get_height, (), {
    return Module.canvas.height;
});

EM_JS(void, resizeCanvas, (), {
    js_resizeCanvas();
});

EM_JS(const char*, get_hostname, (), {
    var jsString = window.location.hostname;
    var lengthBytes = lengthBytesUTF8(jsString)+1;
    var stringOnWasmHeap = _malloc(lengthBytes);
    stringToUTF8(jsString, stringOnWasmHeap, lengthBytes);
    return stringOnWasmHeap;
});

// Helper to display a little (?) mark which shows a tooltip when hovered.
static void ShowHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

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
        ImGui::SetNextWindowSize(ImVec2(200, height), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(width - 200, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("Chat");

        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

        for (unsigned int i = 0; i < Chats.size(); i++)
        {
            const char *item = Chats[i].c_str();
            ImGui::TextUnformatted(item);
        }

        if (m_shouldScrollToBottom || (m_shouldAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        {
            ImGui::SetScrollHere(1.0f);
        }

        m_shouldScrollToBottom = false;

        ImGui::PopStyleVar();
        ImGui::EndChild();


        if (m_resetChatFocusAfterEnter)
        {
            m_resetChatFocusAfterEnter = false;
            ImGui::SetKeyboardFocusHere(0);        
        }
        ImGui::InputText("", m_chat, 1024);
        ImGui::SameLine();


        if (ImGui::Button("Send") || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Enter])))
        {
            sendChat(m_chat);
            m_resetChatFocusAfterEnter = true;
        }

        ImGui::End();
    }

    {
        ImGui::SetNextWindowSize(ImVec2(width - 200, height), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("RCAS");

        // TODO: move to separate files -- command pallete
        {
            const char* items[] = { "None", "Minecraft Bedrock" };
            static int item_current = 1;
            ImGui::Combo("command palete", &item_current, items, IM_ARRAYSIZE(items));
            ImGui::SameLine(); ShowHelpMarker("Please select from the available command palete\n");
            // bedrock buttons
            if (item_current == 1)
            {
                ImGui::Text("Time");
                ImGui::SameLine();

                if (ImGui::Button("Day"))
                {
                    sendCommand("time set day");
                }

                ImGui::SameLine();

                if (ImGui::Button("Night"))
                {
                    sendCommand("time set night");
                }


                ImGui::SameLine();
                ImGui::Text("Weather");
                ImGui::SameLine();

                if (ImGui::Button("Clear"))
                {
                    sendCommand("weather clear");
                }

                ImGui::SameLine();

                if (ImGui::Button("Rainy"))
                {
                    sendCommand("weather rain");
                }

                ImGui::SameLine();

                if (ImGui::Button("Thunder"))
                {
                    sendCommand("weather thunder");
                }

                ImGui::SameLine();
                ImGui::Text("Game Mode");
                ImGui::SameLine();

                if (ImGui::Button("Creative All"))
                {
                    sendCommand("gamemode creative @a");
                }

                ImGui::SameLine();

                if (ImGui::Button("Survival All"))
                {
                    sendCommand("gamemode survival @a");
                }
            }

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


        if (m_resetFocusAfterEnter)
        {
            m_resetFocusAfterEnter = false;
            ImGui::SetKeyboardFocusHere(0);        
        }
        ImGui::InputText("", m_command, 1024);
        ImGui::SameLine();


        if (ImGui::Button("Send") || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Enter])))
        {
            sendCommand(m_command);
            m_resetFocusAfterEnter = true;
        }

        ImGui::SameLine();

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

        ImGui::SameLine();
        ImGui::Text("client: 1.0.0-beta.1");

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

void sendCommand(const char* command)
{
    emscripten_websocket_send_utf8_text(sendSocket, command);
    *m_command = 0;
}

void sendChat(const char* chat)
{
    emscripten_websocket_send_utf8_text(chatSocket, chat);
    *m_chat = 0;
}

void logMessageHandler(const std::string& message)
{
    Items.push_back(message);
}

void chatMessageHandler(const std::string& message)
{
    Chats.push_back(message);
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
    g_window = glfwCreateWindow(canvasWidth, canvasHeight, "RCAS", NULL, NULL);
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

    auto port = 9001;

    {
        // console websocket
        EmscriptenWebSocketCreateAttributes attr;
        emscripten_websocket_init_create_attributes(&attr);

        std::stringstream hostnamess;

        hostnamess << "ws://" << (char*) get_hostname() << ":" << port << "/console";
        const char* pszHostname = strdup(hostnamess.str().c_str());
        attr.url = pszHostname;
        printf("%s\n", attr.url);

        std::stringstream connectmsg;
    
        connectmsg << "Connecting to RCAS at " << attr.url;

        logMessageHandler(connectmsg.str());

        sendSocket = emscripten_websocket_new(&attr);

        emscripten_websocket_set_onopen_callback(sendSocket, (void*)42, webSocketOpen);
        emscripten_websocket_set_onclose_callback(sendSocket, (void*)43, webSocketClose);
        emscripten_websocket_set_onerror_callback(sendSocket, (void*)44, webSocketError);
        emscripten_websocket_set_onmessage_callback(sendSocket, (void*)45, webSocketMessage);
    }

    {
        // chat websocket
        EmscriptenWebSocketCreateAttributes attr;
        emscripten_websocket_init_create_attributes(&attr);

        std::stringstream hostnamess;

        hostnamess << "ws://" << (char*) get_hostname() << ":" << port << "/chat";
        const char* pszHostname = strdup(hostnamess.str().c_str());
        attr.url = pszHostname;
        printf("%s\n", attr.url);

        std::stringstream connectmsg;
    
        connectmsg << "Connecting to RCAS chat at " << attr.url;

        chatMessageHandler(connectmsg.str());

        chatSocket = emscripten_websocket_new(&attr);

        emscripten_websocket_set_onopen_callback(chatSocket, (void*)42, webSocketOpen);
        emscripten_websocket_set_onclose_callback(chatSocket, (void*)43, webSocketClose);
        emscripten_websocket_set_onerror_callback(chatSocket, (void*)44, webSocketError);
        emscripten_websocket_set_onmessage_callback(chatSocket, (void*)45, chatWebSocketMessage);
    }


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
