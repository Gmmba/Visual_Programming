#include "stb_image.h"
#include "stb_image_write.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <imgui.h>
#include <implot.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl2.h>
#include <curl/curl.h>
#include <pqxx/pqxx>
#include "zmq.hpp"
#include "json.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

struct TileJob { std::string id; int zoom, x, y; };
struct TextureData { GLuint id = 0; bool isLoading = false; std::vector<uint8_t> rgbaBlob; int width = 0, height = 0; };

extern std::map<std::string, TextureData> g_TileCache;
extern std::queue<TileJob> g_JobQueue;
extern std::mutex g_JobMutex;
extern std::mutex g_CacheMutex;
extern double g_mapCenterLat;
extern double g_mapCenterLon;
extern int g_mapZoom;

struct MapPoint { double lat, lon; };
extern std::vector<MapPoint> g_dbPoints;
extern std::mutex g_dbPointsMutex;
extern bool g_showDbPoints;
extern int g_maxDbPoints;
extern std::string g_dbConnStr;

struct DBConfig {
    std::string host = "127.0.0.1";
    std::string port = "5432";
    std::string dbname = "network_data";
    std::string user = "thunder.struck1";
    std::string password = "123456";
};

struct LocationData {
    float latitude=0, longitude=0, altitude=0, accuracy=0;
    std::string timestamp, networkType, networkOperator, networkOperatorName;
    std::vector<float> lteRsrpData, lteRsrqData, lteRssiData, lteRssnrData;
    std::vector<int> lteAsuData, lteCqiData, lteTimingAdvanceData;
    std::vector<float> nrSsRsrpData, nrSsRsrqData, nrSsSinrData;
    std::vector<int> nrTimingAdvanceData;
    std::map<int, std::vector<float>> lteRsrpHistory, lteRsrqHistory, lteRssiHistory, lteRssnrHistory;
    std::map<int, std::vector<int>> lteAsuHistory, lteCqiHistory, lteTimingAdvanceHistory;
    std::map<int, std::vector<float>> nrRsrpHistory, nrRsrqHistory, nrSinrHistory;
    std::map<int, std::vector<int>> nrTimingAdvanceHistory;
    int lteCellId=0, lteEarfcn=0, lteMcc=0, lteMnc=0, ltePci=0, lteTac=0;
    int lteAsuLevel=0, lteCqi=0, lteRsrp=0, lteRsrq=0, lteRssi=0, lteRssnr=0, lteTimingAdvance=0;
    int gsmCellId=0, gsmBsic=0, gsmArfcn=0, gsmLac=0, gsmMcc=0, gsmMnc=0, gsmPsc=0, gsmDbm=0, gsmTimingAdvance=0;
    int nrBand=0; long nrNci=0; int nrPci=0, nrNrarfcn=0, nrTac=0, nrMcc=0, nrMnc=0;
    int nrSsRsrp=0, nrSsRsrq=0, nrSsSinr=0, nrTimingAdvance=0;
    mutable std::mutex mtx;
};

class DatabaseManager {
    DBConfig config;
    std::string connection_string;
public:
    explicit DatabaseManager(const DBConfig& cfg);
    bool testConnection();
    bool insertLocationData(const LocationData& data);
};
extern DatabaseManager* dbManager;
extern int recordsInserted;
extern int recordsSkipped;

std::string getTilePath(int zoom, int tx, int ty);
size_t onPullResponse(void* data, size_t size, size_t nmemb, void* userp);
double MercatorXToTileX(double mercX, int zoom);
double MercatorYToTileY(double mercY, int zoom);
double TileXToMercatorX(int tileX, int zoom);
double TileYToMercatorY(int tileY, int zoom);
double latToMercY(double lat);
void FetchWorker();
std::string convertTimeFormat(const std::string& input);
void fetchDBPoints();
void run_server(LocationData* data);
void run_gui(LocationData* data);

enum class HeatmapCriteria { RSRP, RSRQ, RSSI, Altitude };
struct HeatmapPoint { double lat, lon; int earfcn, pci; float rsrp, rsrq, rssi, altitude; };
struct HeatmapJob { std::string id; int zoom, x, y; };

extern std::vector<HeatmapPoint> g_hmPoints;
extern std::mutex g_hmPointsMutex;
extern HeatmapCriteria g_hmCriteria;
extern bool g_hmShow;
extern float g_hmRadius;
extern std::map<std::string, TextureData> g_hmCache;
extern std::mutex g_hmCacheMutex;
extern std::queue<HeatmapJob> g_hmQueue;
extern std::mutex g_hmQueueMutex;
extern int g_hmEarfcn;
extern int g_hmPci;

void fetchHeatmapPoints();
void HeatmapWorker();
std::string hmCriteriaStr(HeatmapCriteria c);