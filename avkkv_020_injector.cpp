// ============================================================
// АВККВ-1.020.000 — ИНЖЕКТОР
// Сохранить как: avkkv_020_injector.cpp
// ============================================================
// Назначение: впрыск жидкого водорода (LH₂) в поток воздуха
// Вход: Ø80 мм (стыковка с конфузором 010)
// Выход: Ø40 мм (стыковка с камерой смешения 030)
// ============================================================
#include "avkkv_common.h"

void generate_020_injector(const char* filename) {
    BinarySTL stl;
    stl.open(filename);
    
    // ==================== КОНСТАНТЫ ====================
    const float INLET_DIAM = 0.080f;         // вход Ø80 мм
    const float OUTLET_DIAM = 0.040f;        // выход Ø40 мм
    const float LENGTH = 0.200f;             // общая длина 200 мм
    const float WALL_THICKNESS = 0.004f;     // 4 мм (AISI 316L)
    
    const float LH2_NOZZLE_DIAM = 0.003f;    // сопло LH₂ Ø3 мм внутр.
    const float LH2_NOZZLE_OUTER = 0.008f;   // внешний диаметр трубки
    const float LH2_PIPE_DIAM = 0.006f;      // подводящая трубка Ø6 мм
    
    const float FLANGE_THICKNESS = 0.008f;   // фланцы 8 мм
    const int SEGMENTS = 64;
    const int SMALL_SEG = 24;
    
    const float r_in = INLET_DIAM / 2.0f;    // 40 мм
    const float r_out = OUTLET_DIAM / 2.0f;  // 20 мм
    
    // ==================== ВХОДНОЙ ФЛАНЕЦ ====================
    float flange_in_x = 0.0f;
    float flange_in_r = r_in + 0.015f;       // +15 мм
    
    // Диск фланца
    cylinder_hollow(stl,
        flange_in_x, flange_in_x + FLANGE_THICKNESS,
        flange_in_r, r_in,
        SEGMENTS, 0, 0);
    
    // Отверстия M8 (4 шт, Ø100 мм)
    float bolt_circle_in_r = 0.050f;         // Ø100 мм
    for(int b = 0; b < 4; b++) {
        float ang = b * 90.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            flange_in_x, flange_in_x + FLANGE_THICKNESS,
            0.0045f, 0.0f,
            16, bolt_circle_in_r * sinf(ang), bolt_circle_in_r * cosf(ang));
    }
    
    // ==================== КОРПУС ИНЖЕКТОРА ====================
    float body_start = FLANGE_THICKNESS;
    float body_end = LENGTH - FLANGE_THICKNESS;
    float body_len = body_end - body_start;
    
    // Внешний корпус (цилиндр → конус)
    const int BODY_SEG = 30;
    float dx = body_len / BODY_SEG;
    
    for(int i = 0; i < BODY_SEG; i++) {
        float x1 = body_start + i * dx;
        float x2 = body_start + (i + 1) * dx;
        float t = ((float)i + 0.5f) / BODY_SEG;
        
        // Плавное сужение (нелинейное — сопло Витошинского)
        float r = r_in + (r_out - r_in) * (t * t * (3 - 2 * t));
        float r_next = r_in + (r_out - r_in) * (((float)(i+1) + 0.5f) / BODY_SEG);
        float r_next_clamped = r_in + (r_out - r_in) * (((float)(i+1) + 0.5f) / BODY_SEG);
        r_next_clamped = r_in + (r_out - r_in) * (((float)(i+1) + 0.5f) / BODY_SEG);
        
        // Пересчитываем для границ сегмента
        float t1 = (float)i / BODY_SEG;
        float t2 = (float)(i + 1) / BODY_SEG;
        float r1 = r_in + (r_out - r_in) * (t1 * t1 * (3 - 2 * t1));
        float r2 = r_in + (r_out - r_in) * (t2 * t2 * (3 - 2 * t2));
        
        cylinder_hollow(stl, x1, x2,
            r1 + WALL_THICKNESS, r1,
            SEGMENTS, 0, 0);
    }
    
    // ==================== СОПЛО LH₂ ====================
    float nozzle_x = body_start + body_len * 0.4f;  // 40% от начала
    
    // Центральная трубка подачи LH₂
    // Заходит сверху, поворачивает в центр потока
    float pipe_entry_y = r_in + WALL_THICKNESS + 0.015f;
    float pipe_entry_x = nozzle_x - 0.030f;
    
    // Вертикальный подвод
    cylinder_hollow(stl,
        pipe_entry_x - 0.004f, pipe_entry_x + 0.004f,
        LH2_PIPE_DIAM/2 + 0.001f, LH2_PIPE_DIAM/2,
        SMALL_SEG, pipe_entry_y, 0);
    
    // Горизонтальный участок к оси
    cable_segment(stl,
        {pipe_entry_x, pipe_entry_y, 0},
        {nozzle_x, r_in * 0.3f, 0},
        LH2_PIPE_DIAM/2 + 0.001f, SMALL_SEG);
    
    // Трубка вдоль оси к соплу
    float nozzle_tube_start = nozzle_x - 0.020f;
    float nozzle_tube_end = nozzle_x + 0.025f;
    
    // Внешняя трубка
    cylinder_hollow(stl,
        nozzle_tube_start, nozzle_tube_end,
        LH2_NOZZLE_OUTER/2, LH2_NOZZLE_DIAM/2,
        SMALL_SEG, 0, 0);
    
    // Теплоизоляция (вакуумная рубашка)
    cylinder_hollow(stl,
        nozzle_tube_start - 0.002f, nozzle_tube_end + 0.002f,
        LH2_NOZZLE_OUTER/2 + 0.003f, LH2_NOZZLE_OUTER/2 + 0.0025f,
        SMALL_SEG, 0, 0);
    
    // Форсунка-распылитель на конце
    float spray_x = nozzle_tube_end;
    float spray_r = 0.005f;
    
    // Конус распылителя
    cone_hollow(stl,
        spray_x, spray_x + 0.008f,
        LH2_NOZZLE_DIAM/2, spray_r,
        0.001f, SMALL_SEG, 0, 0);
    
    // Отверстия распыла (6 шт под углом 30°)
    for(int h = 0; h < 6; h++) {
        float hang = h * 60.0f * M_PI / 180.0f;
        float spray_angle = 30.0f * M_PI / 180.0f;
        float hy = spray_r * 0.7f * sinf(hang);
        float hz = spray_r * 0.7f * cosf(hang);
        cylinder_hollow(stl,
            spray_x + 0.003f, spray_x + 0.008f,
            0.0005f, 0.0f,
            6, hy, hz);
    }
    
    // ==================== ЗАВИХРИТЕЛЬ ПОТОКА ====================
    float swirler_x = nozzle_x + 0.010f;
    float swirler_r = r_in + (r_out - r_in) * 0.5f;  // радиус на позиции
    
    for(int v = 0; v < 8; v++) {
        float ang = v * 45.0f * M_PI / 180.0f;
        float twist = 15.0f * M_PI / 180.0f;
        
        float vy = swirler_r * 0.35f * sinf(ang);
        float vz = swirler_r * 0.35f * cosf(ang);
        float vy_out = swirler_r * 0.7f * sinf(ang + twist);
        float vz_out = swirler_r * 0.7f * cosf(ang + twist);
        
        fct(stl,
            {swirler_x - 0.006f, vy, vz},
            {swirler_x + 0.006f, vy, vz},
            {swirler_x + 0.006f, vy_out, vz_out});
        fct(stl,
            {swirler_x - 0.006f, vy, vz},
            {swirler_x + 0.006f, vy_out, vz_out},
            {swirler_x - 0.006f, vy_out, vz_out});
    }
    
    // Кольцо завихрителя
    cylinder_hollow(stl,
        swirler_x - 0.008f, swirler_x + 0.008f,
        swirler_r * 0.75f, swirler_r * 0.7f,
        SEGMENTS, 0, 0);
    
    // ==================== ОБРАТНЫЙ КЛАПАН ====================
    float check_valve_x = nozzle_x - 0.040f;
    
    // Седло клапана
    cylinder_hollow(stl,
        check_valve_x - 0.005f, check_valve_x + 0.005f,
        0.012f, 0.008f,
        SMALL_SEG, 0, 0);
    
    // Шарик (подпружиненный)
    float ball_x = check_valve_x + 0.003f;
    float ball_r = 0.007f;
    int ball_seg = 16;
    
    for(int i = 0; i < ball_seg/2; i++) {
        float phi1 = i * M_PI / ball_seg;
        float phi2 = (i + 1) * M_PI / ball_seg;
        for(int j = 0; j < ball_seg; j++) {
            float th1 = j * 2 * M_PI / ball_seg;
            float th2 = (j + 1) * 2 * M_PI / ball_seg;
            
            P v1 = {ball_x + ball_r*cosf(phi1), ball_r*sinf(phi1)*cosf(th1), ball_r*sinf(phi1)*sinf(th1)};
            P v2 = {ball_x + ball_r*cosf(phi2), ball_r*sinf(phi2)*cosf(th1), ball_r*sinf(phi2)*sinf(th1)};
            P v3 = {ball_x + ball_r*cosf(phi2), ball_r*sinf(phi2)*cosf(th2), ball_r*sinf(phi2)*sinf(th2)};
            P v4 = {ball_x + ball_r*cosf(phi1), ball_r*sinf(phi1)*cosf(th2), ball_r*sinf(phi1)*sinf(th2)};
            
            fct(stl, v1, v2, v3);
            fct(stl, v1, v3, v4);
        }
    }
    
    // ==================== ВЫХОДНОЙ ФЛАНЕЦ ====================
    float flange_out_x = LENGTH - FLANGE_THICKNESS;
    float flange_out_r = r_out + 0.012f;
    
    cylinder_hollow(stl,
        flange_out_x, flange_out_x + FLANGE_THICKNESS,
        flange_out_r, r_out,
        SEGMENTS, 0, 0);
    
    // Отверстия M6 (4 шт, Ø60 мм)
    float bolt_circle_out_r = 0.030f;
    for(int b = 0; b < 4; b++) {
        float ang = b * 90.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            flange_out_x, flange_out_x + FLANGE_THICKNESS,
            0.0035f, 0.0f,
            16, bolt_circle_out_r * sinf(ang), bolt_circle_out_r * cosf(ang));
    }
    
    // ==================== ВНЕШНИЙ ПОДВОД LH₂ ====================
    // Трубка снаружи корпуса
    cable_segment(stl,
        {pipe_entry_x, pipe_entry_y, 0},
        {pipe_entry_x - 0.050f, pipe_entry_y + 0.060f, -0.030f},
        LH2_PIPE_DIAM/2 + 0.001f, SMALL_SEG);
    
    // Штуцер подключения
    float connector_x = pipe_entry_x - 0.050f;
    float connector_y = pipe_entry_y + 0.060f;
    float connector_z = -0.030f;
    cylinder_hollow(stl,
        connector_x - 0.008f, connector_x + 0.008f,
        0.010f, LH2_PIPE_DIAM/2,
        SMALL_SEG, connector_y, connector_z);
    
    std::cout << "  020_injector.stl — готово\n";
    stl.close();
}