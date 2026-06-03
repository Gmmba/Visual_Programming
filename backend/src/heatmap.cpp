#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "globals.h"
std::vector<HeatmapPoint> g_hmPoints;
std::mutex g_hmPointsMutex;
HeatmapCriteria g_hmCriteria = HeatmapCriteria::RSRP;
bool g_hmShow = true;
float g_hmRadius = 80.f;
std::map<std::string, TextureData> g_hmCache;
std::mutex g_hmCacheMutex;
std::queue<HeatmapJob> g_hmQueue;
std::mutex g_hmQueueMutex;
int g_hmEarfcn = 0;
int g_hmPci = 0;
struct Color { int r, g, b; };
struct Stop { double t; Color c; };
std::string hmCriteriaStr(HeatmapCriteria c) {
    switch(c) {
        case HeatmapCriteria::RSRP: return "rsrp";
        case HeatmapCriteria::RSRQ: return "rsrq";
        case HeatmapCriteria::RSSI: return "rssi";
        case HeatmapCriteria::Altitude: return "altitude";
    }
    return "rsrp";
}
void fetchHeatmapPoints() {
    try {
        pqxx::connection conn(g_dbConnStr);
        pqxx::work txn(conn);
        pqxx::result res = txn.exec("SELECT latitude,longitude,earfcn,pci,"
            "COALESCE(rsrp,0)rsrp,COALESCE(rsrq,0)rsrq,COALESCE(rssi,0)rssi,"
            "COALESCE(altitude,0)altitude FROM lte_data WHERE latitude!=0 AND longitude!=0 ORDER BY time DESC");
        std::vector<HeatmapPoint> pts;
        pts.reserve(res.size());
        for(size_t i=0; i<res.size(); ++i) {
            HeatmapPoint p;
            p.lat = res[i][0].as<double>();
            p.lon = res[i][1].as<double>();
            p.earfcn = res[i][2].as<int>(0);
            p.pci = res[i][3].as<int>(0);
            p.rsrp = res[i][4].as<float>();
            p.rsrq = res[i][5].as<float>();
            p.rssi = res[i][6].as<float>();
            p.altitude = res[i][7].as<float>();
            pts.push_back(p);
        }
        std::lock_guard<std::mutex> lk(g_hmPointsMutex);
        g_hmPoints = std::move(pts);
    } catch(const std::exception& e) {
        std::cerr << "[HM] " << e.what() << "\n";
    }
}
Color gradientColor(Color c1, Color c2, double ratio) {
    if(ratio > 1.0) ratio = 1.0;
    return {(int)(c1.r+(c2.r-c1.r)*ratio), (int)(c1.g+(c2.g-c1.g)*ratio), (int)(c1.b+(c2.b-c1.b)*ratio)};
}
Color signalToColor(double t) {
    static const Stop stops[] = {
        {0.00,{48,18,59}}, {0.15,{40,120,240}}, {0.30,{20,230,200}}, {0.45,{90,245,60}},
        {0.60,{220,235,27}}, {0.75,{251,145,5}}, {0.90,{200,30,10}}, {1.00,{122,4,3}}
    };
    int N = 8;
    t = std::max(0.0, std::min(1.0, t));
    for(int i=1; i<N; ++i) {
        if(t <= stops[i].t) {
            double span = stops[i].t - stops[i-1].t;
            double ratio = (span < 1e-9) ? 0.0 : (t - stops[i-1].t) / span;
            return gradientColor(stops[i-1].c, stops[i].c, ratio);
        }
    }
    return stops[N-1].c;
}
double haversineMeters(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000.0;
    double phi1 = lat1 * M_PI / 180.0;
    double phi2 = lat2 * M_PI / 180.0;
    double dphi = (lat2 - lat1) * M_PI / 180.0;
    double dlambda = (lon2 - lon1) * M_PI / 180.0;
    double a = std::sin(dphi/2.0)*std::sin(dphi/2.0) + std::cos(phi1)*std::cos(phi2)*std::sin(dlambda/2.0)*std::sin(dlambda/2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return R * c;
}
float computeIDW(double lat, double lon, const std::vector<const HeatmapPoint*>& pts, HeatmapCriteria crit, float radiusM, double& outNearestM) {
    const double P = 1.0;
    double sumWeights = 0.0, sumWeightedValues = 0.0;
    outNearestM = 1e9;
    for(size_t i=0; i<pts.size(); ++i) {
        const HeatmapPoint* p = pts[i];
        double d = haversineMeters(lat, lon, p->lat, p->lon);
        if(d > radiusM) continue;
        if(d < outNearestM) outNearestM = d;
        float value = 0.0f;
        switch(crit) {
            case HeatmapCriteria::RSRP: value = p->rsrp; break;
            case HeatmapCriteria::RSRQ: value = p->rsrq; break;
            case HeatmapCriteria::RSSI: value = p->rssi; break;
            case HeatmapCriteria::Altitude: value = p->altitude; break;
        }
        if(d < 0.5) { outNearestM = 0.0; return value; }
        double w = 1.0 / std::pow(d, P);
        sumWeights += w;
        sumWeightedValues += w * (double)value;
    }
    if(sumWeights < 1e-12) return NAN;
    return (float)(sumWeightedValues / sumWeights);
}
double mercYToLat(double mercY) {
    return 2.0 * std::atan(std::exp(mercY * M_PI / 180.0)) * 180.0 / M_PI - 90.0;
}

std::vector<uint8_t> renderTile(int zoom, int tileX, int tileY, const std::vector<HeatmapPoint>& allPoints, HeatmapCriteria crit, float baseRadiusM, int earfcnFilter, int pciFilter) {
    const int SZ = 256;
    std::vector<uint8_t> image(SZ * SZ * 4, 0);
    double mercX0 = TileXToMercatorX(tileX, zoom);
    double mercX1 = TileXToMercatorX(tileX + 1, zoom);
    double mercY0 = TileYToMercatorY(tileY, zoom);
    double mercY1 = TileYToMercatorY(tileY + 1, zoom);
    double latTop = mercYToLat(mercY0);
    double latBottom = mercYToLat(mercY1);
    double midLat = (latTop + latBottom) / 2.0;
    double cosLat = std::cos(midLat * M_PI / 180.0);
    float radiusM = baseRadiusM;
    int delta = zoom - 14;
    if(delta > 0) radiusM = baseRadiusM / std::pow(2.0f, (float)delta);
    else if(delta < 0) radiusM = baseRadiusM * std::pow(2.0f, (float)(-delta));
    radiusM = std::max(5.0f, std::min(radiusM, 500000.0f));
    double latMarginDeg = radiusM / 111320.0;
    double lonMarginDeg = (cosLat > 1e-6) ? radiusM / (111320.0 * cosLat) : 1.0;
    std::vector<const HeatmapPoint*> nearbyPoints;
    for(size_t i = 0; i < allPoints.size(); ++i) {
        const HeatmapPoint& p = allPoints[i];
        if(earfcnFilter != 0 && p.earfcn != earfcnFilter) continue;
        if(pciFilter != 0 && p.pci != pciFilter) continue;
        if(p.lat < latBottom - latMarginDeg || p.lat > latTop + latMarginDeg) continue;
        if(p.lon < mercX0 - lonMarginDeg || p.lon > mercX1 + lonMarginDeg) continue;
        nearbyPoints.push_back(&p);
    }
    if(nearbyPoints.empty()) return image;
    float valMin, valMax;
    if(crit == HeatmapCriteria::RSRP) { valMin = -110.0f; valMax = -80.0f; }
    else if(crit == HeatmapCriteria::RSRQ) { valMin = -20.0f; valMax = -3.0f; }
    else if(crit == HeatmapCriteria::RSSI) { valMin = -110.0f; valMax = -40.0f; }
    else {
        valMin = 1e9f; valMax = -1e9f;
        for(size_t i = 0; i < nearbyPoints.size(); ++i) {
            const HeatmapPoint* p = nearbyPoints[i];
            if(p->altitude < valMin) valMin = p->altitude;
            if(p->altitude > valMax) valMax = p->altitude;
        }
        if(valMax - valMin < 1.0f) { 
            valMin -= 0.5f; valMax += 0.5f; 
        }
    }
    for(int py = 0; py < SZ; ++py) {
        double mercY = mercY0 + (py + 0.5) / SZ * (mercY1 - mercY0);
        double lat = mercYToLat(mercY);
        for(int px = 0; px < SZ; ++px) {
            double lon = mercX0 + (px + 0.5) / SZ * (mercX1 - mercX0);
            double nearestM = 1e9;
            float idwValue = computeIDW(lat, lon, nearbyPoints, crit, radiusM, nearestM);
            if(std::isnan(idwValue)) continue;
            float clamped = std::max(valMin, std::min(valMax, idwValue));
            double ratio = (double)(clamped - valMin) / (valMax - valMin);
            Color color = signalToColor(ratio);
            float distRatio = (float)std::min(nearestM / radiusM, 1.0);
            uint8_t alpha = (uint8_t)((1.0f - distRatio * 0.75f) * 210.0f);
            int index = (py * SZ + px) * 4;
            image[index + 0] = (uint8_t)color.r;
            image[index + 1] = (uint8_t)color.g;
            image[index + 2] = (uint8_t)color.b;
            image[index + 3] = alpha;
        }
    }
    return image;
}
void HeatmapWorker() {
    while(true) {
        HeatmapJob job;
        {
            std::unique_lock<std::mutex> lk(g_hmQueueMutex);
            if(g_hmQueue.empty()) { lk.unlock(); std::this_thread::sleep_for(std::chrono::milliseconds(50)); continue; }
            job = g_hmQueue.front(); g_hmQueue.pop();
        }
        std::vector<HeatmapPoint> points;
        HeatmapCriteria crit;
        float radiusM;
        int earfcnFilter, pciFilter;
        {
            std::lock_guard<std::mutex> lk(g_hmPointsMutex);
            points = g_hmPoints; crit = g_hmCriteria; radiusM = g_hmRadius; earfcnFilter = g_hmEarfcn; pciFilter = g_hmPci;
        }
        if(points.empty()) { std::lock_guard<std::mutex> lk(g_hmCacheMutex); g_hmCache[job.id].isLoading = false; continue; }
        std::string tilePath = "heatmap/" + 
        hmCriteriaStr(crit) + "/r" + 
        std::to_string((int)radiusM) + "/e" + 
        std::to_string(earfcnFilter) + "/p" + 
        std::to_string(pciFilter) + "/" + 
        std::to_string(job.zoom) + "/" + 
        std::to_string(job.x) + "/" + 
        std::to_string(job.y) + ".png";
        std::vector<uint8_t> rgba;
        if(fs::exists(tilePath)) {
            int w, h, ch;
            uint8_t* pixels = stbi_load(tilePath.c_str(), &w, &h, &ch, 4);
            if(pixels) { 
                size_t totalBytes = w * h * 4;
                rgba.resize(totalBytes);
                for(size_t i = 0; i < totalBytes; ++i) {
                    rgba[i] = pixels[i];
                }
                stbi_image_free(pixels); 
            }
        }
        if(rgba.empty()) {
            rgba = renderTile(job.zoom, job.x, job.y, points, crit, radiusM, earfcnFilter, pciFilter);
            fs::create_directories(fs::path(tilePath).parent_path());
            stbi_write_png(tilePath.c_str(), 256, 256, 4, rgba.data(), 256 * 4);
        }
        {
            std::lock_guard<std::mutex> lk(g_hmCacheMutex);
            TextureData& tex = g_hmCache[job.id];
            tex.rgbaBlob = std::move(rgba); tex.width = 256; tex.height = 256; tex.isLoading = false;
        }
    }
}