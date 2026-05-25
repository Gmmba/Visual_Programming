#include "globals.h"

std::vector<MapPoint> g_dbPoints;
std::mutex g_dbPointsMutex;
bool g_showDbPoints = true;
int g_maxDbPoints = 200;
std::string g_dbConnStr;

DatabaseManager* dbManager = nullptr;
int recordsInserted = 0;
int recordsSkipped = 0;

std::string convertTimeFormat(const std::string& input) {
    return input.substr(6,4) + "-" + input.substr(3,2) + "-" + input.substr(0,2) + " " + input.substr(11,8);
}

DatabaseManager::DatabaseManager(const DBConfig& cfg) : config(cfg) {
    connection_string = "host=" + config.host + " port=" + config.port +
                        " dbname=" + config.dbname + " user=" + config.user +
                        " password=" + config.password;
}

bool DatabaseManager::testConnection() {
    try {
        pqxx::connection c(connection_string);
        if (c.is_open()) { 
            std::cout << "PostgreSQL OK\n"; 
            return true; 
        }
    } catch (const std::exception& e) { 
        std::cerr << e.what() << "\n"; 
    }
    return false;
}

bool DatabaseManager::insertLocationData(const LocationData& d) {
    try {
        pqxx::connection conn(connection_string);
        pqxx::work txn(conn);
        
        if (d.networkType == "LTE") {
            auto res = txn.exec_params(
                "INSERT INTO lte_data(latitude,longitude,altitude,accuracy,time,networkType,rsrp,rsrq,rssi,rssnr,timing_advance,cell_id,earfcn,mcc,mnc,pci,tac,asu_level,cqi) "
                "VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18,$19) ON CONFLICT(time) DO NOTHING",
                d.latitude, d.longitude, d.altitude, d.accuracy, d.timestamp, d.networkType,
                d.lteRsrp, d.lteRsrq, d.lteRssi, d.lteRssnr, d.lteTimingAdvance,
                d.lteCellId, d.lteEarfcn, d.lteMcc, d.lteMnc, d.ltePci, d.lteTac,
                d.lteAsuLevel, d.lteCqi);
            txn.commit();
            return res.affected_rows() > 0;
        }
        if (d.networkType == "NR") {
            auto res = txn.exec_params(
                "INSERT INTO nr_data(latitude,longitude,altitude,accuracy,time,networkType,band,nr_nci,nr_pci,nr_nrarfcn,nr_tac,nr_mcc,nr_mnc,nr_ss_rsrp,nr_ss_rsrq,nr_ss_sinr,nr_timing_advance) "
                "VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17) ON CONFLICT(time) DO NOTHING",
                d.latitude, d.longitude, d.altitude, d.accuracy, d.timestamp, d.networkType,
                d.nrBand, d.nrNci, d.nrPci, d.nrNrarfcn, d.nrTac, d.nrMcc, d.nrMnc,
                d.nrSsRsrp, d.nrSsRsrq, d.nrSsSinr, d.nrTimingAdvance);
            txn.commit();
            return res.affected_rows() > 0;
        }
        if (d.networkType == "GSM") {
            auto res = txn.exec_params(
                "INSERT INTO gsm_data(latitude,longitude,altitude,accuracy,time,networkType,cell_id,gsm_bsic,gsm_arfcn,gsm_lac,gsm_mcc,gsm_mnc,gsm_psc,gsm_dbm,gsm_timing_advance) "
                "VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15) ON CONFLICT(time) DO NOTHING",
                d.latitude, d.longitude, d.altitude, d.accuracy, d.timestamp, d.networkType,
                d.gsmCellId, d.gsmBsic, d.gsmArfcn, d.gsmLac, d.gsmMcc, d.gsmMnc,
                d.gsmPsc, d.gsmDbm, d.gsmTimingAdvance);
            txn.commit();
            return res.affected_rows() > 0;
        }
        return false;
    } catch (const std::exception& e) {
        std::cerr << "DB insert: " << e.what() << "\n";
        return false;
    }
}

void fetchDBPoints() {
    try {
        pqxx::connection conn(g_dbConnStr);
        pqxx::work txn(conn);
        std::string q = "SELECT latitude, longitude FROM ("
                        "SELECT latitude, longitude, time FROM lte_data UNION ALL "
                        "SELECT latitude, longitude, time FROM nr_data UNION ALL "
                        "SELECT latitude, longitude, time FROM gsm_data) t "
                        "WHERE latitude!=0 AND longitude!=0 ORDER BY time DESC LIMIT " + std::to_string(g_maxDbPoints);
        
        auto res = txn.exec(q);
        std::vector<MapPoint> pts;
        pts.reserve(res.size());
        for (const auto& row : res) {
            pts.push_back({row["latitude"].as<double>(), row["longitude"].as<double>()});
        }
        std::lock_guard<std::mutex> lk(g_dbPointsMutex);
        g_dbPoints = std::move(pts);
    } catch (const std::exception& e) {
        std::cerr << "[fetchDBPoints] " << e.what() << "\n";
    }
}