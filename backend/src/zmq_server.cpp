#include "globals.h"

template<typename T>
T getJsonValue(const json& j, const std::string& k, T def = T{}) {
    try { if (j.contains(k) && !j[k].is_null()) return j[k].get<T>(); } catch(...) {}
    return def;
}

template<typename T>
void updateHistory(std::map<int,std::vector<T>>& h, int pci, T val, size_t mx=100) {
    if (!pci) return;
    auto& v = h[pci]; v.push_back(val);
    if (v.size() > mx) v.erase(v.begin());
}

void processJsonObject(const json& jdata, LocationData* data) {
    float lat = getJsonValue<float>(jdata, "latitude", 0.0f);
    float lon = getJsonValue<float>(jdata, "longitude", 0.0f);
    float alt = getJsonValue<float>(jdata, "altitude", 0.0f);
    float acc = getJsonValue<float>(jdata, "accuracy", 0.0f);
    std::string time = getJsonValue<std::string>(jdata, "time", "");
    time = convertTimeFormat(time);
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
            zmq::message_t req;
            if (!sock.recv(req, zmq::recv_flags::none)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            std::string msg(static_cast<char*>(req.data()), req.size());
            auto jdata = json::parse(msg);
            if (jdata.is_array()) for (const auto& item : jdata) processJsonObject(item, data);
            else processJsonObject(jdata, data);
            sock.send(zmq::buffer("ACK"), zmq::send_flags::none);
        } catch (const std::exception& e) {
            std::cerr << "ZMQ: " << e.what() << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}