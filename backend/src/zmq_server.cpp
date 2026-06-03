#include "globals.h"

float getFloatValue(const json& j, const std::string& k, float def = 0.0f) { return j.value(k, def); }
int getIntValue(const json& j, const std::string& k, int def = 0) { return j.value(k, def); }
long getLongValue(const json& j, const std::string& k, long def = 0) { return j.value(k, def); }
std::string getStringValue(const json& j, const std::string& k, std::string def = "") { return j.value(k, def); }

void updateHistory(std::map<int, std::vector<float>>& h, int pci, float val, size_t mx = 100) {
    if (!pci) return;
    auto& v = h[pci]; v.push_back(val);
    if (v.size() > mx) v.erase(v.begin());
}

void updateHistory(std::map<int, std::vector<int>>& h, int pci, int val, size_t mx = 100) {
    if (!pci) return;
    auto& v = h[pci]; v.push_back(val);
    if (v.size() > mx) v.erase(v.begin());
}

void processJsonObject(const json& jdata, LocationData* data) {
    float lat = getFloatValue(jdata, "latitude", 0.0f);
    float lon = getFloatValue(jdata, "longitude", 0.0f);
    float alt = getFloatValue(jdata, "altitude", 0.0f);
    float acc = getFloatValue(jdata, "accuracy", 0.0f);
    std::string time = convertTimeFormat(getStringValue(jdata, "time", ""));
    std::string netType = getStringValue(jdata, "networkType", "Unknown");
    std::string netOp = getStringValue(jdata, "networkOperator", "");
    std::string netOpName = getStringValue(jdata, "networkOperatorName", "");

    int lteCellId = getIntValue(jdata, "lteCellId", 0), lteEarfcn = getIntValue(jdata, "lteEarfcn", 0);
    int lteMcc = getIntValue(jdata, "lteMcc", 0), lteMnc = getIntValue(jdata, "lteMnc", 0);
    int ltePci = getIntValue(jdata, "ltePci", 0), lteTac = getIntValue(jdata, "lteTac", 0);
    int lteAsu = getIntValue(jdata, "lteAsuLevel", 0), lteCqi = getIntValue(jdata, "lteCqi", 0);
    int lteRsrp = getIntValue(jdata, "lteRsrp", 0), lteRsrq = getIntValue(jdata, "lteRsrq", 0);
    int lteRssi = getIntValue(jdata, "lteRssi", 0), lteRssnr = getIntValue(jdata, "lteRssnr", 0);
    int lteTa = getIntValue(jdata, "lteTimingAdvance", 0);

    int gsmCellId = getIntValue(jdata, "gsmCellId", 0), gsmBsic = getIntValue(jdata, "gsmBsic", 0);
    int gsmArfcn = getIntValue(jdata, "gsmArfcn", 0), gsmLac = getIntValue(jdata, "gsmLac", 0);
    int gsmMcc = getIntValue(jdata, "gsmMcc", 0), gsmMnc = getIntValue(jdata, "gsmMnc", 0);
    int gsmPsc = getIntValue(jdata, "gsmPsc", 0), gsmDbm = getIntValue(jdata, "gsmDbm", 0);
    int gsmTa = getIntValue(jdata, "gsmTimingAdvance", 0);

    int nrBand = getIntValue(jdata, "nrBand", 0); long nrNci = getLongValue(jdata, "nrNci", 0L);
    int nrPci = getIntValue(jdata, "nrPci", 0), nrNrarfcn = getIntValue(jdata, "nrNrarfcn", 0);
    int nrTac = getIntValue(jdata, "nrTac", 0), nrMcc = getIntValue(jdata, "nrMcc", 0), nrMnc = getIntValue(jdata, "nrMnc", 0);
    int nrRsrp = getIntValue(jdata, "nrSsRsrp", 0), nrRsrq = getIntValue(jdata, "nrSsRsrq", 0);
    int nrSinr = getIntValue(jdata, "nrSsSinr", 0), nrTa = getIntValue(jdata, "nrTimingAdvance", 0);

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
            data->lteRsrpData.push_back(lteRsrp); data->lteRsrqData.push_back(lteRsrq);
            data->lteRssiData.push_back(lteRssi); data->lteRssnrData.push_back(lteRssnr);
            data->lteAsuData.push_back(lteAsu); data->lteCqiData.push_back(lteCqi);
            data->lteTimingAdvanceData.push_back(lteTa);
            if (data->lteRsrpData.size() > 100) {
                data->lteRsrpData.erase(data->lteRsrpData.begin());
                data->lteRsrqData.erase(data->lteRsrqData.begin());
                data->lteRssiData.erase(data->lteRssiData.begin());
                data->lteRssnrData.erase(data->lteRssnrData.begin());
                data->lteAsuData.erase(data->lteAsuData.begin());
                data->lteCqiData.erase(data->lteCqiData.begin());
                data->lteTimingAdvanceData.erase(data->lteTimingAdvanceData.begin());
            }
            if (jdata.contains("cells") && jdata["cells"].is_array()) {
                for (const auto& cell : jdata["cells"]) {
                    int pci = getIntValue(cell, "pci", 0);
                    if (!pci) continue;
                    updateHistory(data->lteRsrpHistory, pci, (float)getIntValue(cell, "rsrp", 0));
                    updateHistory(data->lteRsrqHistory, pci, (float)getIntValue(cell, "rsrq", 0));
                    updateHistory(data->lteRssiHistory, pci, (float)getIntValue(cell, "rssi", 0));
                    updateHistory(data->lteRssnrHistory, pci, (float)getIntValue(cell, "rssnr", 0));
                    updateHistory(data->lteAsuHistory, pci, getIntValue(cell, "asuLevel", 0));
                    updateHistory(data->lteCqiHistory, pci, getIntValue(cell, "cqi", 0));
                    updateHistory(data->lteTimingAdvanceHistory, pci, getIntValue(cell, "timingAdvance", 0));
                }
            } else if (ltePci != 0) {
                updateHistory(data->lteRsrpHistory, ltePci, (float)lteRsrp);
                updateHistory(data->lteRsrqHistory, ltePci, (float)lteRsrq);
                updateHistory(data->lteRssiHistory, ltePci, (float)lteRssi);
                updateHistory(data->lteRssnrHistory, ltePci, (float)lteRssnr);
                updateHistory(data->lteAsuHistory, ltePci, lteAsu);
                updateHistory(data->lteCqiHistory, ltePci, lteCqi);
                updateHistory(data->lteTimingAdvanceHistory, ltePci, lteTa);
            }
        } else if (netType == "NR") {
            data->nrSsRsrpData.push_back(nrRsrp); data->nrSsRsrqData.push_back(nrRsrq);
            data->nrSsSinrData.push_back(nrSinr); data->nrTimingAdvanceData.push_back(nrTa);
            if (data->nrSsRsrpData.size() > 100) {
                data->nrSsRsrpData.erase(data->nrSsRsrpData.begin());
                data->nrSsRsrqData.erase(data->nrSsRsrqData.begin());
                data->nrSsSinrData.erase(data->nrSsSinrData.begin());
                data->nrTimingAdvanceData.erase(data->nrTimingAdvanceData.begin());
            }
            if (jdata.contains("cells") && jdata["cells"].is_array()) {
                for (const auto& cell : jdata["cells"]) {
                    int pci = getIntValue(cell, "pci", 0);
                    if (!pci) continue;
                    updateHistory(data->nrRsrpHistory, pci, (float)getIntValue(cell, "ssRsrp", 0));
                    updateHistory(data->nrRsrqHistory, pci, (float)getIntValue(cell, "ssRsrq", 0));
                    updateHistory(data->nrSinrHistory, pci, (float)getIntValue(cell, "ssSinr", 0));
                    updateHistory(data->nrTimingAdvanceHistory, pci, getIntValue(cell, "timingAdvance", 0));
                }
            } else if (nrPci != 0) {
                updateHistory(data->nrRsrpHistory, nrPci, (float)nrRsrp);
                updateHistory(data->nrRsrqHistory, nrPci, (float)nrRsrq);
                updateHistory(data->nrSinrHistory, nrPci, (float)nrSinr);
                updateHistory(data->nrTimingAdvanceHistory, nrPci, nrTa);
            }
        }
        recordsInserted++;
    } else {
        recordsSkipped++;
    }

    if ((recordsInserted + recordsSkipped) % 10 == 0) {
        std::cout << "Вставлено: " << recordsInserted << ", пропущено: " << recordsSkipped << "\n";
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
            if (jdata.is_array()) {
                for (const auto& item : jdata) {
                    processJsonObject(item, data);
                }
            } else {
                processJsonObject(jdata, data);
            }
            sock.send(zmq::buffer("ACK"), zmq::send_flags::none);
        } catch (const std::exception& e) { 
            std::cerr << "ZMQ: " << e.what() << "\n"; 
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
        }
    }
}