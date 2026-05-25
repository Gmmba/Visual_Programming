#include "globals.h"

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    DBConfig dbConfig;
    dbManager = new DatabaseManager(dbConfig);
    g_dbConnStr = "host=" + dbConfig.host + " port=" + dbConfig.port +
                  " dbname=" + dbConfig.dbname + " user=" + dbConfig.user +
                  " password=" + dbConfig.password;
    fetchDBPoints();
    fetchHeatmapPoints();
    std::thread(HeatmapWorker).detach();
    std::thread(HeatmapWorker).detach();
    std::thread(HeatmapWorker).detach();
    std::thread(HeatmapWorker).detach();
    dbManager->testConnection();
    LocationData locationData;
    std::thread(FetchWorker).detach();
    std::thread server_thread(run_server, &locationData);
    run_gui(&locationData);
    server_thread.join();
    delete dbManager;
    curl_global_cleanup();
    return 0;
}