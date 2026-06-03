#define STB_IMAGE_IMPLEMENTATION
#include "globals.h"

std::map<std::string, TextureData> g_TileCache;
std::queue<TileJob> g_JobQueue;
std::mutex g_JobMutex;
std::mutex g_CacheMutex;
double g_mapCenterLat = 55.0;
double g_mapCenterLon = 83.0;
int g_mapZoom = 14;

double MercatorXToTileX(double mercX, int zoom) { return (0.5 + mercX / 360.0) * (1 << zoom); }
double MercatorYToTileY(double mercY, int zoom) { return (0.5 - mercY / 360.0) * (1 << zoom); }
double TileXToMercatorX(int tileX, int zoom) { return (tileX / static_cast<double>(1 << zoom) - 0.5) * 360.0; }
double TileYToMercatorY(int tileY, int zoom) { return (0.5 - tileY / static_cast<double>(1 << zoom)) * 360.0; }
double latToMercY(double lat) { double r = lat * M_PI / 180.0; return std::log(std::tan(M_PI / 4.0 + r / 2.0)) * 180.0 / M_PI; }

std::string getTilePath(int zoom, int tx, int ty) {
    return "tiles/" + std::to_string(zoom) + "/" + std::to_string(tx) + "/" + std::to_string(ty) + ".png";
}

size_t onPullResponse(void* data, size_t size, size_t nmemb, void* userp) {
    auto& blob = *static_cast<std::vector<unsigned char>*>(userp);
    blob.insert(blob.end(), static_cast<unsigned char*>(data), static_cast<unsigned char*>(data) + size * nmemb);
    return size * nmemb;
}

void FetchWorker() {
    CURL* curl = curl_easy_init();
    if (!curl) return;

    while (true) {
        TileJob job;
        {
            std::unique_lock<std::mutex> lk(g_JobMutex);
            if (g_JobQueue.empty()) { 
                lk.unlock(); std::this_thread::sleep_for(std::chrono::milliseconds(50)); 
                continue; 
            }
            job = g_JobQueue.front(); g_JobQueue.pop();
        }

        std::string path = getTilePath(job.zoom, job.x, job.y);
        std::vector<unsigned char> rawBlob;
        if (fs::exists(path)) {
            std::ifstream ifs(path, std::ios::binary);
            if (ifs) {
                rawBlob.clear();
                char byte;
                while (ifs.get(byte)) {
                    rawBlob.push_back(static_cast<unsigned char>(byte));
                }
            }
        }

        if (rawBlob.empty()) {
            fs::create_directories(fs::path(path).parent_path());
            std::ostringstream url;
            url << "https://tile.openstreetmap.org/" << job.zoom << '/' << job.x << '/' << job.y << ".png";
            curl_easy_reset(curl);
            curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "LocationMonitor/1.0");
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rawBlob);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onPullResponse);

            if (curl_easy_perform(curl) != CURLE_OK) {
                std::lock_guard<std::mutex> lk(g_CacheMutex);
                g_TileCache[job.id].isLoading = false; continue;
            }
            long http = 0; curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http);
            if (http != 200) {
                std::lock_guard<std::mutex> lk(g_CacheMutex);
                g_TileCache[job.id].isLoading = false; continue;
            }
            std::ofstream ofs(path, std::ios::binary);
            ofs.write(reinterpret_cast<const char*>(rawBlob.data()), rawBlob.size());
        }
        if (rawBlob.empty()) continue;

            int w = 0, h = 0, ch = 0;
            unsigned char* pixels = stbi_load_from_memory(rawBlob.data(), (int)rawBlob.size(), &w, &h, &ch, STBI_rgb_alpha);
            if (!pixels) { 
                std::lock_guard<std::mutex> lk(g_CacheMutex); 
                g_TileCache[job.id].isLoading = false; 
                continue; 
            }

            {
                std::lock_guard<std::mutex> lk(g_CacheMutex);
                auto& tex = g_TileCache[job.id];
                tex.rgbaBlob.clear();
                for (int i = 0; i < w * h * 4; ++i) {
                    tex.rgbaBlob.push_back(pixels[i]);
                }
                tex.width = w; tex.height = h; tex.isLoading = false;
            }
            stbi_image_free(pixels);
        }
        curl_easy_cleanup(curl);
}