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

struct LocationData {
    float latitude = 0.0f;
    float longitude = 0.0f;
    float altitude = 0.0f;
    float accuracy = 0.0f;
    std::string timestamp;
    std::string networkType;
    int lteBand = -1;
    int lteCellId = -1;
    int lteEarfcn = -1;
    int lteMcc = -1;
    int lteMnc = -1;
    int ltePci = -1;
    int lteTac = -1;
    int lteAsuLevel = -1;
    int lteCqi = -1;
    int lteRsrp = -1;
    int lteRsrq = -1;
    int lteRssi = -1;
    int lteRssnr = -1;
    int lteTimingAdvance = -1;
    int gsmCellId = -1;
    int gsmBsic = -1;
    int gsmArfcn = -1;
    int gsmLac = -1;
    int gsmMcc = -1;
    int gsmMnc = -1;
    int gsmPsc = -1;
    int gsmDbm = -1;
    int gsmRssi = -1;
    int gsmTimingAdvance = -1;
    int nrBand = -1;
    long nrNci = -1;
    int nrPci = -1;
    int nrNrarfcn = -1;
    int nrTac = -1;
    int nrMcc = -1;
    int nrMnc = -1;
    int nrSsRsrp = -1;
    int nrSsRsrq = -1;
    int nrSsSinr = -1;
    int nrTimingAdvance = -1;
    std::string networkOperator;
    std::string networkOperatorName;
    mutable std::mutex mtx;
};

void run_server(LocationData* data) {
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::rep);
    sock.bind("tcp://*:5555");
    std::cout << "сервер запущен на порту 5555" << std::endl;

    while (true) {
        try {
            zmq::message_t request;
            if (!sock.recv(request)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            std::string msg(static_cast<char*>(request.data()), request.size());
            auto jdata = json::parse(msg);

            {
                std::lock_guard<std::mutex> lock(data->mtx);
                data->latitude = jdata.value("latitude", 0.0f);
                data->longitude = jdata.value("longitude", 0.0f);
                data->altitude = jdata.value("altitude", 0.f);
                data->accuracy = jdata.value("accuracy", 0.0f);
                data->timestamp = jdata.value("time", "");
                data->networkType = jdata.value("networkType", "Unknown");
                data->lteBand = jdata.value("lteBand", -1);
                data->lteCellId = jdata.value("lteCellId", -1);
                data->lteEarfcn = jdata.value("lteEarfcn", -1);
                data->lteMcc = jdata.value("lteMcc", -1);
                data->lteMnc = jdata.value("lteMnc", -1);
                data->ltePci = jdata.value("ltePci", -1);
                data->lteTac = jdata.value("lteTac", -1);
                data->lteAsuLevel = jdata.value("lteAsuLevel", -1);
                data->lteCqi = jdata.value("lteCqi", -1);
                data->lteRsrp = jdata.value("lteRsrp", -1);
                data->lteRsrq = jdata.value("lteRsrq", -1);
                data->lteRssi = jdata.value("lteRssi", -1);
                data->lteRssnr = jdata.value("lteRssnr", -1);
                data->lteTimingAdvance = jdata.value("lteTimingAdvance", -1);
                data->gsmCellId = jdata.value("gsmCellId", -1);
                data->gsmBsic = jdata.value("gsmBsic", -1);
                data->gsmArfcn = jdata.value("gsmArfcn", -1);
                data->gsmLac = jdata.value("gsmLac", -1);
                data->gsmMcc = jdata.value("gsmMcc", -1);
                data->gsmMnc = jdata.value("gsmMnc", -1);
                data->gsmPsc = jdata.value("gsmPsc", -1);
                data->gsmDbm = jdata.value("gsmDbm", -1);
                data->gsmRssi = jdata.value("gsmRssi", -1);
                data->gsmTimingAdvance = jdata.value("gsmTimingAdvance", -1);
                data->nrBand = jdata.value("nrBand", -1);
                data->nrNci = jdata.value("nrNci", -1L);
                data->nrPci = jdata.value("nrPci", -1);
                data->nrNrarfcn = jdata.value("nrNrarfcn", -1);
                data->nrTac = jdata.value("nrTac", -1);
                data->nrMcc = jdata.value("nrMcc", -1);
                data->nrMnc = jdata.value("nrMnc", -1);
                data->nrSsRsrp = jdata.value("nrSsRsrp", -1);
                data->nrSsRsrq = jdata.value("nrSsRsrq", -1);
                data->nrSsSinr = jdata.value("nrSsSinr", -1);
                data->nrTimingAdvance = jdata.value("nrTimingAdvance", -1);
                data->networkOperator = jdata.value("networkOperator", "");
                data->networkOperatorName = jdata.value("networkOperatorName", "");
            }

            std::cout << "Получено: lat=" << data->latitude 
                      << ", lon=" << data->longitude 
                      << ", network=" << data->networkType << std::endl;

            json entry;
            {
                std::lock_guard<std::mutex> lock(data->mtx);
                entry = {
                    {"latitude", data->latitude},
                    {"longitude", data->longitude},
                    {"altitude", data->altitude},
                    {"accuracy", data->accuracy},
                    {"time", data->timestamp},
                    {"networkType", data->networkType},
                    {"networkOperator", data->networkOperator},
                    {"networkOperatorName", data->networkOperatorName},
                    {"lteBand", data->lteBand},
                    {"lteCellId", data->lteCellId},
                    {"lteEarfcn", data->lteEarfcn},
                    {"lteMcc", data->lteMcc},
                    {"lteMnc", data->lteMnc},
                    {"ltePci", data->ltePci},
                    {"lteTac", data->lteTac},
                    {"lteAsuLevel", data->lteAsuLevel},
                    {"lteCqi", data->lteCqi},
                    {"lteRsrp", data->lteRsrp},
                    {"lteRsrq", data->lteRsrq},
                    {"lteRssi", data->lteRssi},
                    {"lteRssnr", data->lteRssnr},
                    {"lteTimingAdvance", data->lteTimingAdvance},
                    {"gsmCellId", data->gsmCellId},
                    {"gsmBsic", data->gsmBsic},
                    {"gsmArfcn", data->gsmArfcn},
                    {"gsmLac", data->gsmLac},
                    {"gsmMcc", data->gsmMcc},
                    {"gsmMnc", data->gsmMnc},
                    {"gsmPsc", data->gsmPsc},
                    {"gsmDbm", data->gsmDbm},
                    {"gsmRssi", data->gsmRssi},
                    {"gsmTimingAdvance", data->gsmTimingAdvance},
                    {"nrBand", data->nrBand},
                    {"nrNci", data->nrNci},
                    {"nrPci", data->nrPci},
                    {"nrNrarfcn", data->nrNrarfcn},
                    {"nrTac", data->nrTac},
                    {"nrMcc", data->nrMcc},
                    {"nrMnc", data->nrMnc},
                    {"nrSsRsrp", data->nrSsRsrp},
                    {"nrSsRsrq", data->nrSsRsrq},
                    {"nrSsSinr", data->nrSsSinr},
                    {"nrTimingAdvance", data->nrTimingAdvance}
                };
            }

            std::ofstream file("locations.json", std::ios::app);
            file << entry.dump(2) << ",\n";
            file.close();

            std::string reply = "ACK";
            sock.send(zmq::buffer(reply), zmq::send_flags::none);

        } catch (const std::exception& e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

void run_gui(LocationData* data) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window* window = SDL_CreateWindow(
        "Location Monitor", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 800, 
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
            if (event.type == SDL_QUIT) running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);

        {
            ImGui::Begin("Location");
            float lat, lon, alt, acc;
            std::string time_str;
            {
                std::lock_guard<std::mutex> lock(data->mtx);
                lat = data->latitude;
                lon = data->longitude;
                alt = data->altitude;
                acc = data->accuracy;
                time_str = data->timestamp;
            }
            ImGui::Text("Latitude: %.6f", lat);
            ImGui::Text("Longitude: %.6f", lon);
            ImGui::Text("Altitude: %.1f m", alt);
            ImGui::Text("Accuracy: %.1f m", acc);
            ImGui::Text("Time: %s", time_str.c_str());
            ImGui::End();
        }

        {
            ImGui::Begin("Network");
            std::string netType, netOp, netOpName;
            {
                std::lock_guard<std::mutex> lock(data->mtx);
                netType = data->networkType;
                netOp = data->networkOperator;
                netOpName = data->networkOperatorName;
            }
            ImGui::Text("Network Type: %s", netType.c_str());
            ImGui::Text("Operator: %s", netOpName.c_str());
            ImGui::Text("MCC-MNC: %s", netOp.c_str());

            if (netType == "LTE") {
                ImGui::Text("LTE Band: %d", data->lteBand);
                ImGui::Text("EARFCN: %d", data->lteEarfcn);
                ImGui::Text("Cell ID: %d", data->lteCellId);
                ImGui::Text("PCI: %d", data->ltePci);
                ImGui::Text("TAC: %d", data->lteTac);
                ImGui::Text("MCC: %d", data->lteMcc);
                ImGui::Text("MNC: %d", data->lteMnc);
                ImGui::Text("ASU Level: %d", data->lteAsuLevel);
                ImGui::Text("CQI: %d", data->lteCqi);
                ImGui::Text("RSRP: %d dBm", data->lteRsrp);
                ImGui::Text("RSRQ: %d dB", data->lteRsrq);
                ImGui::Text("RSSI: %d dBm", data->lteRssi);
                ImGui::Text("RSSNR: %d dB", data->lteRssnr);
                ImGui::Text("Timing Advance: %d", data->lteTimingAdvance);
            } else if (netType == "GSM") {
                ImGui::Text("ARFCN: %d", data->gsmArfcn);
                ImGui::Text("Cell ID: %d", data->gsmCellId);
                ImGui::Text("BSIC: %d", data->gsmBsic);
                ImGui::Text("LAC: %d", data->gsmLac);
                ImGui::Text("PSC: %d", data->gsmPsc);
                ImGui::Text("MCC: %d", data->gsmMcc);
                ImGui::Text("MNC: %d", data->gsmMnc);
                ImGui::Text("Dbm: %d dBm", data->gsmDbm);
                ImGui::Text("RSSI: %d dBm", data->gsmRssi);
                ImGui::Text("Timing Advance: %d", data->gsmTimingAdvance);
            } else if (netType == "NR") {
                ImGui::Text("Band: %d", data->nrBand);
                ImGui::Text("NR-ARFCN: %d", data->nrNrarfcn);
                ImGui::Text("NCI: %ld", data->nrNci);
                ImGui::Text("PCI: %d", data->nrPci);
                ImGui::Text("TAC: %d", data->nrTac);
                ImGui::Text("MCC: %d", data->nrMcc);
                ImGui::Text("MNC: %d", data->nrMnc);
                ImGui::Text("SS-RSRP: %d dBm", data->nrSsRsrp);
                ImGui::Text("SS-RSRQ: %d dB", data->nrSsRsrq);
                ImGui::Text("SS-SINR: %d dB", data->nrSsSinr);
                ImGui::Text("Timing Advance: %d", data->nrTimingAdvance);
            }
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
    static LocationData locationData;
    std::thread server_thread(run_server, &locationData);
    run_gui(&locationData);
    server_thread.join();
    return 0;
}