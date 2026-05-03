#include "globals.h"

template<typename T>
void plotMultiLines(const std::map<int,std::vector<T>>& d, const char* lp) {
    for (const auto& p : d)
        if (!p.second.empty())
            ImPlot::PlotLine((lp + std::to_string(p.first)).c_str(),
                             p.second.data(), p.second.size());
}

void run_gui(LocationData* data) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window* window = SDL_CreateWindow("Location Monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    ImGui::CreateContext(); ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) { ImGui_ImplSDL2_ProcessEvent(&event); if (event.type == SDL_QUIT) running = false; }
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplSDL2_NewFrame(); ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);
        
        ImGui::Begin("Location");
        { std::lock_guard<std::mutex> lock(data->mtx);
            ImGui::Text("Latitude: %.6f", data->latitude); ImGui::Text("Longitude: %.6f", data->longitude);
            ImGui::Text("Altitude: %.1f m", data->altitude); ImGui::Text("Accuracy: %.1f m", data->accuracy);
            ImGui::Text("Time: %s", data->timestamp.c_str()); ImGui::Text("Network: %s", data->networkType.c_str());
            ImGui::Text("Operator: %s (%s)", data->networkOperatorName.c_str(), data->networkOperator.c_str());
        } ImGui::End();
        
        ImGui::Begin("Network Details");
        { std::lock_guard<std::mutex> lock(data->mtx);
            if (data->networkType == "LTE") {
                ImGui::Text("Cell ID: %d", data->lteCellId); ImGui::Text("EARFCN: %d", data->lteEarfcn);
                ImGui::Text("PCI: %d", data->ltePci); ImGui::Text("TAC: %d", data->lteTac);
                ImGui::Text("MCC/MNC: %d / %d", data->lteMcc, data->lteMnc);
                ImGui::Text("ASU: %d", data->lteAsuLevel); ImGui::Text("CQI: %d", data->lteCqi);
                ImGui::Text("RSRP: %d dBm", data->lteRsrp); ImGui::Text("RSRQ: %d dB", data->lteRsrq);
                ImGui::Text("RSSI: %d dBm", data->lteRssi); ImGui::Text("RSSNR: %d dB", data->lteRssnr);
                ImGui::Text("Timing Adv: %d", data->lteTimingAdvance);
            } else if (data->networkType == "GSM") {
                ImGui::Text("Cell ID: %d", data->gsmCellId); ImGui::Text("ARFCN: %d", data->gsmArfcn);
                ImGui::Text("BSIC: %d", data->gsmBsic); ImGui::Text("LAC: %d", data->gsmLac);
                ImGui::Text("MCC/MNC: %d / %d", data->gsmMcc, data->gsmMnc);
                ImGui::Text("DBM: %d dBm", data->gsmDbm); ImGui::Text("Timing Adv: %d", data->gsmTimingAdvance);
            } else if (data->networkType == "NR") {
                ImGui::Text("Band: %d", data->nrBand); ImGui::Text("NCI: %ld", data->nrNci);
                ImGui::Text("PCI: %d", data->nrPci); ImGui::Text("NR-ARFCN: %d", data->nrNrarfcn);
                ImGui::Text("TAC: %d", data->nrTac); ImGui::Text("MCC/MNC: %d / %d", data->nrMcc, data->nrMnc);
                ImGui::Text("SS-RSRP: %d dBm", data->nrSsRsrp); ImGui::Text("SS-RSRQ: %d dB", data->nrSsRsrq);
                ImGui::Text("SS-SINR: %d dB", data->nrSsSinr); ImGui::Text("Timing Adv: %d", data->nrTimingAdvance);
            } else ImGui::Text("No network data");
        } ImGui::End();
        
        ImGui::Begin("Signal Graphs");
        {
            std::lock_guard<std::mutex> lock(data->mtx);
            if (data->networkType == "LTE" && !data->lteRsrpHistory.empty()) {
                if (ImPlot::BeginPlot("LTE RSRP / RSRQ / RSSI", "Time", "dBm", ImVec2(-1, 250))) {
                    plotMultiLines(data->lteRsrpHistory, "RSRP PCI=");
                    plotMultiLines(data->lteRsrqHistory, "RSRQ PCI=");
                    plotMultiLines(data->lteRssiHistory, "RSSI PCI=");
                    ImPlot::EndPlot();
                }
                if (ImPlot::BeginPlot("LTE ASU / CQI / RSSNR", "Time", "level / dB", ImVec2(-1, 250))) {
                    for (const auto& pair : data->lteAsuHistory) {
                        if (!pair.second.empty()) {
                            std::vector<float> asuFloat(pair.second.begin(), pair.second.end());
                            ImPlot::PlotLine(("ASU PCI=" + std::to_string(pair.first)).c_str(), asuFloat.data(), asuFloat.size());
                        }
                    }
                    for (const auto& pair : data->lteCqiHistory) {
                        if (!pair.second.empty()) {
                            std::vector<float> cqiFloat(pair.second.begin(), pair.second.end());
                            ImPlot::PlotLine(("CQI PCI=" + std::to_string(pair.first)).c_str(), cqiFloat.data(), cqiFloat.size());
                        }
                    }
                    plotMultiLines(data->lteRssnrHistory, "RSSNR PCI=");
                    ImPlot::EndPlot();
                }
                if (!data->lteTimingAdvanceHistory.empty()) {
                    if (ImPlot::BeginPlot("LTE Timing Advance", "Time", "TA (symbols)", ImVec2(-1, 250))) {
                        for (const auto& pair : data->lteTimingAdvanceHistory) {
                            if (!pair.second.empty()) {
                                std::vector<float> taFloat(pair.second.begin(), pair.second.end());
                                ImPlot::PlotLine(("TA PCI=" + std::to_string(pair.first)).c_str(), taFloat.data(), taFloat.size());
                            }
                        }
                        ImPlot::EndPlot();
                    }
                }
            }
            else if (data->networkType == "NR" && !data->nrRsrpHistory.empty()) {
                if (ImPlot::BeginPlot("NR Signal", "Time", "dBm / dB", ImVec2(-1, 300))) {
                    plotMultiLines(data->nrRsrpHistory, "RSRP PCI=");
                    plotMultiLines(data->nrRsrqHistory, "RSRQ PCI=");
                    plotMultiLines(data->nrSinrHistory, "SINR PCI=");
                    ImPlot::EndPlot();
                }
                if (!data->nrTimingAdvanceHistory.empty()) {
                    if (ImPlot::BeginPlot("NR Timing Advance", "Time", "TA", ImVec2(-1, 300))) {
                        for (const auto& pair : data->nrTimingAdvanceHistory) {
                            if (!pair.second.empty()) {
                                std::vector<float> taFloat(pair.second.begin(), pair.second.end());
                                ImPlot::PlotLine(("TA PCI=" + std::to_string(pair.first)).c_str(), taFloat.data(), taFloat.size());
                            }
                        }
                        ImPlot::EndPlot();
                    }
                }
            } else {
                ImGui::Text("No data");
            }
        }   
        ImGui::End();

        ImGui::Begin("OpenStreetMap");

        static bool g_mapCentered = false;
        {
            std::lock_guard<std::mutex> lock(data->mtx);
            if (!g_mapCentered && (data->latitude != 0.0f || data->longitude != 0.0f)) {
                g_mapCenterLat = data->latitude;
                g_mapCenterLon = data->longitude;
                g_mapCentered = true;
            }
        }

        static bool  s_zoomChanged = false;
        static double s_zLonMin, s_zLonMax, s_zYMin, s_zYMax;
        static ImPlotRect s_lastLimits = {
            g_mapCenterLon - 1.0, g_mapCenterLon + 1.0,
            latToMercY(g_mapCenterLat) - 1.0,
            latToMercY(g_mapCenterLat) + 1.0
        };

        if (ImGui::Button(" + ")) {
            double cx = (s_lastLimits.X.Min + s_lastLimits.X.Max) / 2.0;
            double cy = (s_lastLimits.Y.Min + s_lastLimits.Y.Max) / 2.0;
            double hw = (s_lastLimits.X.Max - s_lastLimits.X.Min) / 4.0;
            double hh = (s_lastLimits.Y.Max - s_lastLimits.Y.Min) / 4.0;
            s_zLonMin = cx - hw; s_zLonMax = cx + hw;
            s_zYMin   = cy - hh; s_zYMax   = cy + hh;
            s_zoomChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(" - ")) {
            double cx = (s_lastLimits.X.Min + s_lastLimits.X.Max) / 2.0;
            double cy = (s_lastLimits.Y.Min + s_lastLimits.Y.Max) / 2.0;
            double hw = (s_lastLimits.X.Max - s_lastLimits.X.Min);
            double hh = (s_lastLimits.Y.Max - s_lastLimits.Y.Min);
            s_zLonMin = cx - hw; s_zLonMax = cx + hw;
            s_zYMin   = cy - hh; s_zYMax   = cy + hh;
            s_zoomChanged = true;
        }
        ImGui::SameLine();
        ImGui::Text("Zoom: %d", g_mapZoom);

        ImGui::Separator();
        ImGui::Checkbox("Show points", &g_showDbPoints);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        if (ImGui::SliderInt("Num of points", &g_maxDbPoints, 10, 20000)) {}
        ImGui::SameLine();
        if (ImGui::Button("Update")) {
            std::thread(fetchDBPoints).detach();
        }

        ImVec2 plotSize = ImGui::GetContentRegionAvail();
        plotSize.y = std::max(plotSize.y, 300.0f);

        double initMercY = latToMercY(g_mapCenterLat);
        if (s_zoomChanged) {
            ImPlot::SetNextAxesLimits(s_zLonMin, s_zLonMax, s_zYMin, s_zYMax, ImGuiCond_Always);
            s_zoomChanged = false;
        } else {
            ImPlot::SetNextAxesLimits(
                g_mapCenterLon - 1.0, g_mapCenterLon + 1.0,
                initMercY - 1.0,      initMercY + 1.0,
                ImGuiCond_Once);
        }

        if (ImPlot::BeginPlot("##map", plotSize, ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
            ImPlot::SetupAxes(nullptr, nullptr,
                ImPlotAxisFlags_NoTickLabels,
                ImPlotAxisFlags_NoTickLabels);
            s_lastLimits = ImPlot::GetPlotLimits();
            ImPlotRect limits = s_lastLimits;

            int prevZoom = g_mapZoom;
            double lonRange = limits.X.Max - limits.X.Min;
            if      (lonRange > 90.0)  g_mapZoom = 4;
            else if (lonRange > 45.0)  g_mapZoom = 5;
            else if (lonRange > 22.5)  g_mapZoom = 6;
            else if (lonRange > 11.25) g_mapZoom = 7;
            else if (lonRange > 5.6)   g_mapZoom = 8;
            else if (lonRange > 2.8)   g_mapZoom = 9;
            else if (lonRange > 1.4)   g_mapZoom = 10;
            else if (lonRange > 0.7)   g_mapZoom = 11;
            else if (lonRange > 0.35)  g_mapZoom = 12;
            else if (lonRange > 0.17)  g_mapZoom = 13;
            else g_mapZoom = 14;

            if (prevZoom != g_mapZoom) {
                {
                    std::lock_guard<std::mutex> lk(g_JobMutex);
                    while (!g_JobQueue.empty()) g_JobQueue.pop();
                }
                std::lock_guard<std::mutex> lk(g_CacheMutex);
                for (auto& [id, tex] : g_TileCache) {
                    if (tex.id == 0 && tex.rgbaBlob.empty()) {
                        tex.isLoading = false;
                    }
                }
            }

            int maxTileCount = (1 << g_mapZoom) - 1;
            int minX = std::max(0, (int)std::floor(MercatorXToTileX(limits.X.Min, g_mapZoom)));
            int maxX = std::min(maxTileCount, (int)std::floor(MercatorXToTileX(limits.X.Max, g_mapZoom)));
            int minY = std::max(0, (int)std::floor(MercatorYToTileY(limits.Y.Max, g_mapZoom)));
            int maxY = std::min(maxTileCount, (int)std::floor(MercatorYToTileY(limits.Y.Min, g_mapZoom)));

            for (int tx = minX; tx <= maxX; ++tx) {
                for (int ty = minY; ty <= maxY; ++ty) {
                    std::string tileId = std::to_string(g_mapZoom) + "/" +
                                        std::to_string(tx) + "/" + std::to_string(ty);
                    GLuint gpuId = 0;
                    bool needQueue = false;
                    {
                        std::lock_guard<std::mutex> lk(g_CacheMutex);
                        auto it = g_TileCache.find(tileId);
                        if (it == g_TileCache.end()) {
                            g_TileCache[tileId].isLoading = true;
                            needQueue = true;
                        } else if (it->second.id == 0 && it->second.rgbaBlob.empty() && !it->second.isLoading) {
                            it->second.isLoading = true;
                            needQueue = true;
                        } else {
                            auto& tex = it->second;
                            if (!tex.rgbaBlob.empty() && tex.id == 0) {
                                glGenTextures(1, &tex.id);
                                glBindTexture(GL_TEXTURE_2D, tex.id);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                                    tex.width, tex.height, 0,
                                    GL_RGBA, GL_UNSIGNED_BYTE, tex.rgbaBlob.data());
                                tex.rgbaBlob.clear();
                            }
                            gpuId = tex.id;
                        }
                    }
                    if (needQueue) {
                        std::lock_guard<std::mutex> jlk(g_JobMutex);
                        g_JobQueue.push({tileId, g_mapZoom, tx, ty});
                    }
                    if (gpuId != 0) {
                        ImPlotPoint minPt{ TileXToMercatorX(tx,     g_mapZoom), TileYToMercatorY(ty + 1, g_mapZoom) };
                        ImPlotPoint maxPt{ TileXToMercatorX(tx + 1, g_mapZoom), TileYToMercatorY(ty,     g_mapZoom) };
                        ImPlot::PlotImage(("##tile_" + tileId).c_str(), (ImTextureID)(uintptr_t)gpuId, minPt, maxPt);
                    } else {
                        ImPlotPoint minPt{ TileXToMercatorX(tx,     g_mapZoom),
                                   TileYToMercatorY(ty + 1, g_mapZoom) };
                        ImPlotPoint maxPt{ TileXToMercatorX(tx + 1, g_mapZoom),
                                   TileYToMercatorY(ty,     g_mapZoom) };
                        ImVec2 p0 = ImPlot::PlotToPixels(minPt.x, minPt.y);
                        ImVec2 p1 = ImPlot::PlotToPixels(maxPt.x, maxPt.y);
                        ImPlot::GetPlotDrawList()->AddRectFilled(p0, p1, IM_COL32(200,200,200,200));
                        ImPlot::GetPlotDrawList()->AddRect(p0, p1, IM_COL32(150,150,150,255));
                    }
                }
            }

            if (g_showDbPoints) {
                std::lock_guard<std::mutex> lk(g_dbPointsMutex);
                ImDrawList* dl = ImPlot::GetPlotDrawList();
                for (const auto& pt : g_dbPoints) {
                    double mercY = latToMercY(pt.lat);
                    ImVec2 pos = ImPlot::PlotToPixels(pt.lon, mercY);
                    dl->AddCircleFilled(pos, 4.f, IM_COL32(50, 150, 255, 200));
                    dl->AddCircle(pos, 4.f, IM_COL32(255, 255, 255, 180));
                }
            }

            {
                std::lock_guard<std::mutex> lock(data->mtx);
                if (data->latitude != 0.0f || data->longitude != 0.0f) {
                    double mercY = latToMercY(data->latitude);
                    ImVec2 pos = ImPlot::PlotToPixels(data->longitude, mercY);
                    ImPlot::GetPlotDrawList()->AddCircleFilled(pos, 3.f, IM_COL32(50, 150, 255, 220));
                }
            }

            ImPlot::EndPlot();
        }
        ImGui::End();
        
        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext(); ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context); SDL_DestroyWindow(window); SDL_Quit();
}