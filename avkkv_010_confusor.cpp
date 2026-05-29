// ============================================================
// АВККВ-1.010.000 — ВХОДНОЙ КОНФУЗОР
// ============================================================
// Назначение: забор влажного озонированного воздуха, поджатие, подача в инжектор
// Начало координат: центр входного среза
// Ось X: по потоку (→)
// ============================================================
#include "avkkv_common.h"

void generate_010_confusor(const char* filename) {
    BinarySTL stl;
    stl.open(filename);
    
    // ==================== КОНСТАНТЫ ====================
    const float INLET_DIAM = 0.250f;        // Ø250 мм вход
    const float OUTLET_DIAM = 0.080f;        // Ø80 мм выход
    const float LENGTH = 0.500f;             // длина 500 мм
    const float ANGLE_DEG = 12.0f;           // полный угол 12°
    const float WALL_THICKNESS = 0.003f;     // 3 мм стенка (АМг5М)
    const float FLANGE_THICKNESS = 0.008f;   // 8 мм фланец
    const int SEGMENTS = 72;                 // сегментов по окружности
    
    const float MESH_CELL = 0.002f;          // ячейка сетки 2 мм
    const float MESH_WIRE = 0.0005f;         // проволока 0.5 мм
    
    const int GUIDE_VANES = 6;               // направляющих лопаток
    const float VANE_LENGTH = 0.060f;        // длина лопатки 60 мм
    const float VANE_THICKNESS = 0.002f;     // толщина 2 мм
    
    const float DRAIN_DIAM = 0.003f;         // дренаж Ø3 мм
    
    const float r_in = INLET_DIAM / 2.0f;    // 125 мм
    const float r_out = OUTLET_DIAM / 2.0f;  // 40 мм
    
    // ==================== ОСНОВНОЙ КОНУС ====================
    // Разбиваем на 50 сегментов по длине для плавности
    const int LONG_SEG = 50;
    const float dx = LENGTH / LONG_SEG;
    
    for(int i = 0; i < LONG_SEG; i++) {
        float x1 = i * dx;
        float x2 = (i + 1) * dx;
        float t1 = (float)i / LONG_SEG;
        float t2 = (float)(i + 1) / LONG_SEG;
        
        // Линейное уменьшение радиуса
        float r1 = r_in + (r_out - r_in) * t1;
        float r2 = r_in + (r_out - r_in) * t2;
        
        // Стенка
        cylinder_hollow(stl, x1, x2, r1 + WALL_THICKNESS, r1, SEGMENTS, 0, 0);
    }
    
    // ==================== ВХОДНАЯ ЗАЩИТНАЯ СЕТКА ====================
    float mesh_x = 0.0f;
    // Горизонтальные проволоки
    for(float my = -r_in + MESH_CELL; my < r_in; my += MESH_CELL) {
        for(float mz = -r_in + MESH_CELL; mz < r_in; mz += MESH_CELL) {
            if(my*my + mz*mz <= r_in*r_in) {
                // Горизонтальный отрезок
                cable_segment(stl,
                    {mesh_x, my, mz},
                    {mesh_x, my, mz + MESH_CELL},
                    MESH_WIRE, 4);
                // Вертикальный отрезок
                cable_segment(stl,
                    {mesh_x, my, mz},
                    {mesh_x, my + MESH_CELL, mz},
                    MESH_WIRE, 4);
            }
        }
    }
    // Кольцо по краю сетки
    cylinder_hollow(stl, mesh_x - 0.001f, mesh_x + 0.001f,
                    r_in + 0.002f, r_in - 0.001f, SEGMENTS, 0, 0);
    
    // ==================== НАПРАВЛЯЮЩИЕ ЛОПАТКИ ====================
    float vane_start_x = 0.010f;
    for(int v = 0; v < GUIDE_VANES; v++) {
        float ang = v * (360.0f / GUIDE_VANES) * M_PI / 180.0f;
        float vy = r_in * sinf(ang);
        float vz = r_in * cosf(ang);
        
        // Лопатка — тонкая пластина, идущая вдоль потока
        // Чуть наклонена для закрутки (угол атаки 5°)
        float twist = 5.0f * M_PI / 180.0f;
        
        for(float vx = 0; vx < VANE_LENGTH; vx += 0.004f) {
            float x1 = vane_start_x + vx;
            float x2 = vane_start_x + vx + 0.004f;
            
            // Радиус на этой позиции
            float t = (x1 + 0.002f) / LENGTH;
            float r_local = r_in + (r_out - r_in) * t;
            float ly = r_local * sinf(ang);
            float lz = r_local * cosf(ang);
            
            // Нормаль к поверхности лопатки
            float ny = sinf(ang + M_PI/2 + twist);
            float nz = cosf(ang + M_PI/2 + twist);
            
            fct(stl,
                {x1, ly, lz},
                {x1, ly + VANE_THICKNESS * ny, lz + VANE_THICKNESS * nz},
                {x2, ly + VANE_THICKNESS * ny, lz + VANE_THICKNESS * nz});
            fct(stl,
                {x1, ly, lz},
                {x2, ly + VANE_THICKNESS * ny, lz + VANE_THICKNESS * nz},
                {x2, ly, lz});
        }
    }
    
    // ==================== ДРЕНАЖНОЕ ОТВЕРСТИЕ ====================
    float drain_x = LENGTH * 0.5f;
    float drain_y = -(r_in + WALL_THICKNESS);  // нижняя точка
    float drain_r = DRAIN_DIAM / 2.0f;
    
    // Трубка дренажа (выходит наружу)
    cylinder_hollow(stl,
        drain_x - 0.003f, drain_x + 0.003f,
        drain_r + 0.001f, drain_r,
        12, drain_y, 0);
    // Отверстие в стенке
    cylinder_hollow(stl,
        drain_x - 0.002f, drain_x + 0.002f,
        drain_r + 0.0005f, drain_r,
        12, drain_y - (r_in + WALL_THICKNESS - drain_y)/2, 0);
    
    // ==================== ВЫХОДНОЙ ФЛАНЕЦ ====================
    float flange_x = LENGTH;
    float flange_r = r_out + 0.015f;  // +15 мм на фланец
    
    // Диск фланца
    cylinder_hollow(stl,
        flange_x, flange_x + FLANGE_THICKNESS,
        flange_r, r_out,
        SEGMENTS, 0, 0);
    
    // Отверстия под болты (4 шт, M8)
    float bolt_circle_r = r_out + 0.010f;  // Ø100 мм
    float bolt_hole_r = 0.0045f;           // 4.5 мм (M8 с зазором)
    for(int b = 0; b < 4; b++) {
        float bang = b * 90.0f * M_PI / 180.0f;
        float by = bolt_circle_r * sinf(bang);
        float bz = bolt_circle_r * cosf(bang);
        cylinder_hollow(stl,
            flange_x, flange_x + FLANGE_THICKNESS,
            bolt_hole_r, 0.0f,
            16, by, bz);
    }
    
    // ==================== ДАТЧИК ВЛАЖНОСТИ ====================
    float sensor_x = flange_x - 0.020f;
    float sensor_y = r_out + WALL_THICKNESS + 0.005f;
    // Корпус датчика
    cylinder_hollow(stl,
        sensor_x - 0.008f, sensor_x + 0.008f,
        0.006f, 0.004f,
        16, sensor_y, 0);
    // Штуцер в стенке
    cylinder_hollow(stl,
        sensor_x - 0.004f, sensor_x + 0.004f,
        0.003f, 0.0015f,
        12, sensor_y - 0.004f, 0);
    
    // ==================== МАРКИРОВОЧНАЯ ТАБЛИЧКА ====================
    float label_x = LENGTH * 0.3f;
    float label_y = r_in + WALL_THICKNESS + 0.005f;
    // Пластина
    fct(stl,
        {label_x - 0.015f, label_y, -0.010f},
        {label_x + 0.015f, label_y, -0.010f},
        {label_x + 0.015f, label_y + 0.020f, -0.010f});
    fct(stl,
        {label_x - 0.015f, label_y, -0.010f},
        {label_x + 0.015f, label_y + 0.020f, -0.010f},
        {label_x - 0.015f, label_y + 0.020f, -0.010f});
    
    std::cout << "  010_confusor.stl — готово\n";
    stl.close();
}