#include "globals.h"

void plotLinesF(const std::map<int, std::vector<float>>& d, const char* p) {
    for (auto it = d.begin(); it != d.end(); ++it) {
        if (!it->second.empty()) {
            ImPlot::PlotLine((p + std::to_string(it->first)).c_str(), it->second.data(), it->second.size());
        }
    }
}

void plotLinesI(const std::map<int, std::vector<int>>& d, const char* p) {
    for (auto it = d.begin(); it != d.end(); ++it) {
        if (!it->second.empty()) {
            std::vector<float> v(it->second.begin(), it->second.end());
            ImPlot::PlotLine((p + std::to_string(it->first)).c_str(), v.data(), v.size());
        }
    }
}

static void clearHeatmapCache() {
    {
        std::lock_guard<std::mutex> lk(g_hmCacheMutex);
        g_hmCache.clear();
    }
    {
        std::lock_guard<std::mutex> j(g_hmQueueMutex);
        while (!g_hmQueue.empty()) g_hmQueue.pop();
    }
}

static void handleZoom(ImPlotRect& last, bool& chg, double& x0, double& x1, double& y0, double& y1, bool in) {
    double cx = (last.X.Min + last.X.Max) / 2.0;
    double cy = (last.Y.Min + last.Y.Max) / 2.0;
    double hw = in ? (last.X.Max - last.X.Min) / 4.0 : (last.X.Max - last.X.Min);
    double hh = in ? (last.Y.Max - last.Y.Min) / 4.0 : (last.Y.Max - last.Y.Min);
    x0 = cx - hw; x1 = cx + hw; y0 = cy - hh; y1 = cy + hh; chg = true;
}

struct LegendStop { float t; ImU32 c; };
static const LegendStop kStops[] = {
    {0.00f, IM_COL32(48,18,59,255)}, {0.15f, IM_COL32(40,120,240,255)},
    {0.30f, IM_COL32(20,230,200,255)}, {0.45f, IM_COL32(90,245,60,255)},
    {0.60f, IM_COL32(220,235,27,255)}, {0.75f, IM_COL32(251,145,5,255)},
    {0.90f, IM_COL32(200,30,10,255)}, {1.00f, IM_COL32(122,4,3,255)}
};

void run_gui(LocationData* data) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_Window* win = SDL_CreateWindow("Location Monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    SDL_GL_MakeCurrent(win, ctx);
    SDL_GL_SetSwapInterval(1);
    glewExperimental = GL_TRUE; glewInit();
    ImGui::CreateContext(); ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(win, ctx);
    ImGui_ImplOpenGL3_Init("#version 330");
    bool run = true;
    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) {
                run = false;
            }
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        ImGui::Begin("Info"); {
            std::lock_guard<std::mutex> l(data->mtx);
            ImGui::Text("Lat:%.6f\nLon:%.6f\nAlt:%.1fm\nAcc:%.1fm\nTime:%s\nNet:%s\nOp:%s(%s)",
                data->latitude, data->longitude, data->altitude, data->accuracy, data->timestamp.c_str(),
                data->networkType.c_str(), data->networkOperatorName.c_str(), data->networkOperator.c_str());
            ImGui::Separator();
            if (data->networkType == "LTE") {
                ImGui::Text("Cell:%d\nEARFCN:%d\nPCI:%d\nTAC:%d\nMCC/MNC:%d/%d\nRSRP:%ddBm\nRSRQ:%ddB\nRSSI:%ddBm\nRSSNR:%ddB\nASU:%d\nCQI:%d\nTA:%d",
                    data->lteCellId, data->lteEarfcn, data->ltePci, data->lteTac, data->lteMcc, data->lteMnc,
                    data->lteRsrp, data->lteRsrq, data->lteRssi, data->lteRssnr, data->lteAsuLevel, data->lteCqi, data->lteTimingAdvance);
            } else if (data->networkType == "GSM") {
                ImGui::Text("Cell:%d\nARFCN:%d\nBSIC:%d\nLAC:%d\nMCC/MNC:%d/%d\nDBM:%ddBm\nTA:%d",
                    data->gsmCellId, data->gsmArfcn, data->gsmBsic, data->gsmLac, data->gsmMcc, data->gsmMnc, data->gsmDbm, data->gsmTimingAdvance);
            } else if (data->networkType == "NR") {
                ImGui::Text("Band:%d\nNCI:%ld\nPCI:%d\nNR-ARFCN:%d\nTAC:%d\nMCC/MNC:%d/%d\nSS-RSRP:%ddBm\nSS-RSRQ:%ddB\nSS-SINR:%ddB\nTA:%d",
                    data->nrBand, data->nrNci, data->nrPci, data->nrNrarfcn, data->nrTac, data->nrMcc, data->nrMnc, data->nrSsRsrp, data->nrSsRsrq, data->nrSsSinr, data->nrTimingAdvance);
            } else {
                ImGui::Text("No data");
            }
        } ImGui::End();
        ImGui::Begin("Graphs"); {
            std::lock_guard<std::mutex> l(data->mtx);
            if (data->networkType == "LTE" && !data->lteRsrpHistory.empty()) {
                if (ImPlot::BeginPlot("LTE RSRP/RSRQ/RSSI", "Time", "dBm", ImVec2(-1, 250))) {
                    plotLinesF(data->lteRsrpHistory, "RSRP ");
                    plotLinesF(data->lteRsrqHistory, "RSRQ ");
                    plotLinesF(data->lteRssiHistory, "RSSI ");
                    ImPlot::EndPlot();
                }
                if (ImPlot::BeginPlot("LTE ASU/CQI/RSSNR", "Time", "lvl/dB", ImVec2(-1, 250))) {
                    plotLinesI(data->lteAsuHistory, "ASU ");
                    plotLinesI(data->lteCqiHistory, "CQI ");
                    plotLinesF(data->lteRssnrHistory, "RSSNR ");
                    ImPlot::EndPlot();
                }
                if (!data->lteTimingAdvanceHistory.empty() && ImPlot::BeginPlot("LTE TA", "Time", "TA", ImVec2(-1, 250))) {
                    plotLinesI(data->lteTimingAdvanceHistory, "TA ");
                    ImPlot::EndPlot();
                }
            } else if (data->networkType == "NR" && !data->nrRsrpHistory.empty()) {
                if (ImPlot::BeginPlot("NR Signal", "Time", "dBm/dB", ImVec2(-1, 300))) {
                    plotLinesF(data->nrRsrpHistory, "RSRP ");
                    plotLinesF(data->nrRsrqHistory, "RSRQ ");
                    plotLinesF(data->nrSinrHistory, "SINR ");
                    ImPlot::EndPlot();
                }
                if (!data->nrTimingAdvanceHistory.empty() && ImPlot::BeginPlot("NR TA", "Time", "TA", ImVec2(-1, 300))) {
                    plotLinesI(data->nrTimingAdvanceHistory, "TA ");
                    ImPlot::EndPlot();
                }
            } else {
                ImGui::Text("No data");
            }
        } ImGui::End();
        ImGui::Begin("Map");
        static bool centered = false;
        {
            std::lock_guard<std::mutex> l(data->mtx);
            if (!centered && (data->latitude || data->longitude)) {
                g_mapCenterLat = data->latitude;
                g_mapCenterLon = data->longitude;
                centered = true;
            }
        }
        static bool zoomChg = false;
        static double zX0, zX1, zY0, zY1;
        static ImPlotRect lastLim = {g_mapCenterLon - 1, g_mapCenterLon + 1, latToMercY(g_mapCenterLat) - 1, latToMercY(g_mapCenterLat) + 1};
        if (ImGui::Button("+")) {
            handleZoom(lastLim, zoomChg, zX0, zX1, zY0, zY1, true);
        }
        ImGui::SameLine();
        if (ImGui::Button("-")) {
            handleZoom(lastLim, zoomChg, zX0, zX1, zY0, zY1, false);
        }
        ImGui::SameLine();
        ImGui::Text("Zoom:%d", g_mapZoom);
        ImGui::Separator();
        ImGui::Checkbox("Pts", &g_showDbPoints);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        ImGui::SliderInt("Count", &g_maxDbPoints, 10, 20000);
        ImGui::SameLine();
        if (ImGui::Button("Update")) {
            std::thread(fetchDBPoints).detach();
        }
        ImGui::Separator();
        ImGui::Checkbox("Heatmap", &g_hmShow);
        ImGui::SameLine();
        {
            const char* hmItems[] = {"RSRP","RSRQ","RSSI","Alt"};
            int hmIdx = (int)g_hmCriteria;
            ImGui::SetNextItemWidth(90);
            if (ImGui::Combo("##c", &hmIdx, hmItems, 4)) {
                g_hmCriteria = (HeatmapCriteria)hmIdx;
                clearHeatmapCache();
            }
        }
        ImGui::SameLine();
        {
            float prevR = g_hmRadius;
            ImGui::SetNextItemWidth(80);
            ImGui::InputFloat("R(m)", &g_hmRadius, 10, 10, "%.0f");
            g_hmRadius = std::max(10.f, std::min(g_hmRadius, 50000.f));
            if (g_hmRadius != prevR) {
                clearHeatmapCache();
            }
        }
        ImGui::SameLine();
        {
            static std::vector<int> eL, pL;
            static std::vector<std::string> eS, pS;
            static float lastRef = 0.f;
            float now = static_cast<float>(ImGui::GetTime());
            if (now - lastRef > 5.f) {
                lastRef = now;
                eL.clear(); eL.push_back(0);
                pL.clear(); pL.push_back(0);
                {
                    std::lock_guard<std::mutex> lk(g_hmPointsMutex);
                    for (const auto& p : g_hmPoints) {
                        if (p.earfcn && std::find(eL.begin(), eL.end(), p.earfcn) == eL.end()) {
                            eL.push_back(p.earfcn);
                        }
                        if (p.pci && std::find(pL.begin(), pL.end(), p.pci) == pL.end()) {
                            pL.push_back(p.pci);
                        }
                    }
                }
                eS.clear();
                for (int v : eL) {
                    eS.push_back(v == 0 ? "All" : std::to_string(v));
                }
                pS.clear();
                for (int v : pL) {
                    pS.push_back(v == 0 ? "All" : std::to_string(v));
                }
            }
            std::vector<const char*> ec, pc;
            for (const auto& s : eS) {
                ec.push_back(s.c_str());
            }
            for (const auto& s : pS) {
                pc.push_back(s.c_str());
            }
            int eIdx = 0, pIdx = 0;
            for (int i = 0; i < (int)eL.size(); ++i) {
                if (eL[i] == g_hmEarfcn) {
                    eIdx = i;
                }
            }
            for (int i = 0; i < (int)pL.size(); ++i) {
                if (pL[i] == g_hmPci) {
                    pIdx = i;
                }
            }
            ImGui::SetNextItemWidth(90);
            if (ImGui::Combo("EARFCN##ef", &eIdx, ec.data(), (int)ec.size())) {
                g_hmEarfcn = eL[eIdx];
                clearHeatmapCache();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            if (ImGui::Combo("PCI##pf", &pIdx, pc.data(), (int)pc.size())) {
                g_hmPci = pL[pIdx];
                clearHeatmapCache();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload##hm")) {
            std::thread(fetchHeatmapPoints).detach();
            clearHeatmapCache();
        }
        ImVec2 sz = ImGui::GetContentRegionAvail();
        sz.y = std::max(sz.y, 300.f);
        if (zoomChg) {
            ImPlot::SetNextAxesLimits(zX0, zX1, zY0, zY1, ImGuiCond_Always);
            zoomChg = false;
        } else {
            double mY = latToMercY(g_mapCenterLat);
            ImPlot::SetNextAxesLimits(g_mapCenterLon-1, g_mapCenterLon+1, mY-1, mY+1, ImGuiCond_Once);
        }
        if (ImPlot::BeginPlot("##map", sz, ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_NoTickLabels);
            lastLim = ImPlot::GetPlotLimits();
            ImPlotRect lim = lastLim;
            int pz = g_mapZoom;
            double lr = lim.X.Max - lim.X.Min;
            if (lr > 90) {
                g_mapZoom = 4;
            } else if (lr > 45) {
                g_mapZoom = 5;
            } else if (lr > 22.5) {
                g_mapZoom = 6;
            } else if (lr > 11.25) {
                g_mapZoom = 7;
            } else if (lr > 5.6) {
                g_mapZoom = 8;
            } else if (lr > 2.8) {
                g_mapZoom = 9;
            } else if (lr > 1.4) {
                g_mapZoom = 10;
            } else if (lr > 0.7) {
                g_mapZoom = 11;
            } else if (lr > 0.35) {
                g_mapZoom = 12;
            } else if (lr > 0.17) {
                g_mapZoom = 13;
            } else {
                g_mapZoom = 14;
            }
            if (pz != g_mapZoom) {
                std::lock_guard<std::mutex> lk(g_JobMutex);
                while (!g_JobQueue.empty()) g_JobQueue.pop();
                std::lock_guard<std::mutex> c(g_CacheMutex);
                for (auto& kv : g_TileCache) {
                    if (kv.second.id == 0 && kv.second.rgbaBlob.empty()) {
                        kv.second.isLoading = false;
                    }
                }
            }
            int mx = (1 << g_mapZoom) - 1;
            int tx0 = std::max(0, (int)std::floor(MercatorXToTileX(lim.X.Min, g_mapZoom)));
            int tx1 = std::min(mx, (int)std::floor(MercatorXToTileX(lim.X.Max, g_mapZoom)));
            int ty0 = std::max(0, (int)std::floor(MercatorYToTileY(lim.Y.Max, g_mapZoom)));
            int ty1 = std::min(mx, (int)std::floor(MercatorYToTileY(lim.Y.Min, g_mapZoom)));
            for (int tx = tx0; tx <= tx1; ++tx) {
                for (int ty = ty0; ty <= ty1; ++ty) {
                    std::string id = std::to_string(g_mapZoom)+"/"+std::to_string(tx)+"/"+std::to_string(ty);
                    GLuint gid = 0;
                    bool need = false;
                    {
                        std::lock_guard<std::mutex> l(g_CacheMutex);
                        auto it = g_TileCache.find(id);
                        if (it == g_TileCache.end()) {
                            g_TileCache[id].isLoading = true;
                            need = true;
                        } else if (it->second.id == 0 && it->second.rgbaBlob.empty() && !it->second.isLoading) {
                            it->second.isLoading = true;
                            need = true;
                        } else {
                            auto& t = it->second;
                            if (!t.rgbaBlob.empty() && t.id == 0) {
                                glGenTextures(1, &t.id);
                                glBindTexture(GL_TEXTURE_2D, t.id);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.width, t.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t.rgbaBlob.data());
                                t.rgbaBlob.clear();
                            }
                            gid = t.id;
                        }
                    }
                    if (need) {
                        std::lock_guard<std::mutex> j(g_JobMutex);
                        g_JobQueue.push({id, g_mapZoom, tx, ty});
                    }
                    ImPlotPoint a{TileXToMercatorX(tx, g_mapZoom), TileYToMercatorY(ty+1, g_mapZoom)};
                    ImPlotPoint b{TileXToMercatorX(tx+1, g_mapZoom), TileYToMercatorY(ty, g_mapZoom)};
                    if (gid) {
                        ImPlot::PlotImage(("##t_"+id).c_str(), (ImTextureID)(uintptr_t)gid, a, b);
                    } else {
                        ImVec2 p0 = ImPlot::PlotToPixels(a.x, a.y);
                        ImVec2 p1 = ImPlot::PlotToPixels(b.x, b.y);
                        ImPlot::GetPlotDrawList()->AddRectFilled(p0, p1, IM_COL32(200,200,200,200));
                        ImPlot::GetPlotDrawList()->AddRect(p0, p1, IM_COL32(150,150,150,255));
                    }
                }
            }
            if (g_showDbPoints) {
                std::lock_guard<std::mutex> lk(g_dbPointsMutex);
                ImDrawList* dl = ImPlot::GetPlotDrawList();
                for (const auto& pt : g_dbPoints) {
                    ImVec2 pos = ImPlot::PlotToPixels(pt.lon, latToMercY(pt.lat));
                    dl->AddCircleFilled(pos, 4.f, IM_COL32(50,150,255,200));
                    dl->AddCircle(pos, 4.f, IM_COL32(255,255,255,180));
                }
            }
            if (g_hmShow) {
                std::string pfx = hmCriteriaStr(g_hmCriteria)+"/r"+std::to_string((int)g_hmRadius)+"/e"+std::to_string(g_hmEarfcn)+"/p"+std::to_string(g_hmPci)+"/";
                for (int tx = tx0; tx <= tx1; ++tx) {
                    for (int ty = ty0; ty <= ty1; ++ty) {
                        std::string id = pfx + std::to_string(g_mapZoom)+"/"+std::to_string(tx)+"/"+std::to_string(ty);
                        GLuint gid = 0;
                        bool need = false;
                        {
                            std::lock_guard<std::mutex> l(g_hmCacheMutex);
                            auto it = g_hmCache.find(id);
                            if (it == g_hmCache.end()) {
                                g_hmCache[id].isLoading = true;
                                need = true;
                            } else {
                                auto& t = it->second;
                                if (!t.rgbaBlob.empty() && t.id == 0) {
                                    glGenTextures(1, &t.id);
                                    glBindTexture(GL_TEXTURE_2D, t.id);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                                    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.width, t.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t.rgbaBlob.data());
                                    t.rgbaBlob.clear();
                                }
                                gid = t.id;
                            }
                        }
                        if (need) {
                            std::lock_guard<std::mutex> j(g_hmQueueMutex);
                            g_hmQueue.push({id, g_mapZoom, tx, ty});
                        }
                        if (gid) {
                            ImPlotPoint a{TileXToMercatorX(tx, g_mapZoom), TileYToMercatorY(ty+1, g_mapZoom)};
                            ImPlotPoint b{TileXToMercatorX(tx+1, g_mapZoom), TileYToMercatorY(ty, g_mapZoom)};
                            ImPlot::PlotImage(("##hm_"+id).c_str(), (ImTextureID)(uintptr_t)gid, a, b);
                        }
                    }
                }
            }
            {
                std::lock_guard<std::mutex> l(data->mtx);
                if (data->latitude || data->longitude) {
                    ImVec2 pos = ImPlot::PlotToPixels(data->longitude, latToMercY(data->latitude));
                    ImDrawList* dl = ImPlot::GetPlotDrawList();
                    dl->AddCircleFilled(pos, 7.f, IM_COL32(50,150,255,230));
                    dl->AddCircle(pos, 7.f, IM_COL32(255,255,255,220), 16, 1.5f);
                }
            }
            ImPlot::EndPlot();
        }
        if (g_hmShow) {
            float v0, v1;
            const char* u = "";
            switch (g_hmCriteria) {
                case HeatmapCriteria::RSRP: v0=-110; v1=-80; u="dBm"; break;
                case HeatmapCriteria::RSRQ: v0=-20; v1=-3; u="dB"; break;
                case HeatmapCriteria::RSSI: v0=-110; v1=-40; u="dBm"; break;
                default: v0=0; v1=500; u="m";
            }
            ImVec2 r0 = ImGui::GetItemRectMin();
            ImVec2 r1 = ImGui::GetItemRectMax();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const float bw=16, bh=160, pd=12, py=20;
            float x = r1.x - pd - bw;
            float y = r0.y + py;
            for (int i = 0; i < 7; ++i) {
                float ya = y + kStops[i].t * bh;
                float yb = y + kStops[i+1].t * bh;
                dl->AddRectFilledMultiColor({x,ya}, {x+bw,yb}, kStops[i].c, kStops[i].c, kStops[i+1].c, kStops[i+1].c);
            }
            dl->AddRect({x,y}, {x+bw,y+bh}, IM_COL32(200,200,200,180));
            for (int i = 0; i < 5; ++i) {
                float t = (float)i / 4;
                float v = v0 + t * (v1 - v0);
                float yy = y + t * bh;
                char buf[32];
                snprintf(buf, sizeof(buf), "%.0f%s", v, u);
                dl->AddLine({x-4,yy}, {x,yy}, IM_COL32(200,200,200,200));
                dl->AddText({x-52,yy-6}, IM_COL32(255,255,255,230), buf);
            }
        }
        ImGui::End();
        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
}