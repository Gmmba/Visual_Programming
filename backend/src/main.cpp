#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <fstream>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"
#include "implot.h"
#include "zmq.hpp"
#include "json.hpp"

using json = nlohmann::json;

struct Location {
    float latitude = 0.0f;
    float longitude = 0.0f;
    float altitude = 0.0f;
    std::string timestamp;
    mutable std::mutex mtx;
};

void run_server(Location* loc) {
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::rep);
    sock.bind("tcp://*:5555");

    std::cout <<"сервер запущен на порту 5555" << std::endl;

    while (true) {
        try {
            zmq::message_t request;
            if (!sock.recv(request)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            std::string msg(static_cast<char*>(request.data()), request.size());
            auto data = json::parse(msg);

            {
                std::lock_guard<std::mutex> lock(loc->mtx);
                loc->latitude = data["latitude"];
                loc->longitude = data["longitude"];
                loc->altitude = data["altitude"];
                loc->timestamp = data["time"];
            }

            std::cout << "Получено: lat=" << loc->latitude 
                      << ", lon=" << loc->longitude 
                      << ", alt=" << loc->altitude << std::endl;
                      
            json entry = {
                {"latitude", loc->latitude},
                {"longitude", loc->longitude},
                {"altitude", loc->altitude},
                {"time", loc->timestamp}
            };

            std::ofstream file("locations.json", std::ios::app);
            file << entry.dump(4) << ",\n";
            file.close();

            std::string reply = "ACK";
            sock.send(zmq::buffer(reply), zmq::send_flags::none);

        } catch (const std::exception& e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

void run_gui(Location* loc) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window* window = SDL_CreateWindow(
        "Location Monitor", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1024, 768, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);

        {
            ImGui::Begin("Location");
            float lat, lon, alt;
            std::string time_str;
            {
                std::lock_guard<std::mutex> lock(loc->mtx);
                lat = loc->latitude;
                lon = loc->longitude;
                alt = loc->altitude;
                time_str = loc->timestamp;
            }

            ImGui::Text("Latitude: %.6f", lat);
            ImGui::Text("Longitude: %.6f", lon);
            ImGui::Text("Height: %.1f m", alt);
            ImGui::Text("Time: %s", time_str.c_str());
            ImGui::End();
        }

        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    static Location locationInfo;
    std::thread server_thread(run_server, &locationInfo);
    run_gui(&locationInfo);
    server_thread.join();

    return 0;
}