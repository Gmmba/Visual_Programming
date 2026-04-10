#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <map>
#include <pqxx/pqxx>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"
#include "implot.h"
#include "zmq.hpp"
#include "json.hpp"

using json = nlohmann::json;

struct DBConfig {
    std::string host = "127.0.0.1";
    std::string port = "5432";
    std::string dbname = "network_data";
    std::string user = "thunder.struck1";
    std::string password = "123456";
};

#include <vector>
#include <map>
#include <mutex>
#include <string>

struct LocationData {
    float latitude = 0.0f, longitude = 0.0f, altitude = 0.0f, accuracy = 0.0f;
    std::string timestamp, networkType, networkOperator, networkOperatorName;
    
    std::vector<float> lteRsrpData;
    std::vector<float> lteRsrqData;
    std::vector<float> lteRssiData;
    std::vector<float> lteRssnrData;
    std::vector<int>   lteAsuData;
    std::vector<int>   lteCqiData;
    std::vector<int>   lteTimingAdvanceData;
    
    std::vector<float> nrSsRsrpData;
    std::vector<float> nrSsRsrqData;
    std::vector<float> nrSsSinrData;
    std::vector<int>   nrTimingAdvanceData;

    std::map<int, std::vector<float>> lteRsrpHistory;
    std::map<int, std::vector<float>> lteRsrqHistory;
    std::map<int, std::vector<float>> lteRssiHistory;
    std::map<int, std::vector<float>> lteRssnrHistory;
    std::map<int, std::vector<int>>   lteAsuHistory;
    std::map<int, std::vector<int>>   lteCqiHistory;
    std::map<int, std::vector<int>>   lteTimingAdvanceHistory;
    
    std::map<int, std::vector<float>> nrRsrpHistory;
    std::map<int, std::vector<float>> nrRsrqHistory;
    std::map<int, std::vector<float>> nrSinrHistory;
    std::map<int, std::vector<int>>   nrTimingAdvanceHistory;
    
    int lteCellId = 0, lteEarfcn = 0, lteMcc = 0, lteMnc = 0, ltePci = 0, lteTac = 0;
    int lteAsuLevel = 0, lteCqi = 0, lteRsrp = 0, lteRsrq = 0, lteRssi = 0, lteRssnr = 0, lteTimingAdvance = 0;
    int gsmCellId = 0, gsmBsic = 0, gsmArfcn = 0, gsmLac = 0, gsmMcc = 0, gsmMnc = 0, gsmPsc = 0, gsmDbm = 0, gsmTimingAdvance = 0;
    int nrBand = 0; long nrNci = 0; int nrPci = 0, nrNrarfcn = 0, nrTac = 0, nrMcc = 0, nrMnc = 0;
    int nrSsRsrp = 0, nrSsRsrq = 0, nrSsSinr = 0, nrTimingAdvance = 0;
    
    mutable std::mutex mtx;
};

class DatabaseManager {
private:
    DBConfig config;
    std::string connection_string;
public:
    DatabaseManager(const DBConfig& cfg) : config(cfg) {
        connection_string = "host=" + config.host + " port=" + config.port +
                            " dbname=" + config.dbname + " user=" + config.user +
                            " password=" + config.password;
    }
    bool testConnection() {
        try {
            pqxx::connection conn(connection_string);
            if (conn.is_open()) { std::cout << "Подключение к PostgreSQL успешно\n"; return true; }
        } catch (const std::exception& e) { std::cerr << "Ошибка подключения: " << e.what() << std::endl; }
        return false;
    }
    bool timeExists(const std::string& timeStr, const std::string& tableName) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work txn(conn);
            pqxx::result res = txn.exec_params("SELECT 1 FROM " + tableName + " WHERE time = $1 LIMIT 1", timeStr);
            return !res.empty();
        } catch (...) { return true; }
    }

    bool insertLocationData(const LocationData& data) {
    try {
        pqxx::connection conn(connection_string);
        pqxx::work txn(conn);
        pqxx::result res;

        if (data.networkType == "LTE") {
            res = txn.exec_params(
                "INSERT INTO lte_data (latitude, longitude, altitude, accuracy, time, networkType, "
                "rsrp, rsrq, rssi, rssnr, timing_advance, cell_id, earfcn, mcc, mnc, pci, tac, asu_level, cqi) "
                "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18,$19) "
                "ON CONFLICT (time) DO NOTHING",
                data.latitude, data.longitude, data.altitude, data.accuracy,
                data.timestamp, data.networkType,
                data.lteRsrp, data.lteRsrq, data.lteRssi, data.lteRssnr,
                data.lteTimingAdvance, data.lteCellId, data.lteEarfcn,
                data.lteMcc, data.lteMnc, data.ltePci, data.lteTac,
                data.lteAsuLevel, data.lteCqi);
        }
        else if (data.networkType == "NR") {
            res = txn.exec_params(
                "INSERT INTO nr_data (latitude, longitude, altitude, accuracy, time, networkType, "
                "band, nr_nci, nr_pci, nr_nrarfcn, nr_tac, nr_mcc, nr_mnc, "
                "nr_ss_rsrp, nr_ss_rsrq, nr_ss_sinr, nr_timing_advance) "
                "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17) "
                "ON CONFLICT (time) DO NOTHING",
                data.latitude, data.longitude, data.altitude, data.accuracy,
                data.timestamp, data.networkType,
                data.nrBand, data.nrNci, data.nrPci, data.nrNrarfcn,
                data.nrTac, data.nrMcc, data.nrMnc,
                data.nrSsRsrp, data.nrSsRsrq, data.nrSsSinr,
                data.nrTimingAdvance);
        }
        else if (data.networkType == "GSM") {
            res = txn.exec_params(
                "INSERT INTO gsm_data (latitude, longitude, altitude, accuracy, time, networkType, "
                "cell_id, gsm_bsic, gsm_arfcn, gsm_lac, gsm_mcc, gsm_mnc, "
                "gsm_psc, gsm_dbm, gsm_timing_advance) "
                "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15) "
                "ON CONFLICT (time) DO NOTHING",
                data.latitude, data.longitude, data.altitude, data.accuracy,
                data.timestamp, data.networkType,
                data.gsmCellId, data.gsmBsic, data.gsmArfcn, data.gsmLac,
                data.gsmMcc, data.gsmMnc, data.gsmPsc,
                data.gsmDbm, data.gsmTimingAdvance);
        }
        else return false;

        txn.commit();
        return (res.affected_rows() > 0);
    } catch (const std::exception& e) {
        std::cerr << "Ошибка вставки: " << e.what() << std::endl;
        return false;
    }
}
};

template<typename T> T getJsonValue(const json& j, const std::string& key, T defaultValue = T{}) {
    try { if (j.contains(key) && !j[key].is_null()) return j[key].get<T>(); } catch(...) {}
    return defaultValue;
}

DatabaseManager* dbManager = nullptr;
int recordsInserted = 0, recordsSkipped = 0;

template<typename T>
void updateHistory(std::map<int, std::vector<T>>& history, int pci, T value, size_t maxSize = 100) {
    if (pci == 0) return;
    auto& vec = history[pci];
    vec.push_back(value);
    if (vec.size() > maxSize) vec.erase(vec.begin());
}

void processLteCells(const json& cells, LocationData* data) {
    for (const auto& cell : cells) {
        int pci = getJsonValue<int>(cell, "pci", 0);
        if (pci == 0) continue;
        updateHistory(data->lteRsrpHistory, pci, (float)getJsonValue<int>(cell, "rsrp", 0));
        updateHistory(data->lteRsrqHistory, pci, (float)getJsonValue<int>(cell, "rsrq", 0));
        updateHistory(data->lteRssiHistory, pci, (float)getJsonValue<int>(cell, "rssi", 0));
        updateHistory(data->lteRssnrHistory, pci, (float)getJsonValue<int>(cell, "rssnr", 0));
        updateHistory(data->lteAsuHistory, pci, getJsonValue<int>(cell, "asuLevel", 0));
        updateHistory(data->lteCqiHistory, pci, getJsonValue<int>(cell, "cqi", 0));
        updateHistory(data->lteTimingAdvanceHistory, pci, getJsonValue<int>(cell, "timingAdvance", 0));
    }
}

void processNrCells(const json& cells, LocationData* data) {
    for (const auto& cell : cells) {
        int pci = getJsonValue<int>(cell, "pci", 0);
        if (pci == 0) continue;
        updateHistory(data->nrRsrpHistory, pci, (float)getJsonValue<int>(cell, "ssRsrp", 0));
        updateHistory(data->nrRsrqHistory, pci, (float)getJsonValue<int>(cell, "ssRsrq", 0));
        updateHistory(data->nrSinrHistory, pci, (float)getJsonValue<int>(cell, "ssSinr", 0));
        updateHistory(data->nrTimingAdvanceHistory, pci, getJsonValue<int>(cell, "timingAdvance", 0));
    }
}

void processJsonObject(const json& jdata, LocationData* data) {
    float lat = getJsonValue<float>(jdata, "latitude", 0.0f);
    float lon = getJsonValue<float>(jdata, "longitude", 0.0f);
    float alt = getJsonValue<float>(jdata, "altitude", 0.0f);
    float acc = getJsonValue<float>(jdata, "accuracy", 0.0f);
    std::string time = getJsonValue<std::string>(jdata, "time", "");
    std::string netType = getJsonValue<std::string>(jdata, "networkType", "Unknown");
    std::string netOp = getJsonValue<std::string>(jdata, "networkOperator", "");
    std::string netOpName = getJsonValue<std::string>(jdata, "networkOperatorName", "");

    int lteCellId = getJsonValue<int>(jdata, "lteCellId", 0);
    int lteEarfcn = getJsonValue<int>(jdata, "lteEarfcn", 0);
    int lteMcc = getJsonValue<int>(jdata, "lteMcc", 0);
    int lteMnc = getJsonValue<int>(jdata, "lteMnc", 0);
    int ltePci = getJsonValue<int>(jdata, "ltePci", 0);
    int lteTac = getJsonValue<int>(jdata, "lteTac", 0);
    int lteAsu = getJsonValue<int>(jdata, "lteAsuLevel", 0);
    int lteCqi = getJsonValue<int>(jdata, "lteCqi", 0);
    int lteRsrp = getJsonValue<int>(jdata, "lteRsrp", 0);
    int lteRsrq = getJsonValue<int>(jdata, "lteRsrq", 0);
    int lteRssi = getJsonValue<int>(jdata, "lteRssi", 0);
    int lteRssnr = getJsonValue<int>(jdata, "lteRssnr", 0);
    int lteTa = getJsonValue<int>(jdata, "lteTimingAdvance", 0);

    int gsmCellId = getJsonValue<int>(jdata, "gsmCellId", 0);
    int gsmBsic = getJsonValue<int>(jdata, "gsmBsic", 0);
    int gsmArfcn = getJsonValue<int>(jdata, "gsmArfcn", 0);
    int gsmLac = getJsonValue<int>(jdata, "gsmLac", 0);
    int gsmMcc = getJsonValue<int>(jdata, "gsmMcc", 0);
    int gsmMnc = getJsonValue<int>(jdata, "gsmMnc", 0);
    int gsmPsc = getJsonValue<int>(jdata, "gsmPsc", 0);
    int gsmDbm = getJsonValue<int>(jdata, "gsmDbm", 0);
    int gsmTa = getJsonValue<int>(jdata, "gsmTimingAdvance", 0);

    int nrBand = getJsonValue<int>(jdata, "nrBand", 0);
    long nrNci = getJsonValue<long>(jdata, "nrNci", 0L);
    int nrPci = getJsonValue<int>(jdata, "nrPci", 0);
    int nrNrarfcn = getJsonValue<int>(jdata, "nrNrarfcn", 0);
    int nrTac = getJsonValue<int>(jdata, "nrTac", 0);
    int nrMcc = getJsonValue<int>(jdata, "nrMcc", 0);
    int nrMnc = getJsonValue<int>(jdata, "nrMnc", 0);
    int nrRsrp = getJsonValue<int>(jdata, "nrSsRsrp", 0);
    int nrRsrq = getJsonValue<int>(jdata, "nrSsRsrq", 0);
    int nrSinr = getJsonValue<int>(jdata, "nrSsSinr", 0);
    int nrTa = getJsonValue<int>(jdata, "nrTimingAdvance", 0);

    LocationData temp;
    temp.latitude = lat; temp.longitude = lon; temp.altitude = alt; temp.accuracy = acc;
    temp.timestamp = time; temp.networkType = netType;
    temp.networkOperator = netOp; temp.networkOperatorName = netOpName;
    temp.lteCellId = lteCellId; temp.lteEarfcn = lteEarfcn; temp.lteMcc = lteMcc; temp.lteMnc = lteMnc;
    temp.ltePci = ltePci; temp.lteTac = lteTac; temp.lteAsuLevel = lteAsu; temp.lteCqi = lteCqi;
    temp.lteRsrp = lteRsrp; temp.lteRsrq = lteRsrq; temp.lteRssi = lteRssi; temp.lteRssnr = lteRssnr;
    temp.lteTimingAdvance = lteTa;
    temp.gsmCellId = gsmCellId; temp.gsmBsic = gsmBsic; temp.gsmArfcn = gsmArfcn; temp.gsmLac = gsmLac;
    temp.gsmMcc = gsmMcc; temp.gsmMnc = gsmMnc; temp.gsmPsc = gsmPsc; temp.gsmDbm = gsmDbm;
    temp.gsmTimingAdvance = gsmTa;
    temp.nrBand = nrBand; temp.nrNci = nrNci; temp.nrPci = nrPci; temp.nrNrarfcn = nrNrarfcn;
    temp.nrTac = nrTac; temp.nrMcc = nrMcc; temp.nrMnc = nrMnc; temp.nrSsRsrp = nrRsrp;
    temp.nrSsRsrq = nrRsrq; temp.nrSsSinr = nrSinr; temp.nrTimingAdvance = nrTa;

    bool inserted = dbManager->insertLocationData(temp);

    {
        std::lock_guard<std::mutex> lock(data->mtx);
        data->latitude = lat; data->longitude = lon; data->altitude = alt; data->accuracy = acc;
        data->timestamp = time; data->networkType = netType;
        data->networkOperator = netOp; data->networkOperatorName = netOpName;
        data->lteCellId = lteCellId; data->lteEarfcn = lteEarfcn; data->lteMcc = lteMcc; data->lteMnc = lteMnc;
        data->ltePci = ltePci; data->lteTac = lteTac; data->lteAsuLevel = lteAsu; data->lteCqi = lteCqi;
        data->lteRsrp = lteRsrp; data->lteRsrq = lteRsrq; data->lteRssi = lteRssi; data->lteRssnr = lteRssnr;
        data->lteTimingAdvance = lteTa;
        data->gsmCellId = gsmCellId; data->gsmBsic = gsmBsic; data->gsmArfcn = gsmArfcn; data->gsmLac = gsmLac;
        data->gsmMcc = gsmMcc; data->gsmMnc = gsmMnc; data->gsmPsc = gsmPsc; data->gsmDbm = gsmDbm;
        data->gsmTimingAdvance = gsmTa;
        data->nrBand = nrBand; data->nrNci = nrNci; data->nrPci = nrPci; data->nrNrarfcn = nrNrarfcn;
        data->nrTac = nrTac; data->nrMcc = nrMcc; data->nrMnc = nrMnc; data->nrSsRsrp = nrRsrp;
        data->nrSsRsrq = nrRsrq; data->nrSsSinr = nrSinr; data->nrTimingAdvance = nrTa;
    }

    if (inserted) {
        std::lock_guard<std::mutex> lock(data->mtx);
        if (netType == "LTE") {
            // Старые векторы (одна сота)
            data->lteRsrpData.push_back(static_cast<float>(lteRsrp));
            data->lteRsrqData.push_back(static_cast<float>(lteRsrq));
            data->lteRssiData.push_back(static_cast<float>(lteRssi));
            data->lteAsuData.push_back(lteAsu);
            data->lteCqiData.push_back(lteCqi);
            data->lteRssnrData.push_back(static_cast<float>(lteRssnr));
            data->lteTimingAdvanceData.push_back(lteTa);
            if (data->lteRsrpData.size() > 100) {
                data->lteRsrpData.erase(data->lteRsrpData.begin());
                data->lteRsrqData.erase(data->lteRsrqData.begin());
                data->lteRssiData.erase(data->lteRssiData.begin());
                data->lteAsuData.erase(data->lteAsuData.begin());
                data->lteCqiData.erase(data->lteCqiData.begin());
                data->lteRssnrData.erase(data->lteRssnrData.begin());
                data->lteTimingAdvanceData.erase(data->lteTimingAdvanceData.begin());
            }

            if (jdata.contains("cells") && jdata["cells"].is_array()) {
                for (const auto& cell : jdata["cells"]) {
                    int pci = getJsonValue<int>(cell, "pci", 0);
                    if (pci == 0) continue;
                    updateHistory(data->lteRsrpHistory, pci, static_cast<float>(getJsonValue<int>(cell, "rsrp", 0)));
                    updateHistory(data->lteRsrqHistory, pci, static_cast<float>(getJsonValue<int>(cell, "rsrq", 0)));
                    updateHistory(data->lteRssiHistory, pci, static_cast<float>(getJsonValue<int>(cell, "rssi", 0)));
                    updateHistory(data->lteRssnrHistory, pci, static_cast<float>(getJsonValue<int>(cell, "rssnr", 0)));
                    updateHistory(data->lteAsuHistory, pci, getJsonValue<int>(cell, "asuLevel", 0));
                    updateHistory(data->lteCqiHistory, pci, getJsonValue<int>(cell, "cqi", 0));
                    updateHistory(data->lteTimingAdvanceHistory, pci, getJsonValue<int>(cell, "timingAdvance", 0));
                }
            } else if (ltePci != 0) {
                updateHistory(data->lteRsrpHistory, ltePci, static_cast<float>(lteRsrp));
                updateHistory(data->lteRsrqHistory, ltePci, static_cast<float>(lteRsrq));
                updateHistory(data->lteRssiHistory, ltePci, static_cast<float>(lteRssi));
                updateHistory(data->lteRssnrHistory, ltePci, static_cast<float>(lteRssnr));
                updateHistory(data->lteAsuHistory, ltePci, lteAsu);
                updateHistory(data->lteCqiHistory, ltePci, lteCqi);
                updateHistory(data->lteTimingAdvanceHistory, ltePci, lteTa);
            }
        }
        else if (netType == "NR") {
            data->nrSsRsrpData.push_back(static_cast<float>(nrRsrp));
            data->nrSsRsrqData.push_back(static_cast<float>(nrRsrq));
            data->nrSsSinrData.push_back(static_cast<float>(nrSinr));
            data->nrTimingAdvanceData.push_back(nrTa);
            if (data->nrSsRsrpData.size() > 100) {
                data->nrSsRsrpData.erase(data->nrSsRsrpData.begin());
                data->nrSsRsrqData.erase(data->nrSsRsrqData.begin());
                data->nrSsSinrData.erase(data->nrSsSinrData.begin());
                data->nrTimingAdvanceData.erase(data->nrTimingAdvanceData.begin());
            }

            if (jdata.contains("cells") && jdata["cells"].is_array()) {
                for (const auto& cell : jdata["cells"]) {
                    int pci = getJsonValue<int>(cell, "pci", 0);
                    if (pci == 0) continue;
                    updateHistory(data->nrRsrpHistory, pci, static_cast<float>(getJsonValue<int>(cell, "ssRsrp", 0)));
                    updateHistory(data->nrRsrqHistory, pci, static_cast<float>(getJsonValue<int>(cell, "ssRsrq", 0)));
                    updateHistory(data->nrSinrHistory, pci, static_cast<float>(getJsonValue<int>(cell, "ssSinr", 0)));
                    updateHistory(data->nrTimingAdvanceHistory, pci, getJsonValue<int>(cell, "timingAdvance", 0));
                }
            } else if (nrPci != 0) {
                updateHistory(data->nrRsrpHistory, nrPci, static_cast<float>(nrRsrp));
                updateHistory(data->nrRsrqHistory, nrPci, static_cast<float>(nrRsrq));
                updateHistory(data->nrSinrHistory, nrPci, static_cast<float>(nrSinr));
                updateHistory(data->nrTimingAdvanceHistory, nrPci, nrTa);
            }
        }
        recordsInserted++;
    } else {
        recordsSkipped++;
    }

    if ((recordsInserted + recordsSkipped) % 10 == 0) {
        std::cout << "Вставлено: " << recordsInserted << ", пропущено: " << recordsSkipped << std::endl;
    }
}

void run_server(LocationData* data) {
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::rep);
    sock.bind("tcp://*:5555");
    std::cout << "Сервер запущен на порту 5555\n";
    while (true) {
        try {
            zmq::message_t request;
            if (!sock.recv(request, zmq::recv_flags::none)) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); continue; }
            std::string msg(static_cast<char*>(request.data()), request.size());
            auto jdata = json::parse(msg);
            if (jdata.is_array()) for (const auto& item : jdata) processJsonObject(item, data);
            else processJsonObject(jdata, data);
            sock.send(zmq::buffer("ACK"), zmq::send_flags::none);
        } catch (const std::exception& e) { std::cerr << "Ошибка: " << e.what() << std::endl; std::this_thread::sleep_for(std::chrono::milliseconds(500)); }
    }
}

template<typename T>
void plotMultiLines(const std::map<int, std::vector<T>>& data, const char* labelPrefix) {
    for (const auto& pair : data) {
        if (!pair.second.empty()) {
            std::string label = labelPrefix + std::to_string(pair.first);
            ImPlot::PlotLine(label.c_str(), pair.second.data(), pair.second.size());
        }
    }
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
        
        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext(); ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context); SDL_DestroyWindow(window); SDL_Quit();
}

int main() {
    DBConfig dbConfig;
    dbManager = new DatabaseManager(dbConfig);
    dbManager->testConnection();
    LocationData locationData;
    std::thread server_thread(run_server, &locationData);
    run_gui(&locationData);
    server_thread.join();
    delete dbManager;
    return 0;
}