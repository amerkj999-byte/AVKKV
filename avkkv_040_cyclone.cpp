// ============================================================
// АВККВ-1.040.000 — ЦИКЛОН-СЕПАРАТОР
// Сохранить как: avkkv_040_cyclone.cpp
// ============================================================
// Назначение: отделение ледяной крошки от газового потока
// Вход: тангенциальный, газ + лёд от камеры 030
// Выход газа: осевой верхний Ø50 мм (→ детандер 050)
// Выход льда: нижний конус 15°, Ø60 мм (→ бункер 060)
// ============================================================
#include "avkkv_common.h"

void generate_040_cyclone(const char* filename) {
    BinarySTL stl;
    stl.open(filename);
    
    // ==================== КОНСТАНТЫ ====================
    const float CYLINDER_DIAM = 0.150f;       // Ø150 мм цилиндр
    const float CYLINDER_HEIGHT = 0.200f;     // высота цилиндра
    const float CONE_ANGLE_DEG = 15.0f;       // угол конуса 15°
    const float CONE_HEIGHT = 0.180f;         // высота конуса
    
    const float INLET_DIAM = 0.050f;          // вход Ø50 мм
    const float GAS_OUTLET_DIAM = 0.050f;     // выход газа Ø50 мм
    const float SNOW_OUTLET_DIAM = 0.060f;    // выход льда Ø60 мм
    
    const float WALL_THICKNESS = 0.004f;      // 4 мм (AISI 316L)
    const float FLANGE_THICKNESS = 0.008f;
    
    const float GAS_PIPE_LENGTH = 0.080f;     // длина выходной трубы
    const float GAS_PIPE_INSERT = 0.060f;     // погружение трубы в циклон
    
    const float ROTARY_VALVE_LENGTH = 0.060f; // длина шлюзового затвора
    const float ROTARY_VALVE_DIAM = 0.070f;   // диаметр затвора
    const int ROTARY_VANES = 4;               // 4 лопасти
    
    const int SEGMENTS = 72;
    const int SMALL_SEG = 32;
    
    const float r_cyl = CYLINDER_DIAM / 2.0f;  // 75 мм
    const float r_in = INLET_DIAM / 2.0f;      // 25 мм
    const float r_gas = GAS_OUTLET_DIAM / 2.0f;// 25 мм
    const float r_snow = SNOW_OUTLET_DIAM / 2.0f; // 30 мм
    
    // Центр циклона: ось X (вверх), Y=0, Z=0
    // Поток идёт снизу вверх? Нет. Газ входит тангенциально,
    // тяжёлые частицы падают вниз, газ уходит вверх.
    // Ось: Y — вверх, X — по горизонтали
    
    // ==================== ВХОДНОЙ ПАТРУБОК (ТАНГЕНЦИАЛЬНЫЙ) ====================
    float inlet_y = CYLINDER_HEIGHT * 0.7f;  // на 70% высоты цилиндра
    float inlet_x_offset = r_cyl + 0.010f;   // снаружи цилиндра
    
    // Труба подходит тангенциально
    float inlet_start_x = inlet_x_offset + 0.060f;
    float inlet_start_z = -r_cyl;
    
    cable_segment(stl,
        {inlet_start_x, inlet_y, inlet_start_z},
        {r_cyl + 0.002f, inlet_y, -r_cyl + 0.005f},
        r_in + 0.003f, SMALL_SEG);
    
    // Фланец входа
    cylinder_hollow(stl,
        inlet_start_x - FLANGE_THICKNESS, inlet_start_x,
        r_in + 0.010f, r_in,
        SEGMENTS, inlet_y, inlet_start_z);
    
    for(int b = 0; b < 4; b++) {
        float ang = b * 90.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            inlet_start_x - FLANGE_THICKNESS, inlet_start_x,
            0.0035f, 0.0f, 16,
            inlet_y + (r_in + 0.007f) * sinf(ang),
            inlet_start_z + (r_in + 0.007f) * cosf(ang));
    }
    
    // Отверстие в стенке цилиндра
    cylinder_hollow(stl,
        r_cyl - 0.005f, r_cyl + 0.005f,
        r_in + 0.001f, r_in,
        SEGMENTS, inlet_y, -r_cyl + 0.005f);
    
    // ==================== ЦИЛИНДРИЧЕСКАЯ ЧАСТЬ ====================
    float cyl_bottom = 0.0f;
    float cyl_top = CYLINDER_HEIGHT;
    
    // Стенка цилиндра
    cylinder_hollow(stl,
        cyl_bottom, cyl_top,
        r_cyl + WALL_THICKNESS, r_cyl,
        SEGMENTS, 0, 0);
    
    // Верхняя крышка
    float top_cover_y = cyl_top;
    cylinder_hollow(stl,
        top_cover_y, top_cover_y + WALL_THICKNESS,
        r_cyl + WALL_THICKNESS, 0.0f,
        SEGMENTS, 0, 0);
    
    // ==================== ЦЕНТРАЛЬНАЯ ТРУБА (ВЫХОД ГАЗА) ====================
    // Труба входит сверху и погружается внутрь циклона
    float gas_pipe_top = top_cover_y + WALL_THICKNESS;
    float gas_pipe_bottom = top_cover_y - GAS_PIPE_INSERT;
    
    // Внутренняя часть (в циклоне)
    cylinder_hollow(stl,
        gas_pipe_bottom, top_cover_y + 0.001f,
        r_gas + 0.002f, r_gas,
        SEGMENTS, 0, 0);
    
    // Внешняя часть (над циклоном)
    cylinder_hollow(stl,
        gas_pipe_top, gas_pipe_top + GAS_PIPE_LENGTH,
        r_gas + 0.003f, r_gas,
        SEGMENTS, 0, 0);
    
    // Фланец выхода газа
    float gas_flange_y = gas_pipe_top + GAS_PIPE_LENGTH;
    cylinder_hollow(stl,
        gas_flange_y, gas_flange_y + FLANGE_THICKNESS,
        r_gas + 0.012f, r_gas,
        SEGMENTS, 0, 0);
    
    for(int b = 0; b < 4; b++) {
        float ang = b * 90.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            gas_flange_y, gas_flange_y + FLANGE_THICKNESS,
            0.0045f, 0.0f, 16,
            (r_gas + 0.008f) * sinf(ang),
            (r_gas + 0.008f) * cosf(ang));
    }
    
    // ==================== КОНУСНАЯ ЧАСТЬ ====================
    float cone_top = cyl_bottom;
    float cone_bottom = cone_top - CONE_HEIGHT;
    float r_cone_bottom = r_snow;
    float r_cone_top = r_cyl;
    
    const int CONE_SEG = 40;
    float dy = CONE_HEIGHT / CONE_SEG;
    
    for(int i = 0; i < CONE_SEG; i++) {
        float y1 = cone_top - i * dy;
        float y2 = cone_top - (i + 1) * dy;
        float t1 = (float)i / CONE_SEG;
        float t2 = (float)(i + 1) / CONE_SEG;
        float r1 = r_cone_top + (r_cone_bottom - r_cone_top) * t1;
        float r2 = r_cone_top + (r_cone_bottom - r_cone_top) * t2;
        
        cylinder_hollow(stl, y2, y1,
            r1 + WALL_THICKNESS, r1,
            SEGMENTS, 0, 0);
    }
    
    // ==================== ШЛЮЗОВЫЙ ЗАТВОР ====================
    float rotary_center_y = cone_bottom - 0.040f;
    float rotary_x = 0.0f;  // ось X для ротора
    
    // Корпус затвора
    cylinder_hollow(stl,
        rotary_center_y - ROTARY_VALVE_LENGTH/2,
        rotary_center_y + ROTARY_VALVE_LENGTH/2,
        ROTARY_VALVE_DIAM/2 + 0.004f, ROTARY_VALVE_DIAM/2,
        SEGMENTS, rotary_x, 0);
    
    // Ротор (4 лопасти)
    float rotor_r = ROTARY_VALVE_DIAM/2 - 0.001f;
    float shaft_r = 0.008f;
    
    // Вал ротора
    cylinder_solid(stl,
        rotary_center_y - ROTARY_VALVE_LENGTH/2 - 0.020f,
        rotary_center_y + ROTARY_VALVE_LENGTH/2 + 0.020f,
        shaft_r, SMALL_SEG, rotary_x, 0);
    
    // Лопасти
    for(int v = 0; v < ROTARY_VANES; v++) {
        float ang = v * (360.0f / ROTARY_VANES) * M_PI / 180.0f;
        float blade_y = rotary_center_y;
        
        float bx1 = rotary_x + shaft_r * sinf(ang);
        float bz1 = shaft_r * cosf(ang);
        float bx2 = rotary_x + rotor_r * sinf(ang);
        float bz2 = rotor_r * cosf(ang);
        
        fct(stl,
            {blade_y - 0.025f, bx1, bz1},
            {blade_y + 0.025f, bx1, bz1},
            {blade_y + 0.025f, bx2, bz2});
        fct(stl,
            {blade_y - 0.025f, bx1, bz1},
            {blade_y + 0.025f, bx2, bz2},
            {blade_y - 0.025f, bx2, bz2});
    }
    
    // Шаговый двигатель
    float motor_y = rotary_center_y + ROTARY_VALVE_LENGTH/2 + 0.020f;
    cylinder_hollow(stl,
        motor_y, motor_y + 0.050f,
        0.030f, 0.025f,
        36, rotary_x, 0);
    
    // ==================== ПЕРЕХОД К ШЛЮЗУ ====================
    float transition_y = cone_bottom;
    float transition_len = 0.025f;
    
    cone_hollow(stl,
        transition_y - transition_len, transition_y,
        r_snow + WALL_THICKNESS, ROTARY_VALVE_DIAM/2 + 0.006f,
        WALL_THICKNESS, SEGMENTS, 0, 0);
    
    // ==================== ВЫХОД ЛЬДА (НИЖНИЙ ПАТРУБОК) ====================
    float outlet_y = rotary_center_y - ROTARY_VALVE_LENGTH/2;
    
    cylinder_hollow(stl,
        outlet_y - 0.030f, outlet_y,
        r_snow + WALL_THICKNESS, r_snow,
        SEGMENTS, 0, 0);
    
    // Фланец
    float snow_flange_y = outlet_y - 0.030f;
    cylinder_hollow(stl,
        snow_flange_y - FLANGE_THICKNESS, snow_flange_y,
        r_snow + 0.015f, r_snow,
        SEGMENTS, 0, 0);
    
    for(int b = 0; b < 6; b++) {
        float ang = b * 60.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            snow_flange_y - FLANGE_THICKNESS, snow_flange_y,
            0.0045f, 0.0f, 16,
            (r_snow + 0.010f) * sinf(ang),
            (r_snow + 0.010f) * cosf(ang));
    }
    
    // ==================== КРОНШТЕЙНЫ ====================
    for(int i = 0; i < 3; i++) {
        float ang = i * 120.0f * M_PI / 180.0f;
        float by = cyl_top * 0.3f;
        float bx = (r_cyl + WALL_THICKNESS + 0.015f) * sinf(ang);
        float bz = (r_cyl + WALL_THICKNESS + 0.015f) * cosf(ang);
        
        // Лапа
        fct(stl,
            {by - 0.004f, bx - 0.010f, bz - 0.010f},
            {by - 0.004f, bx + 0.010f, bz - 0.010f},
            {by - 0.004f, bx + 0.010f, bz + 0.010f});
        fct(stl,
            {by - 0.004f, bx - 0.010f, bz - 0.010f},
            {by - 0.004f, bx + 0.010f, bz + 0.010f},
            {by - 0.004f, bx - 0.010f, bz + 0.010f});
        
        // Ребро
        fct(stl,
            {by - 0.030f, bx, bz},
            {by - 0.004f, bx, bz - 0.008f},
            {by - 0.004f, bx, bz + 0.008f});
        
        // Отверстие
        cylinder_hollow(stl,
            by - 0.006f, by - 0.002f,
            0.005f, 0.0f, 12, bx, bz);
    }
    
    std::cout << "  040_cyclone.stl — готово\n";
    stl.close();
}