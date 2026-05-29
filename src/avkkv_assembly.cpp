// ============================================================
// TUBE GENERATOR — Трубки АВККВ (бункер ↔ PEM)
// Компиляция: g++ -O3 -std=c++11 -o tube_gen.exe tube_generator.cpp
// ============================================================

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <cstring>
#include <cstdint>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

struct P { float x, y, z; };

// ==================== БИНАРНЫЙ STL (без изменений) ====================
struct STLHeader {
    char header[80];
    uint32_t num_triangles;
};

#pragma pack(push, 1)
struct STLTriangle {
    float normal[3];
    float v1[3], v2[3], v3[3];
    uint16_t attribute;
};
#pragma pack(pop)

class BinarySTL {
    std::ofstream file;
    std::vector<STLTriangle> buffer;
    uint32_t count = 0;
    bool closed = false;
public:
    BinarySTL() {}
    void open(const char* filename) {
        file.open(filename, std::ios::binary);
        STLHeader hdr = {};
        strncpy(hdr.header, "Tube Generator v1.0", 80);
        hdr.num_triangles = 0;
        file.write((char*)&hdr, sizeof(STLHeader));
        count = 0;
        closed = false;
    }
    void addTriangle(P v1, P v2, P v3) {
        P normal;
        P d1 = {v2.x-v1.x, v2.y-v1.y, v2.z-v1.z};
        P d2 = {v3.x-v1.x, v3.y-v1.y, v3.z-v1.z};
        normal.x = d1.y*d2.z - d1.z*d2.y;
        normal.y = d1.z*d2.x - d1.x*d2.z;
        normal.z = d1.x*d2.y - d1.y*d2.x;
        float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
        if(len > 0.0000001f) { normal.x /= len; normal.y /= len; normal.z /= len; }
        
        STLTriangle tri;
        tri.normal[0] = normal.x; tri.normal[1] = normal.y; tri.normal[2] = normal.z;
        tri.v1[0] = v1.x; tri.v1[1] = v1.y; tri.v1[2] = v1.z;
        tri.v2[0] = v2.x; tri.v2[1] = v2.y; tri.v2[2] = v2.z;
        tri.v3[0] = v3.x; tri.v3[1] = v3.y; tri.v3[2] = v3.z;
        tri.attribute = 0;
        buffer.push_back(tri);
        count++;
        if(buffer.size() >= 10000) flush();
    }
    void flush() {
        if(!buffer.empty()) {
            file.write((char*)buffer.data(), buffer.size() * sizeof(STLTriangle));
            buffer.clear();
        }
    }
    void close() {
        if(closed) return;
        flush();
        file.seekp(80, std::ios::beg);
        file.write((char*)&count, sizeof(uint32_t));
        file.close();
        closed = true;
    }
    ~BinarySTL() { close(); }
};

// ==================== МАТЕМАТИКА (без изменений) ====================
inline P normalize(P v) {
    float l = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if(l < 0.0000001f) return {0,0,0};
    return {v.x/l, v.y/l, v.z/l};
}

inline P cross(P a, P b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}

// ==================== ТРУБКА (CABLE) (без изменений) ====================
void cable_segment(BinarySTL& stl, P start, P end, float radius, int segments = 16) {
    P dir = {end.x - start.x, end.y - start.y, end.z - start.z};
    float length = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    if(length < 0.0001f) return;
    dir = normalize(dir);
    
    P up = {0, 1, 0};
    if(fabsf(dir.y) > 0.999f) up = {1, 0, 0};
    
    P right = normalize(cross(dir, up));
    P up_real = normalize(cross(right, dir));
    
    float step = 360.0f / segments;
    for(float a = 0; a < 360; a += step) {
        float a1 = a * M_PI / 180.0f;
        float a2 = (a + step) * M_PI / 180.0f;
        P o1 = {right.x*cosf(a1) + up_real.x*sinf(a1),
                right.y*cosf(a1) + up_real.y*sinf(a1),
                right.z*cosf(a1) + up_real.z*sinf(a1)};
        P o2 = {right.x*cosf(a2) + up_real.x*sinf(a2),
                right.y*cosf(a2) + up_real.y*sinf(a2),
                right.z*cosf(a2) + up_real.z*sinf(a2)};
        
        P p1 = {start.x + o1.x*radius, start.y + o1.y*radius, start.z + o1.z*radius};
        P p2 = {start.x + o2.x*radius, start.y + o2.y*radius, start.z + o2.z*radius};
        P p3 = {end.x + o2.x*radius, end.y + o2.y*radius, end.z + o2.z*radius};
        P p4 = {end.x + o1.x*radius, end.y + o1.y*radius, end.z + o1.z*radius};
        stl.addTriangle(p1, p2, p3);
        stl.addTriangle(p1, p3, p4);
    }
    
    // Крышки
    for(float a = 0; a < 360; a += step) {
        float a1 = a * M_PI / 180.0f;
        float a2 = (a + step) * M_PI / 180.0f;
        P o1 = {right.x*cosf(a1) + up_real.x*sinf(a1),
                right.y*cosf(a1) + up_real.y*sinf(a1),
                right.z*cosf(a1) + up_real.z*sinf(a1)};
        P o2 = {right.x*cosf(a2) + up_real.x*sinf(a2),
                right.y*cosf(a2) + up_real.y*sinf(a2),
                right.z*cosf(a2) + up_real.z*sinf(a2)};
        stl.addTriangle(start,
            {start.x + o2.x*radius, start.y + o2.y*radius, start.z + o2.z*radius},
            {start.x + o1.x*radius, start.y + o1.y*radius, start.z + o1.z*radius});
        stl.addTriangle(end,
            {end.x + o1.x*radius, end.y + o1.y*radius, end.z + o1.z*radius},
            {end.x + o2.x*radius, end.y + o2.y*radius, end.z + o2.z*radius});
    }
}

int main() {
    std::cout << "=== AVKKV TUBE GENERATOR (v4 — final) ===\n";
    
    // ============================================================
    // Трубка газа: бункер → PEM
    // ============================================================
    {
        BinarySTL stl;
        stl.open("tube_bunker_to_pem_gas.stl");
        
        P p1 = {2.1085f,  0.1372f, -0.0940f};   // бункер
        P p2 = {2.1085f, -0.2500f, -0.0940f};   // вниз
        P p3 = {2.5150f, -0.5979f,  0.3227f};   // PEM
        P p4 = {2.5150f, -0.5979f,  0.3227f};
        
        cable_segment(stl, p1, p2, 0.006f, 16);
        cable_segment(stl, p2, p3, 0.006f, 16);
        
        std::cout << "  tube_bunker_to_pem_gas.stl — готово\n";
        stl.close();
    }
    
    // ============================================================
    // Трубка воды: PEM → бункер (верхний угол)
    // ============================================================
    {
        BinarySTL stl;
        stl.open("tube_pem_to_bunker_water.stl");
        
        P p1 = {2.5310f, -0.7060f,  0.2340f};   // PEM выход воды
        P p2 = {2.5310f, -0.7060f, -0.1260f};   // уходим к Z бункера
        P p3 = {2.1980f,  0.1740f, -0.1260f};   // поднимаемся в угол
        P p4 = {2.1980f,  0.1740f, -0.1260f};   // вход в бункер
        
        cable_segment(stl, p1, p2, 0.005f, 12);
        cable_segment(stl, p2, p3, 0.005f, 12);
        
        std::cout << "  tube_pem_to_bunker_water.stl — готово\n";
        stl.close();
    }
    
    std::cout << "\nГотово! Вставляй обе в 0,0,0.\n";
    return 0;
}
