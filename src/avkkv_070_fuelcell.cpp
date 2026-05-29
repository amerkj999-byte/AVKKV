// ============================================================
// АВККВ-1.070.000 — ТОПЛИВНЫЙ ЭЛЕМЕНТ
// Сохранить как: avkkv_070_fuelcell.cpp
// ============================================================
// Назначение: дожигание H₂, генерация 10 кВт, нейтрализация O₃
// Тип: PEM (протонообменная мембрана)
// Катализатор: CeO₂ (оксид церия) на входе
// ============================================================
#include "avkkv_common.h"

void generate_070_fuelcell(const char* filename) {
    BinarySTL stl;
    stl.open(filename);
    
    // ==================== КОНСТАНТЫ ====================
    const float STACK_LENGTH = 0.300f;         // длина стека
    const float STACK_WIDTH = 0.200f;          // ширина
    const float STACK_HEIGHT = 0.250f;         // высота
    const float CELL_COUNT = 20;               // ячеек в стеке
    const float CELL_PITCH = 0.012f;           // шаг ячейки
    
    const float H2_INLET_DIAM = 0.015f;        // вход H₂ Ø15 мм
    const float AIR_INLET_DIAM = 0.025f;       // вход воздуха Ø25 мм
    const float WATER_OUTLET_DIAM = 0.012f;    // выход воды Ø12 мм
    const float O2_OUTLET_DIAM = 0.020f;       // выход O₂ Ø20 мм
    
    const float CATALYST_DIAM = 0.040f;        // Ø катализатора 40 мм
    const float CATALYST_LENGTH = 0.140f;      // длина 140 мм
    
    const float WALL_THICKNESS = 0.003f;       // 3 мм (AISI 316L)
    const float FLANGE_THICKNESS = 0.006f;
    const int SEGMENTS = 64;
    const int SMALL_SEG = 32;
    
    // Ось X — по длине стека, Y — вверх
    
    // ==================== КОРПУС СТЕКА ====================
    float stack_x = 0.0f;
    float stack_y = 0.0f;
    float stack_z = 0.0f;
    
    // Внешний корпус
    box(stl,
        {stack_x, stack_y - STACK_HEIGHT/2, stack_z - STACK_WIDTH/2},
        {stack_x + STACK_LENGTH, stack_y + STACK_HEIGHT/2, stack_z + STACK_WIDTH/2});
    
    // Внутренняя полость (упрощённо — двойные стенки)
    box(stl,
        {stack_x + WALL_THICKNESS, stack_y - STACK_HEIGHT/2 + WALL_THICKNESS, stack_z - STACK_WIDTH/2 + WALL_THICKNESS},
        {stack_x + STACK_LENGTH - WALL_THICKNESS, stack_y + STACK_HEIGHT/2 - WALL_THICKNESS, stack_z + STACK_WIDTH/2 - WALL_THICKNESS});
    
    // ==================== ЯЧЕЙКИ СТЕКА ====================
    // Биполярные пластины (горизонтальные)
    for(int c = 0; c < CELL_COUNT; c++) {
        float cx = stack_x + 0.030f + c * CELL_PITCH;
        
        // Биполярная пластина
        box(stl,
            {cx - 0.001f, stack_y - STACK_HEIGHT/2 + 0.015f, stack_z - STACK_WIDTH/2 + 0.015f},
            {cx + 0.001f, stack_y + STACK_HEIGHT/2 - 0.015f, stack_z + STACK_WIDTH/2 - 0.015f});
        
        // Каналы (рёбра на пластине)
        for(int ch = 0; ch < 15; ch++) {
            float chz = stack_z - STACK_WIDTH/2 + 0.025f + ch * 0.012f;
            box(stl,
                {cx - 0.0003f, stack_y - STACK_HEIGHT/2 + 0.020f, chz - 0.001f},
                {cx + 0.0003f, stack_y + STACK_HEIGHT/2 - 0.020f, chz + 0.001f});
        }
    }
    
    // Торцевые пластины
    float end_plate_x1 = stack_x + 0.015f;
    float end_plate_x2 = stack_x + STACK_LENGTH - 0.015f;
    
    box(stl,
        {end_plate_x1 - 0.005f, stack_y - STACK_HEIGHT/2 + 0.010f, stack_z - STACK_WIDTH/2 + 0.010f},
        {end_plate_x1 + 0.005f, stack_y + STACK_HEIGHT/2 - 0.010f, stack_z + STACK_WIDTH/2 - 0.010f});
    box(stl,
        {end_plate_x2 - 0.005f, stack_y - STACK_HEIGHT/2 + 0.010f, stack_z - STACK_WIDTH/2 + 0.010f},
        {end_plate_x2 + 0.005f, stack_y + STACK_HEIGHT/2 - 0.010f, stack_z + STACK_WIDTH/2 - 0.010f});
    
    // ==================== ВХОД ВОДОРОДА ====================
    float h2_inlet_x = stack_x;
    float h2_inlet_y = stack_y + STACK_HEIGHT/2 - 0.030f;
    float h2_inlet_z = stack_z - STACK_WIDTH/2 + 0.040f;
    float h2_r = H2_INLET_DIAM / 2.0f;
    
    cylinder_hollow(stl,
        h2_inlet_x - 0.020f, h2_inlet_x,
        h2_r + 0.002f, h2_r,
        SMALL_SEG, h2_inlet_y, h2_inlet_z);
    
    cylinder_hollow(stl,
        h2_inlet_x - 0.020f - FLANGE_THICKNESS, h2_inlet_x - 0.020f,
        h2_r + 0.008f, h2_r,
        SMALL_SEG, h2_inlet_y, h2_inlet_z);
    
    for(int b = 0; b < 3; b++) {
        float ang = b * 120.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            h2_inlet_x - 0.020f - FLANGE_THICKNESS, h2_inlet_x - 0.020f,
            0.0025f, 0.0f, 12,
            h2_inlet_y + (h2_r + 0.005f) * sinf(ang),
            h2_inlet_z + (h2_r + 0.005f) * cosf(ang));
    }
    
    // ==================== ВХОД ВОЗДУХА (ОЗОН + O₂) ====================
    float air_inlet_x = stack_x;
    float air_inlet_y = stack_y - STACK_HEIGHT/2 + 0.030f;
    float air_inlet_z = stack_z - STACK_WIDTH/2 + 0.040f;
    float air_r = AIR_INLET_DIAM / 2.0f;
    
    cylinder_hollow(stl,
        air_inlet_x - 0.025f, air_inlet_x,
        air_r + 0.003f, air_r,
        SEGMENTS, air_inlet_y, air_inlet_z);
    
    cylinder_hollow(stl,
        air_inlet_x - 0.025f - FLANGE_THICKNESS, air_inlet_x - 0.025f,
        air_r + 0.010f, air_r,
        SEGMENTS, air_inlet_y, air_inlet_z);
    
    for(int b = 0; b < 4; b++) {
        float ang = b * 90.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            air_inlet_x - 0.025f - FLANGE_THICKNESS, air_inlet_x - 0.025f,
            0.0035f, 0.0f, 16,
            air_inlet_y + (air_r + 0.007f) * sinf(ang),
            air_inlet_z + (air_r + 0.007f) * cosf(ang));
    }
    
    // ==================== КАТАЛИЗАТОР CeO₂ ====================
    float cat_x = air_inlet_x - 0.060f;
    float cat_y = air_inlet_y;
    float cat_z = air_inlet_z;
    float cat_r = CATALYST_DIAM / 2.0f;
    
    // Корпус катализатора
    cylinder_hollow(stl,
        cat_x - CATALYST_LENGTH/2, cat_x + CATALYST_LENGTH/2,
        cat_r + 0.004f, cat_r,
        SEGMENTS, cat_y, cat_z);
    
    // Сотовая структура внутри
    for(int row = -4; row <= 4; row++) {
        for(int col = -4; col <= 4; col++) {
            float cy = cat_y + row * 0.008f;
            float cz = cat_z + col * 0.008f;
            if(cy*cy + cz*cz <= (cat_r - 0.002f)*(cat_r - 0.002f)) {
                cylinder_hollow(stl,
                    cat_x - CATALYST_LENGTH/2 + 0.005f, cat_x + CATALYST_LENGTH/2 - 0.005f,
                    0.003f, 0.002f,
                    12, cy, cz);
            }
        }
    }
    
    // Фланцы катализатора
    cylinder_hollow(stl,
        cat_x - CATALYST_LENGTH/2 - FLANGE_THICKNESS, cat_x - CATALYST_LENGTH/2,
        cat_r + 0.010f, cat_r,
        SEGMENTS, cat_y, cat_z);
    cylinder_hollow(stl,
        cat_x + CATALYST_LENGTH/2, cat_x + CATALYST_LENGTH/2 + FLANGE_THICKNESS,
        cat_r + 0.010f, cat_r,
        SEGMENTS, cat_y, cat_z);
    
    // Труба от катализатора к стеку
    cable_segment(stl,
        {cat_x + CATALYST_LENGTH/2 + FLANGE_THICKNESS, cat_y, cat_z},
        {air_inlet_x - 0.025f - FLANGE_THICKNESS, air_inlet_y, air_inlet_z},
        air_r, SEGMENTS);
    
    // ==================== ВЫХОД ВОДЫ ====================
    float water_out_x = stack_x + STACK_LENGTH;
    float water_out_y = stack_y - STACK_HEIGHT/2 + 0.020f;
    float water_out_z = stack_z + STACK_WIDTH/2 - 0.060f;
    float water_r = WATER_OUTLET_DIAM / 2.0f;
    
    cylinder_hollow(stl,
        water_out_x, water_out_x + 0.025f,
        water_r + 0.002f, water_r,
        SMALL_SEG, water_out_y, water_out_z);
    
    cylinder_hollow(stl,
        water_out_x + 0.025f, water_out_x + 0.025f + FLANGE_THICKNESS,
        water_r + 0.008f, water_r,
        SMALL_SEG, water_out_y, water_out_z);
    
    // ==================== ВЫХОД КИСЛОРОДА ====================
    float o2_out_x = stack_x + STACK_LENGTH;
    float o2_out_y = stack_y + STACK_HEIGHT/2 - 0.030f;
    float o2_out_z = stack_z + STACK_WIDTH/2 - 0.040f;
    float o2_r = O2_OUTLET_DIAM / 2.0f;
    
    cylinder_hollow(stl,
        o2_out_x, o2_out_x + 0.030f,
        o2_r + 0.003f, o2_r,
        SEGMENTS, o2_out_y, o2_out_z);
    
    cylinder_hollow(stl,
        o2_out_x + 0.030f, o2_out_x + 0.030f + FLANGE_THICKNESS,
        o2_r + 0.010f, o2_r,
        SEGMENTS, o2_out_y, o2_out_z);
    
    for(int b = 0; b < 3; b++) {
        float ang = b * 120.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            o2_out_x + 0.030f, o2_out_x + 0.030f + FLANGE_THICKNESS,
            0.0035f, 0.0f, 16,
            o2_out_y + (o2_r + 0.007f) * sinf(ang),
            o2_out_z + (o2_r + 0.007f) * cosf(ang));
    }
    
    // ==================== КОНТУР ОХЛАЖДЕНИЯ ====================
    // Трубки охлаждения по бокам стека
    float cool_y = stack_y;
    float cool_z1 = stack_z - STACK_WIDTH/2 - 0.015f;
    float cool_z2 = stack_z + STACK_WIDTH/2 + 0.015f;
    
    for(int side = 0; side < 2; side++) {
        float cz = (side == 0) ? cool_z1 : cool_z2;
        
        // Трубка вдоль стека
        cylinder_hollow(stl,
            stack_x + 0.010f, stack_x + STACK_LENGTH - 0.010f,
            0.006f, 0.004f,
            24, cool_y, cz);
        
        // Штуцеры
        cylinder_hollow(stl,
            stack_x - 0.015f, stack_x + 0.010f,
            0.008f, 0.006f,
            24, cool_y, cz);
        cylinder_hollow(stl,
            stack_x + STACK_LENGTH - 0.010f, stack_x + STACK_LENGTH + 0.015f,
            0.008f, 0.006f,
            24, cool_y, cz);
    }
    
    // ==================== ЭЛЕКТРИЧЕСКИЕ ВЫВОДЫ ====================
    float terminal_y = stack_y + STACK_HEIGHT/2 + 0.015f;
    float terminal_z = stack_z;
    
    // Клеммная колодка
    box(stl,
        {stack_x + 0.050f, terminal_y, terminal_z - 0.025f},
        {stack_x + 0.100f, terminal_y + 0.025f, terminal_z + 0.025f});
    
    // Силовые кабели
    cable_segment(stl,
        {stack_x + 0.075f, terminal_y + 0.025f, terminal_z - 0.010f},
        {stack_x + 0.075f, terminal_y + 0.080f, terminal_z - 0.010f},
        0.004f, 12);
    cable_segment(stl,
        {stack_x + 0.075f, terminal_y + 0.025f, terminal_z + 0.010f},
        {stack_x + 0.075f, terminal_y + 0.080f, terminal_z + 0.010f},
        0.004f, 12);
    
    // ==================== КРЕПЁЖНЫЕ ЛАПЫ ====================
    for(int corner = 0; corner < 4; corner++) {
        float cx = stack_x + 0.040f + (corner / 2) * (STACK_LENGTH - 0.080f);
        float cz = stack_z + ((corner % 2) ? STACK_WIDTH/2 + 0.020f : -STACK_WIDTH/2 - 0.020f);
        
        box(stl,
            {cx - 0.015f, stack_y - STACK_HEIGHT/2 - 0.010f, cz - 0.015f},
            {cx + 0.015f, stack_y - STACK_HEIGHT/2, cz + 0.015f});
        
        cylinder_hollow(stl,
            stack_y - STACK_HEIGHT/2 - 0.012f, stack_y - STACK_HEIGHT/2 + 0.002f,
            0.005f, 0.0f, 12, cx, cz);
    }
    
    std::cout << "  070_fuelcell.stl — готово\n";
    stl.close();
}
