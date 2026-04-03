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
    std::vector<float> lteRsrpData;
    std::vector<float> lteRsrqData;
    std::vector<float> lteRssiData;
    std::vector<int>   lteAsuData;
    std::vector<int>   lteCqiData;
    std::vector<float> nrSsRsrpData;
    std::vector<float> nrSsRsrqData;
    std::vector<float> nrSsSinrData;
    int lteCellId = 0;
    int lteEarfcn = 0;
    int lteMcc = 0;
    int lteMnc = 0;
    int ltePci = 0;
    int lteTac = 0;
    int lteAsuLevel = 0;
    int lteCqi = 0;
    int lteRsrp = 0;
    int lteRsrq = 0;
    int lteRssi = 0;
    int lteRssnr = 0;
    int lteTimingAdvance = 0;
    int gsmCellId = 0;
    int gsmBsic = 0;
    int gsmArfcn = 0;
    int gsmLac = 0;
    int gsmMcc = 0;
    int gsmMnc = 0;
    int gsmPsc = 0;
    int gsmDbm = 0;
    int gsmTimingAdvance = 0;
    int nrBand = 0;
    long nrNci = 0;
    int nrPci = 0;
    int nrNrarfcn = 0;
    int nrTac = 0;
    int nrMcc = 0;
    int nrMnc = 0;
    int nrSsRsrp = 0;
    int nrSsRsrq = 0;
    int nrSsSinr = 0;
    int nrTimingAdvance = 0;
    
    std::string networkOperator;
    std::string networkOperatorName;
    
    mutable std::mutex mtx;
};

template<typename T>
T getJsonValue(const json& j, const std::string& key, T defaultValue = T{}) {
    try {
        if (j.contains(key) && !j[key].is_null()) {
            return j[key].get<T>();
        }
    } catch (...) {}
    return defaultValue;
}

void run_server(LocationData* data) {
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::rep);
    sock.bind("tcp://*:5555");
    std::cout << "Сервер запущен на порту 5555\n";
    
    while (true) {
        try {
            zmq::message_t request;
            if (!sock.recv(request, zmq::recv_flags::none)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            std::string msg(static_cast<char*>(request.data()), request.size());
            auto jdata = json::parse(msg);
            
            {
                std::lock_guard<std::mutex> lock(data->mtx);
                
                data->latitude   = getJsonValue<float>(jdata, "latitude", 0.0f);
                data->longitude  = getJsonValue<float>(jdata, "longitude", 0.0f);
                data->altitude   = getJsonValue<float>(jdata, "altitude", 0.0f);
                data->accuracy   = getJsonValue<float>(jdata, "accuracy", 0.0f);
                data->timestamp  = getJsonValue<std::string>(jdata, "time", "");
                data->networkType = getJsonValue<std::string>(jdata, "networkType", "Unknown");
                data->networkOperator = getJsonValue<std::string>(jdata, "networkOperator", "");
                data->networkOperatorName = getJsonValue<std::string>(jdata, "networkOperatorName", "");
                
                data->lteCellId = getJsonValue<int>(jdata, "lteCellId", 0);
                data->lteEarfcn = getJsonValue<int>(jdata, "lteEarfcn", 0);
                data->lteMcc = getJsonValue<int>(jdata, "lteMcc", 0);
                data->lteMnc = getJsonValue<int>(jdata, "lteMnc", 0);
                data->ltePci = getJsonValue<int>(jdata, "ltePci", 0);
                data->lteTac = getJsonValue<int>(jdata, "lteTac", 0);
                data->lteAsuLevel = getJsonValue<int>(jdata, "lteAsuLevel", 0);
                data->lteCqi = getJsonValue<int>(jdata, "lteCqi", 0);
                data->lteRsrp = getJsonValue<int>(jdata, "lteRsrp", 0);
                data->lteRsrq = getJsonValue<int>(jdata, "lteRsrq", 0);
                data->lteRssi = getJsonValue<int>(jdata, "lteRssi", 0);
                data->lteRssnr = getJsonValue<int>(jdata, "lteRssnr", 0);
                data->lteTimingAdvance = getJsonValue<int>(jdata, "lteTimingAdvance", 0);
                
                data->gsmCellId = getJsonValue<int>(jdata, "gsmCellId", 0);
                data->gsmBsic = getJsonValue<int>(jdata, "gsmBsic", 0);
                data->gsmArfcn = getJsonValue<int>(jdata, "gsmArfcn", 0);
                data->gsmLac = getJsonValue<int>(jdata, "gsmLac", 0);
                data->gsmMcc = getJsonValue<int>(jdata, "gsmMcc", 0);
                data->gsmMnc = getJsonValue<int>(jdata, "gsmMnc", 0);
                data->gsmPsc = getJsonValue<int>(jdata, "gsmPsc", 0);
                data->gsmDbm = getJsonValue<int>(jdata, "gsmDbm", 0);
                data->gsmTimingAdvance = getJsonValue<int>(jdata, "gsmTimingAdvance", 0);
                
                data->nrBand = getJsonValue<int>(jdata, "nrBand", 0);
                data->nrNci = getJsonValue<long>(jdata, "nrNci", 0L);
                data->nrPci = getJsonValue<int>(jdata, "nrPci", 0);
                data->nrNrarfcn = getJsonValue<int>(jdata, "nrNrarfcn", 0);
                data->nrTac = getJsonValue<int>(jdata, "nrTac", 0);
                data->nrMcc = getJsonValue<int>(jdata, "nrMcc", 0);
                data->nrMnc = getJsonValue<int>(jdata, "nrMnc", 0);
                data->nrSsRsrp = getJsonValue<int>(jdata, "nrSsRsrp", 0);
                data->nrSsRsrq = getJsonValue<int>(jdata, "nrSsRsrq", 0);
                data->nrSsSinr = getJsonValue<int>(jdata, "nrSsSinr", 0);
                data->nrTimingAdvance = getJsonValue<int>(jdata, "nrTimingAdvance", 0);
                
                if (data->networkType == "LTE") {
                    data->lteRsrpData.push_back(static_cast<float>(data->lteRsrp));
                    data->lteRsrqData.push_back(static_cast<float>(data->lteRsrq));
                    data->lteRssiData.push_back(static_cast<float>(data->lteRssi));
                    data->lteAsuData.push_back(data->lteAsuLevel);
                    data->lteCqiData.push_back(data->lteCqi);
                    
                    if (data->lteRsrpData.size() > 100) {
                        data->lteRsrpData.erase(data->lteRsrpData.begin());
                        data->lteRsrqData.erase(data->lteRsrqData.begin());
                        data->lteRssiData.erase(data->lteRssiData.begin());
                        data->lteAsuData.erase(data->lteAsuData.begin());
                        data->lteCqiData.erase(data->lteCqiData.begin());
                    }
                }
                else if (data->networkType == "NR") {
                    data->nrSsRsrpData.push_back(static_cast<float>(data->nrSsRsrp));
                    data->nrSsRsrqData.push_back(static_cast<float>(data->nrSsRsrq));
                    data->nrSsSinrData.push_back(static_cast<float>(data->nrSsSinr));
                    
                    if (data->nrSsRsrpData.size() > 100) {
                        data->nrSsRsrpData.erase(data->nrSsRsrpData.begin());
                        data->nrSsRsrqData.erase(data->nrSsRsrqData.begin());
                        data->nrSsSinrData.erase(data->nrSsSinrData.begin());
                    }
                }
            }
            
            std::cout << "Данные: lat=" << data->latitude << " lon=" << data->longitude
                      << " net=" << data->networkType << " RSRP=" << data->lteRsrp << " dBm\n";
            {
                std::lock_guard<std::mutex> lock(data->mtx);
                json entry = {
                    {"latitude", data->latitude},
                    {"longitude", data->longitude},
                    {"altitude", data->altitude},
                    {"accuracy", data->accuracy},
                    {"time", data->timestamp},
                    {"networkType", data->networkType}
                };

                if (data->networkType == "LTE") {
                    entry["rsrp"] = data->lteRsrp;
                } else if (data->networkType == "NR") {
                    entry["rsrp"] = data->nrSsRsrp;
                } else {
                    entry["rsrp"] = 0;
                }
    
                std::ofstream file("locations.json", std::ios::app);
                if (file.is_open()) file << entry.dump() << "\n";
            }
            
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
    SDL_Window* window = SDL_CreateWindow("Location Monitor", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
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
        
        ImGui::Begin("Location");
        {
            std::lock_guard<std::mutex> lock(data->mtx);
            ImGui::Text("Latitude:  %.6f", data->latitude);
            ImGui::Text("Longitude: %.6f", data->longitude);
            ImGui::Text("Altitude:  %.1f m", data->altitude);
            ImGui::Text("Accuracy:  %.1f m", data->accuracy);
            ImGui::Text("Time:      %s", data->timestamp.c_str());
            ImGui::Text("Network:   %s", data->networkType.c_str());
            ImGui::Text("Operator:  %s (%s)", data->networkOperatorName.c_str(), data->networkOperator.c_str());
        }
        ImGui::End();
        
        ImGui::Begin("Network Details");
        {
            std::lock_guard<std::mutex> lock(data->mtx);
            if (data->networkType == "LTE") {
                ImGui::Text("Cell ID:   %d", data->lteCellId);
                ImGui::Text("EARFCN:    %d", data->lteEarfcn);
                ImGui::Text("PCI:       %d", data->ltePci);
                ImGui::Text("TAC:       %d", data->lteTac);
                ImGui::Text("MCC/MNC:   %d / %d", data->lteMcc, data->lteMnc);
                ImGui::Text("ASU Level: %d", data->lteAsuLevel);
                ImGui::Text("CQI:       %d", data->lteCqi);
                ImGui::Text("RSRP:      %d dBm", data->lteRsrp);
                ImGui::Text("RSRQ:      %d dB", data->lteRsrq);
                ImGui::Text("RSSI:      %d dBm", data->lteRssi);
                ImGui::Text("RSSNR:     %d dB", data->lteRssnr);
                ImGui::Text("Timing Adv:%d", data->lteTimingAdvance);
            } else if (data->networkType == "GSM") {
                ImGui::Text("Cell ID:   %d", data->gsmCellId);
                ImGui::Text("ARFCN:     %d", data->gsmArfcn);
                ImGui::Text("BSIC:      %d", data->gsmBsic);
                ImGui::Text("LAC:       %d", data->gsmLac);
                ImGui::Text("PSC:       %d", data->gsmPsc);
                ImGui::Text("MCC/MNC:   %d / %d", data->gsmMcc, data->gsmMnc);
                ImGui::Text("DBM:       %d dBm", data->gsmDbm);
                ImGui::Text("Timing Adv:%d", data->gsmTimingAdvance);
            } else if (data->networkType == "NR") {
                ImGui::Text("Band:      %d", data->nrBand);
                ImGui::Text("NCI:       %ld", data->nrNci);
                ImGui::Text("PCI:       %d", data->nrPci);
                ImGui::Text("NR-ARFCN:  %d", data->nrNrarfcn);
                ImGui::Text("TAC:       %d", data->nrTac);
                ImGui::Text("MCC/MNC:   %d / %d", data->nrMcc, data->nrMnc);
                ImGui::Text("SS-RSRP:   %d dBm", data->nrSsRsrp);
                ImGui::Text("SS-RSRQ:   %d dB", data->nrSsRsrq);
                ImGui::Text("SS-SINR:   %d dB", data->nrSsSinr);
                ImGui::Text("Timing Adv:%d", data->nrTimingAdvance);
            } else {
                ImGui::Text("No network data available");
            }
        }
        ImGui::End();
        
        ImGui::Begin("Signal Graphs");
        {
            std::lock_guard<std::mutex> lock(data->mtx);
            if (data->networkType == "LTE" && !data->lteRsrpData.empty()) {
                if (ImPlot::BeginPlot("LTE RSRP / RSRQ", "Time", "dBm", ImVec2(-1, 250))) {
                    ImPlot::PlotLine("RSRP", data->lteRsrpData.data(), data->lteRsrpData.size());
                    ImPlot::PlotLine("RSRQ", data->lteRsrqData.data(), data->lteRsrqData.size());
                    ImPlot::EndPlot();
                }
                if (ImPlot::BeginPlot("LTE RSSI / ASU", "Time", "dBm / level", ImVec2(-1, 250))) {
                    ImPlot::PlotLine("RSSI", data->lteRssiData.data(), data->lteRssiData.size());
                    ImPlot::PlotLine("ASU",  data->lteAsuData.data(), data->lteAsuData.size());
                    ImPlot::EndPlot();
                }
                if (ImPlot::BeginPlot("LTE CQI", "Time", "CQI", ImVec2(-1, 250))) {
                    ImPlot::PlotLine("CQI", data->lteCqiData.data(), data->lteCqiData.size());
                    ImPlot::EndPlot();
                }
            } else if (data->networkType == "NR" && !data->nrSsRsrpData.empty()) {
                if (ImPlot::BeginPlot("NR Signal", "Time", "dBm / dB", ImVec2(-1, 400))) {
                    ImPlot::PlotLine("SS-RSRP", data->nrSsRsrpData.data(), data->nrSsRsrpData.size());
                    ImPlot::PlotLine("SS-RSRQ", data->nrSsRsrqData.data(), data->nrSsRsrqData.size());
                    ImPlot::PlotLine("SS-SINR", data->nrSsSinrData.data(), data->nrSsSinrData.size());
                    ImPlot::EndPlot();
                }
            } else {
                ImGui::Text("No data");
            }
        }
        ImGui::End();
        
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

int main() {
    LocationData locationData;
    std::thread server_thread(run_server, &locationData);
    run_gui(&locationData);
    server_thread.join();
    return 0;
}