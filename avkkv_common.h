// avkkv_common.h
#pragma once
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <cstring>
#include <cstdint>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ==================== СТРУКТУРЫ ====================
struct P { float x, y, z; };

// ==================== МАТЕМАТИКА (ДО КЛАССА BinarySTL!) ====================
inline P lerp(P a, P b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

inline P cross(P a, P b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float len(P v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline P normalize(P v) {
    float l = len(v);
    if (l < 0.0000001f) return {0, 0, 0};
    return {v.x / l, v.y / l, v.z / l};
}

// ==================== БИНАРНЫЙ STL ====================
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
        strncpy(hdr.header, "AVKKV Generator v1.0", 80);
        hdr.num_triangles = 0;
        file.write((char*)&hdr, sizeof(STLHeader));
        count = 0;
        closed = false;
    }
    void addTriangle(P v1, P v2, P v3) {
        P normal = normalize(cross(
            {v2.x - v1.x, v2.y - v1.y, v2.z - v1.z},
            {v3.x - v1.x, v3.y - v1.y, v3.z - v1.z}
        ));
        STLTriangle tri;
        tri.normal[0] = normal.x; tri.normal[1] = normal.y; tri.normal[2] = normal.z;
        tri.v1[0] = v1.x; tri.v1[1] = v1.y; tri.v1[2] = v1.z;
        tri.v2[0] = v2.x; tri.v2[1] = v2.y; tri.v2[2] = v2.z;
        tri.v3[0] = v3.x; tri.v3[1] = v3.y; tri.v3[2] = v3.z;
        tri.attribute = 0;
        buffer.push_back(tri);
        count++;
        if (buffer.size() >= 10000) flush();
    }
    void flush() {
        if (!buffer.empty()) {
            file.write((char*)buffer.data(), buffer.size() * sizeof(STLTriangle));
            buffer.clear();
        }
    }
    void close() {
        if (closed) return;
        flush();
        file.seekp(80, std::ios::beg);
        file.write((char*)&count, sizeof(uint32_t));
        file.close();
        closed = true;
    }
    ~BinarySTL() { close(); }
};

// ==================== СТРУКТУРА ОКНА ====================
struct Window {
    float x_center;
    float x_half;
    float angle_deg;
    float angle_half;
};

// ==================== ПРИМИТИВЫ (ВСЕ С inline!) ====================

inline void fct(BinarySTL& stl, P p1, P p2, P p3) {
    stl.addTriangle(p1, p2, p3);
}

inline void cylinder_hollow(
    BinarySTL& stl,
    float x1, float x2,
    float r_out, float r_in,
    int segments,
    float offset_y = 0, float offset_z = 0,
    const std::vector<Window>& windows = {}
) {
    float step = 360.0f / segments;
    
    for(float a = 0; a < 360; a += step) {
        float a1 = a * M_PI / 180.0f;
        float a2 = (a + step) * M_PI / 180.0f;
        
        bool skip = false;
        for(auto& w : windows) {
            if(x1 >= w.x_center + w.x_half || x2 <= w.x_center - w.x_half) continue;
            float diff = fabsf(a + step/2 - w.angle_deg);
            if(diff > 180) diff = 360 - diff;
            if(diff < w.angle_half) { skip = true; break; }
        }
        if(skip) continue;
        
        fct(stl,
            {x1, offset_y + r_out*sinf(a1), offset_z + r_out*cosf(a1)},
            {x1, offset_y + r_out*sinf(a2), offset_z + r_out*cosf(a2)},
            {x2, offset_y + r_out*sinf(a2), offset_z + r_out*cosf(a2)});
        fct(stl,
            {x1, offset_y + r_out*sinf(a1), offset_z + r_out*cosf(a1)},
            {x2, offset_y + r_out*sinf(a2), offset_z + r_out*cosf(a2)},
            {x2, offset_y + r_out*sinf(a1), offset_z + r_out*cosf(a1)});
        
        if(r_in > 0.001f) {
            fct(stl,
                {x2, offset_y + r_in*sinf(a1), offset_z + r_in*cosf(a1)},
                {x2, offset_y + r_in*sinf(a2), offset_z + r_in*cosf(a2)},
                {x1, offset_y + r_in*sinf(a2), offset_z + r_in*cosf(a2)});
            fct(stl,
                {x2, offset_y + r_in*sinf(a1), offset_z + r_in*cosf(a1)},
                {x1, offset_y + r_in*sinf(a2), offset_z + r_in*cosf(a2)},
                {x1, offset_y + r_in*sinf(a1), offset_z + r_in*cosf(a1)});
        }
    }
    
    if(r_in > 0.001f) {
        for(float a = 0; a < 360; a += step) {
            float a1 = a * M_PI / 180.0f;
            float a2 = (a + step) * M_PI / 180.0f;
            bool skip_x1 = false, skip_x2 = false;
            for(auto& w : windows) {
                if(x1 < w.x_center + w.x_half && x1 > w.x_center - w.x_half) {
                    float diff = fabsf(a + step/2 - w.angle_deg);
                    if(diff > 180) diff = 360 - diff;
                    if(diff < w.angle_half) skip_x1 = true;
                }
                if(x2 < w.x_center + w.x_half && x2 > w.x_center - w.x_half) {
                    float diff = fabsf(a + step/2 - w.angle_deg);
                    if(diff > 180) diff = 360 - diff;
                    if(diff < w.angle_half) skip_x2 = true;
                }
            }
            if(!skip_x1) {
                P a1o = {x1, offset_y + r_out*sinf(a1), offset_z + r_out*cosf(a1)};
                P a2o = {x1, offset_y + r_out*sinf(a2), offset_z + r_out*cosf(a2)};
                P a1i = {x1, offset_y + r_in*sinf(a1), offset_z + r_in*cosf(a1)};
                P a2i = {x1, offset_y + r_in*sinf(a2), offset_z + r_in*cosf(a2)};
                fct(stl, a1i, a2i, a2o);
                fct(stl, a1i, a2o, a1o);
            }
            if(!skip_x2) {
                P b1o = {x2, offset_y + r_out*sinf(a1), offset_z + r_out*cosf(a1)};
                P b2o = {x2, offset_y + r_out*sinf(a2), offset_z + r_out*cosf(a2)};
                P b1i = {x2, offset_y + r_in*sinf(a1), offset_z + r_in*cosf(a1)};
                P b2i = {x2, offset_y + r_in*sinf(a2), offset_z + r_in*cosf(a2)};
                fct(stl, b1i, b1o, b2o);
                fct(stl, b1i, b2o, b2i);
            }
        }
    } else {
        for(float a = 0; a < 360; a += step) {
            float a1 = a * M_PI / 180.0f;
            float a2 = (a + step) * M_PI / 180.0f;
            fct(stl, {x1, offset_y, offset_z},
                {x1, offset_y + r_out*sinf(a2), offset_z + r_out*cosf(a2)},
                {x1, offset_y + r_out*sinf(a1), offset_z + r_out*cosf(a1)});
            fct(stl, {x2, offset_y, offset_z},
                {x2, offset_y + r_out*sinf(a1), offset_z + r_out*cosf(a1)},
                {x2, offset_y + r_out*sinf(a2), offset_z + r_out*cosf(a2)});
        }
    }
}

inline void cylinder_solid(BinarySTL& stl, float x1, float x2, float r, int segments,
                    float offset_y = 0, float offset_z = 0) {
    cylinder_hollow(stl, x1, x2, r, 0.0f, segments, offset_y, offset_z);
}

inline void cable_segment(BinarySTL& stl, P start, P end, float radius, int segments = 16) {
    P dir = {end.x - start.x, end.y - start.y, end.z - start.z};
    float length = len(dir);
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
        fct(stl, p1, p2, p3);
        fct(stl, p1, p3, p4);
    }
    
    for(float a = 0; a < 360; a += step) {
        float a1 = a * M_PI / 180.0f;
        float a2 = (a + step) * M_PI / 180.0f;
        P o1 = {right.x*cosf(a1) + up_real.x*sinf(a1),
                right.y*cosf(a1) + up_real.y*sinf(a1),
                right.z*cosf(a1) + up_real.z*sinf(a1)};
        P o2 = {right.x*cosf(a2) + up_real.x*sinf(a2),
                right.y*cosf(a2) + up_real.y*sinf(a2),
                right.z*cosf(a2) + up_real.z*sinf(a2)};
        fct(stl, start,
            {start.x + o2.x*radius, start.y + o2.y*radius, start.z + o2.z*radius},
            {start.x + o1.x*radius, start.y + o1.y*radius, start.z + o1.z*radius});
        fct(stl, end,
            {end.x + o1.x*radius, end.y + o1.y*radius, end.z + o1.z*radius},
            {end.x + o2.x*radius, end.y + o2.y*radius, end.z + o2.z*radius});
    }
}

inline void bolt(BinarySTL& stl, P pos, float radius, float height, int segments = 16) {
    float step = 360.0f / segments;
    float r_shank = radius * 0.6f;
    
    for(float a = 0; a < 360; a += step) {
        float a1 = a * M_PI / 180.0f;
        float a2 = (a + step) * M_PI / 180.0f;
        fct(stl,
            {pos.x, pos.y + radius*sinf(a1), pos.z + radius*cosf(a1)},
            {pos.x, pos.y + radius*sinf(a2), pos.z + radius*cosf(a2)},
            {pos.x + height*0.3f, pos.y + radius*sinf(a2), pos.z + radius*cosf(a2)});
        fct(stl,
            {pos.x, pos.y + radius*sinf(a1), pos.z + radius*cosf(a1)},
            {pos.x + height*0.3f, pos.y + radius*sinf(a2), pos.z + radius*cosf(a2)},
            {pos.x + height*0.3f, pos.y + radius*sinf(a1), pos.z + radius*cosf(a1)});
    }
    
    for(float a = 0; a < 360; a += step) {
        float a1 = a * M_PI / 180.0f;
        float a2 = (a + step) * M_PI / 180.0f;
        fct(stl,
            {pos.x + height*0.3f, pos.y + r_shank*sinf(a1), pos.z + r_shank*cosf(a1)},
            {pos.x + height*0.3f, pos.y + r_shank*sinf(a2), pos.z + r_shank*cosf(a2)},
            {pos.x + height, pos.y + r_shank*sinf(a2), pos.z + r_shank*cosf(a2)});
        fct(stl,
            {pos.x + height*0.3f, pos.y + r_shank*sinf(a1), pos.z + r_shank*cosf(a1)},
            {pos.x + height, pos.y + r_shank*sinf(a2), pos.z + r_shank*cosf(a2)},
            {pos.x + height, pos.y + r_shank*sinf(a1), pos.z + r_shank*cosf(a1)});
    }
    
    for(float a = 0; a < 360; a += step) {
        float a1 = a * M_PI / 180.0f;
        float a2 = (a + step) * M_PI / 180.0f;
        fct(stl,
            {pos.x, pos.y, pos.z},
            {pos.x, pos.y + radius*sinf(a2), pos.z + radius*cosf(a2)},
            {pos.x, pos.y + radius*sinf(a1), pos.z + radius*cosf(a1)});
    }
    
    for(float a = 0; a < 360; a += step) {
        float a1 = a * M_PI / 180.0f;
        float a2 = (a + step) * M_PI / 180.0f;
        fct(stl,
            {pos.x + height, pos.y, pos.z},
            {pos.x + height, pos.y + r_shank*sinf(a1), pos.z + r_shank*cosf(a1)},
            {pos.x + height, pos.y + r_shank*sinf(a2), pos.z + r_shank*cosf(a2)});
    }
    
    for(float a = 0; a < 360; a += step) {
        float a1 = a * M_PI / 180.0f;
        float a2 = (a + step) * M_PI / 180.0f;
        P a1o = {pos.x + height*0.3f, pos.y + radius*sinf(a1), pos.z + radius*cosf(a1)};
        P a2o = {pos.x + height*0.3f, pos.y + radius*sinf(a2), pos.z + radius*cosf(a2)};
        P a1i = {pos.x + height*0.3f, pos.y + r_shank*sinf(a1), pos.z + r_shank*cosf(a1)};
        P a2i = {pos.x + height*0.3f, pos.y + r_shank*sinf(a2), pos.z + r_shank*cosf(a2)};
        fct(stl, a1i, a2i, a2o);
        fct(stl, a1i, a2o, a1o);
    }
}

inline void cone_hollow(BinarySTL& stl, float x1, float x2, float r1, float r2,
                 float wall_thickness, int segments,
                 float offset_y = 0, float offset_z = 0) {
    float step = 360.0f / segments;
    float r1_inner = r1 - wall_thickness;
    float r2_inner = r2 - wall_thickness;
    if(r1_inner < 0) r1_inner = 0;
    if(r2_inner < 0) r2_inner = 0;
    
    for(float a = 0; a < 360; a += step) {
        float a1 = a * M_PI / 180.0f;
        float a2 = (a + step) * M_PI / 180.0f;
        
        fct(stl,
            {x1, offset_y + r1*sinf(a1), offset_z + r1*cosf(a1)},
            {x1, offset_y + r1*sinf(a2), offset_z + r1*cosf(a2)},
            {x2, offset_y + r2*sinf(a2), offset_z + r2*cosf(a2)});
        fct(stl,
            {x1, offset_y + r1*sinf(a1), offset_z + r1*cosf(a1)},
            {x2, offset_y + r2*sinf(a2), offset_z + r2*cosf(a2)},
            {x2, offset_y + r2*sinf(a1), offset_z + r2*cosf(a1)});
        
        if(r1_inner > 0.001f && r2_inner > 0.001f) {
            fct(stl,
                {x2, offset_y + r2_inner*sinf(a1), offset_z + r2_inner*cosf(a1)},
                {x2, offset_y + r2_inner*sinf(a2), offset_z + r2_inner*cosf(a2)},
                {x1, offset_y + r1_inner*sinf(a2), offset_z + r1_inner*cosf(a2)});
            fct(stl,
                {x2, offset_y + r2_inner*sinf(a1), offset_z + r2_inner*cosf(a1)},
                {x1, offset_y + r1_inner*sinf(a2), offset_z + r1_inner*cosf(a2)},
                {x1, offset_y + r1_inner*sinf(a1), offset_z + r1_inner*cosf(a1)});
        }
    }
}

inline void box(BinarySTL& stl, P min, P max) {
    P p[8] = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z},
        {max.x, max.y, min.z}, {min.x, max.y, min.z},
        {min.x, min.y, max.z}, {max.x, min.y, max.z},
        {max.x, max.y, max.z}, {min.x, max.y, max.z}
    };
    int faces[6][4] = {
        {0,1,2,3}, {4,7,6,5}, {0,4,5,1},
        {1,5,6,2}, {2,6,7,3}, {3,7,4,0}
    };
    for(int f = 0; f < 6; f++) {
        fct(stl, p[faces[f][0]], p[faces[f][1]], p[faces[f][2]]);
        fct(stl, p[faces[f][0]], p[faces[f][2]], p[faces[f][3]]);
    }
}