// ============================================================
// АВККВ-1.060.000 — БУНКЕР-РЕКУПЕРАТОР
// Сохранить как: avkkv_060_bunker.cpp
// ============================================================
// Назначение: приём ледяной крошки, таяние теплом сжатого газа, сбор воды
// Объём: 50 литров (рабочий)
// Змеевик: трубка 12×1 мм, AISI 304
// ============================================================
#include "avkkv_common.h"

void generate_060_bunker(const char* filename) {
    BinarySTL stl;
    stl.open(filename);
    
    // ==================== КОНСТАНТЫ ====================
    const float BUNKER_DIAM = 0.350f;          // внутренний Ø350 мм
    const float BUNKER_HEIGHT = 0.550f;        // высота 550 мм (≈53 л)
    const float WALL_THICKNESS = 0.003f;       // 3 мм (AISI 304)
    
    const float COIL_TUBE_OD = 0.012f;         // Ø трубки 12 мм
    const float COIL_TUBE_ID = 0.010f;         // внутр. Ø10 мм
    const float COIL_DIAM = 0.250f;            // Ø змеевика 250 мм
    const float COIL_TURNS = 8;                // витков
    const float COIL_PITCH = 0.035f;           // шаг витка 35 мм
    
    const float AUGER_DIAM = 0.025f;           // Ø вала мешалки
    const float AUGER_BLADE_WIDTH = 0.040f;    // ширина лопасти
    const float AUGER_BLADE_THICKNESS = 0.003f;
    
    const float INLET_SNOW_DIAM = 0.060f;      // вход льда Ø60 мм
    const float OUTLET_WATER_DIAM = 0.020f;    // слив воды Ø20 мм
    const float GAS_INLET_DIAM = 0.030f;       // вход газа Ø30 мм
    const float GAS_OUTLET_DIAM = 0.020f;      // выход газа Ø20 мм
    
    const float FLANGE_THICKNESS = 0.008f;
    const int SEGMENTS = 72;
    const int COIL_SEG = 48;
    
    const float r_bunker = BUNKER_DIAM / 2.0f;
    
    // Ось Y — вверх, днище на Y=0
    
    // ==================== КОРПУС БУНКЕРА ====================
    float bunker_bottom = 0.0f;
    float bunker_top = BUNKER_HEIGHT;
    
    // Цилиндрическая стенка
    cylinder_hollow(stl,
        bunker_bottom, bunker_top,
        r_bunker + WALL_THICKNESS, r_bunker,
        SEGMENTS, 0, 0);
    
    // Днище (полусферическое)
    int dome_seg = 30;
    for(int i = 0; i < dome_seg; i++) {
        float phi1 = i * M_PI/2 / dome_seg;
        float phi2 = (i + 1) * M_PI/2 / dome_seg;
        float y1 = bunker_bottom - r_bunker + r_bunker * sinf(phi1);
        float y2 = bunker_bottom - r_bunker + r_bunker * sinf(phi2);
        float r1 = r_bunker * cosf(phi1);
        float r2 = r_bunker * cosf(phi2);
        
        if(y1 < 0) y1 = 0;
        if(y2 < 0) y2 = 0;
        if(r1 > r_bunker) r1 = r_bunker;
        if(r2 > r_bunker) r2 = r_bunker;
        
        if(y1 >= 0 && y2 >= 0) {
            cylinder_hollow(stl, y1, y2,
                r1 + WALL_THICKNESS, r1,
                SEGMENTS, 0, 0);
        }
    }
    
    // Плоское днище (если полусфера не доходит)
    cylinder_hollow(stl,
        bunker_bottom - 0.002f, bunker_bottom,
        r_bunker + WALL_THICKNESS, 0.0f,
        SEGMENTS, 0, 0);
    
    // Верхняя крышка
    cylinder_hollow(stl,
        bunker_top, bunker_top + WALL_THICKNESS,
        r_bunker + WALL_THICKNESS, 0.0f,
        SEGMENTS, 0, 0);
    
    // ==================== ЗМЕЕВИК ====================
    float coil_bottom = bunker_bottom + 0.050f;
    float coil_r = COIL_DIAM / 2.0f;
    float tube_r = COIL_TUBE_OD / 2.0f;
    float tube_inner_r = COIL_TUBE_ID / 2.0f;
    
    // Витки змеевика
    for(int t = 0; t < COIL_TURNS; t++) {
        float y_center = coil_bottom + t * COIL_PITCH;
        
        for(int i = 0; i < COIL_SEG; i++) {
            float a1 = i * 2.0f * M_PI / COIL_SEG;
            float a2 = (i + 1) * 2.0f * M_PI / COIL_SEG;
            
            float y1 = y_center;
            float y2 = y_center;
            
            P c1 = {y1, coil_r * sinf(a1), coil_r * cosf(a1)};
            P c2 = {y2, coil_r * sinf(a2), coil_r * cosf(a2)};
            
            // Сегмент трубки
            cable_segment(stl, c1, c2, tube_r, 12);
        }
    }
    
    // Подвод газа (вход змеевика) — сверху, снаружи
    float gas_inlet_y = coil_bottom + (COIL_TURNS - 1) * COIL_PITCH;
    float gas_inlet_z = -(r_bunker + WALL_THICKNESS + 0.020f);
    
    cable_segment(stl,
        {gas_inlet_y, coil_r, 0},
        {gas_inlet_y, r_bunker + WALL_THICKNESS + 0.015f, gas_inlet_z},
        tube_r, 16);
    
    // Входной штуцер
    cylinder_hollow(stl,
        gas_inlet_y - 0.010f, gas_inlet_y + 0.010f,
        tube_r + 0.003f, tube_r,
        24, r_bunker + WALL_THICKNESS + 0.015f, gas_inlet_z);
    
    // Отвод газа (выход змеевика) — снизу
    float gas_outlet_y = coil_bottom;
    
    cable_segment(stl,
        {gas_outlet_y, coil_r, 0},
        {gas_outlet_y, r_bunker + WALL_THICKNESS + 0.015f, -gas_inlet_z},
        tube_r, 16);
    
    // Выходной штуцер
    cylinder_hollow(stl,
        gas_outlet_y - 0.010f, gas_outlet_y + 0.010f,
        tube_r + 0.003f, tube_r,
        24, r_bunker + WALL_THICKNESS + 0.015f, -gas_inlet_z);
    
    // ==================== ШНЕК-МЕШАЛКА ====================
    float auger_bottom = bunker_bottom + 0.020f;
    float auger_top = coil_bottom + (COIL_TURNS - 1) * COIL_PITCH + 0.050f;
    float auger_shaft_r = AUGER_DIAM / 2.0f;
    
    // Вал мешалки
    cylinder_solid(stl,
        auger_bottom, auger_top,
        auger_shaft_r, SEGMENTS, 0, 0);
    
    // Лопасти (крестовина, 4 уровня)
    for(int level = 0; level < 4; level++) {
        float ly = auger_bottom + (auger_top - auger_bottom) * (level + 0.5f) / 4.0f;
        float lr = r_bunker * 0.85f;
        
        for(int arm = 0; arm < 2; arm++) {
            float ang = arm * M_PI;
            float dir_y = sinf(ang);
            float dir_z = cosf(ang);
            
            fct(stl,
                {ly - AUGER_BLADE_THICKNESS/2, auger_shaft_r * dir_y, auger_shaft_r * dir_z},
                {ly + AUGER_BLADE_THICKNESS/2, auger_shaft_r * dir_y, auger_shaft_r * dir_z},
                {ly + AUGER_BLADE_THICKNESS/2, lr * dir_y, lr * dir_z});
            fct(stl,
                {ly - AUGER_BLADE_THICKNESS/2, auger_shaft_r * dir_y, auger_shaft_r * dir_z},
                {ly + AUGER_BLADE_THICKNESS/2, lr * dir_y, lr * dir_z},
                {ly - AUGER_BLADE_THICKNESS/2, lr * dir_y, lr * dir_z});
        }
    }
    
    // Привод мешалки (мотор-редуктор сверху)
    float motor_y = bunker_top + 0.040f;
    cylinder_hollow(stl,
        motor_y, motor_y + 0.060f,
        0.040f, 0.035f,
        36, 0, 0);
    
    // Вал мотора
    cylinder_solid(stl,
        bunker_top + 0.005f, motor_y,
        auger_shaft_r, 24, 0, 0);
    
    // Уплотнение вала
    cylinder_hollow(stl,
        bunker_top + 0.002f, bunker_top + 0.010f,
        auger_shaft_r + 0.005f, auger_shaft_r + 0.002f,
        36, 0, 0);
    
    // ==================== ВХОД ЛЬДА (СВЕРХУ) ====================
    float snow_inlet_y = bunker_top + WALL_THICKNESS;
    float snow_inlet_r = INLET_SNOW_DIAM / 2.0f;
    float snow_offset_z = r_bunker * 0.3f;  // смещён от центра
    
    cylinder_hollow(stl,
        snow_inlet_y, snow_inlet_y + 0.040f,
        snow_inlet_r + WALL_THICKNESS, snow_inlet_r,
        SEGMENTS, 0, snow_offset_z);
    
    // Фланец
    cylinder_hollow(stl,
        snow_inlet_y, snow_inlet_y + FLANGE_THICKNESS,
        snow_inlet_r + 0.015f, snow_inlet_r,
        SEGMENTS, 0, snow_offset_z);
    
    for(int b = 0; b < 6; b++) {
        float ang = b * 60.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            snow_inlet_y, snow_inlet_y + FLANGE_THICKNESS,
            0.0045f, 0.0f, 16,
            (snow_inlet_r + 0.010f) * sinf(ang),
            snow_offset_z + (snow_inlet_r + 0.010f) * cosf(ang));
    }
    
    // Отверстие в крышке
    cylinder_hollow(stl,
        bunker_top, bunker_top + WALL_THICKNESS + 0.001f,
        snow_inlet_r + 0.001f, snow_inlet_r,
        SEGMENTS, 0, snow_offset_z);
    
    // Решётка под входом (чтобы лёд падал на змеевик)
    float grate_y = bunker_top - 0.030f;
    for(int bar = -3; bar <= 3; bar++) {
        float bz = snow_offset_z + bar * 0.015f;
        cable_segment(stl,
            {grate_y, -snow_inlet_r, bz},
            {grate_y, snow_inlet_r, bz},
            0.002f, 6);
    }
    
    // ==================== СЛИВ ВОДЫ ====================
    float drain_y = bunker_bottom + 0.005f;
    float drain_r = OUTLET_WATER_DIAM / 2.0f;
    
    // Патрубок слива
    cylinder_hollow(stl,
        drain_y, drain_y + 0.030f,
        drain_r + 0.003f, drain_r,
        24, 0, 0);
    
    // Кран шаровой
    float valve_y = drain_y + 0.030f;
    cylinder_hollow(stl,
        valve_y, valve_y + 0.040f,
        0.025f, 0.020f,
        36, 0, 0);
    
    // Ручка крана
    cylinder_solid(stl,
        valve_y + 0.020f, valve_y + 0.020f + 0.002f,
        0.008f, 16, 0.030f, 0);
    cable_segment(stl,
        {valve_y + 0.020f, 0.030f, 0},
        {valve_y + 0.020f, 0.060f, 0},
        0.004f, 8);
    
    // Фильтр грубой очистки
    float filter_y = valve_y + 0.040f;
    cylinder_hollow(stl,
        filter_y, filter_y + 0.030f,
        0.022f, 0.018f,
        36, 0, 0);
    
    // Сетка фильтра
    cylinder_hollow(stl,
        filter_y + 0.015f, filter_y + 0.017f,
        0.018f, 0.0f,
        36, 0, 0);
    
    // ==================== ДАТЧИК УРОВНЯ ====================
    float sensor_y = bunker_bottom + 0.050f;
    float sensor_z = r_bunker * 0.6f;
    
    // Поплавковая трубка
    cylinder_hollow(stl,
        bunker_bottom + 0.010f, bunker_top,
        0.008f, 0.006f,
        20, sensor_z, 0);
    
    // Поплавок
    cylinder_hollow(stl,
        sensor_y - 0.015f, sensor_y + 0.015f,
        0.018f, 0.016f,
        24, sensor_z, 0);
    
    // ==================== ВЫХЛОПНОЙ ПАТРУБОК ====================
    float vent_y = bunker_top + WALL_THICKNESS;
    float vent_r = 0.015f;
    float vent_offset_z = -r_bunker * 0.4f;
    
    cylinder_hollow(stl,
        vent_y, vent_y + 0.050f,
        vent_r + 0.002f, vent_r,
        24, 0, vent_offset_z);
    
    // Активированный уголь (фильтр)
    float carbon_y = vent_y + 0.050f;
    cylinder_hollow(stl,
        carbon_y, carbon_y + 0.040f,
        0.025f, 0.022f,
        36, 0, vent_offset_z);
    
    // Крышка фильтра
    cylinder_hollow(stl,
        carbon_y + 0.040f, carbon_y + 0.043f,
        0.025f, vent_r,
        36, 0, vent_offset_z);
    
    // ==================== ОПОРЫ ====================
    for(int i = 0; i < 3; i++) {
        float ang = i * 120.0f * M_PI / 180.0f;
        float by = bunker_bottom - r_bunker * 0.5f;
        float br = r_bunker + WALL_THICKNESS + 0.010f;
        float oy = br * sinf(ang);
        float oz = br * cosf(ang);
        
        // Ножка
        cylinder_solid(stl,
            by, by + 0.100f,
            0.010f, 16, oy, oz);
        
        // Лапа
        fct(stl,
            {by, oy - 0.015f, oz - 0.015f},
            {by, oy + 0.015f, oz - 0.015f},
            {by, oy + 0.015f, oz + 0.015f});
        fct(stl,
            {by, oy - 0.015f, oz - 0.015f},
            {by, oy + 0.015f, oz + 0.015f},
            {by, oy - 0.015f, oz + 0.015f});
        
        // Отверстие в лапе
        cylinder_hollow(stl,
            by - 0.004f, by + 0.004f,
            0.006f, 0.0f, 12, oy, oz);
    }
    
    std::cout << "  060_bunker.stl — готово\n";
    stl.close();
}