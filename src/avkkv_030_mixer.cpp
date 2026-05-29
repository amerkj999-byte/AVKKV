// ============================================================
// АВККВ-1.030.000 — КАМЕРА СМЕШЕНИЯ-ДЕСУБЛИМАТОР
// Сохранить как: avkkv_030_mixer.cpp
// ============================================================
// Назначение: смешение воздуха с впрыснутым LH₂, выпадение инея/снега
// Вход: Ø40 мм (от инжектора 020)
// Выход газа: Ø50 мм (к циклону 040)
// Выход снега: Ø60 мм (шнек → циклон)
// ============================================================
#include "avkkv_common.h"

void generate_030_mixer(const char* filename) {
    BinarySTL stl;
    stl.open(filename);
    
    // ==================== КОНСТАНТЫ ====================
    const float INLET_DIAM = 0.040f;          // вход Ø40 мм
    const float OUTLET_GAS_DIAM = 0.050f;      // выход газа Ø50 мм
    const float OUTLET_SNOW_DIAM = 0.060f;     // выход снега Ø60 мм
    const float LENGTH = 0.800f;               // общая длина 800 мм
    const float CONE_ANGLE_DEG = 7.0f;         // полный угол 7°
    const float WALL_THICKNESS = 0.004f;       // 4 мм (AISI 316L)
    const float TEFLON_THICKNESS = 0.001f;     // 1 мм тефлоновое покрытие
    
    const float AUGER_DIAM = 0.020f;           // Ø вала шнека 20 мм
    const float AUGER_PITCH = 0.060f;          // шаг шнека 60 мм
    const float AUGER_BLADE_THICKNESS = 0.003f;// толщина лопасти 3 мм
    const float AUGER_LENGTH = 0.750f;         // длина шнека
    
    const float HEATER_TAPE_WIDTH = 0.010f;    // ширина нагревательной ленты
    const float HEATER_TAPE_THICKNESS = 0.002f;// толщина ленты
    
    const float FLANGE_THICKNESS = 0.008f;
    const int SEGMENTS = 72;
    const int AUGER_SEG = 48;
    
    const float r_in = INLET_DIAM / 2.0f;      // 20 мм
    const float tan_half = tanf(CONE_ANGLE_DEG / 2.0f * M_PI / 180.0f);
    
    // ==================== ВХОДНОЙ ФЛАНЕЦ ====================
    float flange_in_x = 0.0f;
    float flange_in_r = r_in + 0.012f;
    
    cylinder_hollow(stl,
        flange_in_x, flange_in_x + FLANGE_THICKNESS,
        flange_in_r, r_in, SEGMENTS, 0, 0);
    
    // Отверстия M6 (4 шт, Ø60 мм)
    for(int b = 0; b < 4; b++) {
        float ang = b * 90.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            flange_in_x, flange_in_x + FLANGE_THICKNESS,
            0.0035f, 0.0f, 16,
            0.030f * sinf(ang), 0.030f * cosf(ang));
    }
    
    // ==================== КОНУС КАМЕРЫ ====================
    float cone_start = FLANGE_THICKNESS;
    float cone_end = LENGTH - FLANGE_THICKNESS;
    float cone_len = cone_end - cone_start;
    
    // Вычисляем радиусы на концах
    float r_start = r_in;
    float r_end = r_start + cone_len * tan_half;  // расширение 7°
    
    // Разбиваем на 80 сегментов
    const int CONE_SEG = 80;
    float dx = cone_len / CONE_SEG;
    
    for(int i = 0; i < CONE_SEG; i++) {
        float x1 = cone_start + i * dx;
        float x2 = cone_start + (i + 1) * dx;
        float t1 = (float)i / CONE_SEG;
        float t2 = (float)(i + 1) / CONE_SEG;
        float r1 = r_start + (r_end - r_start) * t1;
        float r2 = r_start + (r_end - r_start) * t2;
        
        // Внешняя стенка + тефлон
        cylinder_hollow(stl, x1, x2,
            r1 + WALL_THICKNESS, r1 - TEFLON_THICKNESS,
            SEGMENTS, 0, 0);
    }
    
    // ==================== НАГРЕВАТЕЛЬНАЯ ЛЕНТА ====================
    // Обогрев стенок до +5°C, 5 витков по длине
    float heater_r = r_start + WALL_THICKNESS + 0.001f;
    for(int h = 0; h < 5; h++) {
        float hx = cone_start + cone_len * (h + 0.5f) / 5.0f;
        float hr = r_start + (r_end - r_start) * (h + 0.5f) / 5.0f + WALL_THICKNESS + 0.001f;
        
        // Виток ленты
        for(float a = 0; a < 360; a += 15) {
            float a1 = a * M_PI / 180.0f;
            float a2 = (a + 15) * M_PI / 180.0f;
            
            fct(stl,
                {hx - HEATER_TAPE_WIDTH/2, hr*sinf(a1), hr*cosf(a1)},
                {hx - HEATER_TAPE_WIDTH/2, hr*sinf(a2), hr*cosf(a2)},
                {hx + HEATER_TAPE_WIDTH/2, hr*sinf(a2), hr*cosf(a2)});
            fct(stl,
                {hx - HEATER_TAPE_WIDTH/2, hr*sinf(a1), hr*cosf(a1)},
                {hx + HEATER_TAPE_WIDTH/2, hr*sinf(a2), hr*cosf(a2)},
                {hx + HEATER_TAPE_WIDTH/2, hr*sinf(a1), hr*cosf(a1)});
        }
    }
    
    // ==================== ШНЕК ====================
    float auger_start = cone_start + 0.040f;
    float auger_end = auger_start + AUGER_LENGTH;
    float auger_r = AUGER_DIAM / 2.0f;
    
    // Вал шнека
    cylinder_solid(stl, auger_start, auger_end, auger_r, AUGER_SEG, 0, 0);
    
    // Лопасть шнека (винтовая)
    int turns = (int)(AUGER_LENGTH / AUGER_PITCH);
    int points_per_turn = 72;
    float dphi = 2.0f * M_PI / points_per_turn;
    float dx_per_point = AUGER_PITCH / points_per_turn;
    
    for(int t = 0; t < turns; t++) {
        for(int p = 0; p < points_per_turn; p++) {
            float phi1 = p * dphi;
            float phi2 = (p + 1) * dphi;
            float x1 = auger_start + t * AUGER_PITCH + p * dx_per_point;
            float x2 = auger_start + t * AUGER_PITCH + (p + 1) * dx_per_point;
            
            // Радиус корпуса в этой точке
            float t_pos = (x1 - cone_start) / cone_len;
            if(t_pos < 0) t_pos = 0;
            if(t_pos > 1) t_pos = 1;
            float local_r = r_start + (r_end - r_start) * t_pos - TEFLON_THICKNESS - 0.001f;
            
            // Внутренняя кромка (на валу)
            float inner_r = auger_r;
            // Внешняя кромка (у стенки)
            float outer_r = local_r * 0.95f;
            
            P inner1 = {x1, inner_r*sinf(phi1), inner_r*cosf(phi1)};
            P inner2 = {x2, inner_r*sinf(phi2), inner_r*cosf(phi2)};
            P outer1 = {x1, outer_r*sinf(phi1), outer_r*cosf(phi1)};
            P outer2 = {x2, outer_r*sinf(phi2), outer_r*cosf(phi2)};
            
            // Толщина лопасти
            P inner1b = {x1 + AUGER_BLADE_THICKNESS, inner_r*sinf(phi1), inner_r*cosf(phi1)};
            P inner2b = {x2 + AUGER_BLADE_THICKNESS, inner_r*sinf(phi2), inner_r*cosf(phi2)};
            P outer1b = {x1 + AUGER_BLADE_THICKNESS, outer_r*sinf(phi1), outer_r*cosf(phi1)};
            P outer2b = {x2 + AUGER_BLADE_THICKNESS, outer_r*sinf(phi2), outer_r*cosf(phi2)};
            
            // Передняя грань
            fct(stl, inner1, outer1, outer2);
            fct(stl, inner1, outer2, inner2);
            // Задняя грань
            fct(stl, inner1b, outer2b, outer1b);
            fct(stl, inner1b, inner2b, outer2b);
            // Торец у вала
            fct(stl, inner1, inner2, inner2b);
            fct(stl, inner1, inner2b, inner1b);
            // Торец у стенки
            fct(stl, outer1, outer1b, outer2b);
            fct(stl, outer1, outer2b, outer2);
        }
    }
    
    // ==================== ПРИВОД ШНЕКА ====================
    float motor_x = auger_start - 0.060f;
    float motor_y = 0.0f;
    float motor_r = 0.040f;
    
    // Корпус мотора
    cylinder_hollow(stl,
        motor_x - 0.050f, motor_x + 0.050f,
        motor_r, motor_r - 0.006f,
        36, motor_y, 0);
    
    // Магнитная муфта
    float coupling_x = auger_start - 0.010f;
    cylinder_hollow(stl,
        coupling_x - 0.015f, coupling_x + 0.015f,
        0.025f, auger_r,
        36, 0, 0);
    
    // Вал мотора к муфте
    cylinder_solid(stl,
        motor_x + 0.040f, coupling_x - 0.010f,
        0.006f, 16, 0, 0);
    
    // ==================== ВЫХОД ГАЗА (ВЕРХНИЙ) ====================
    float gas_outlet_x = cone_end - 0.050f;
    float gas_outlet_r = OUTLET_GAS_DIAM / 2.0f;
    float gas_outlet_y = r_end * 0.8f;  // в верхней точке
    
    // Патрубок выхода газа
    cylinder_hollow(stl,
        gas_outlet_x - 0.010f, gas_outlet_x + 0.010f,
        gas_outlet_r + 0.003f, gas_outlet_r,
        SEGMENTS, gas_outlet_y, 0);
    
    // Отверстие в стенке
    cylinder_hollow(stl,
        gas_outlet_x - 0.005f, gas_outlet_x + 0.005f,
        gas_outlet_r + 0.001f, gas_outlet_r,
        SEGMENTS, gas_outlet_y - (gas_outlet_y - r_end)/2, 0);
    
    // Фланец газового выхода
    float gas_flange_x = gas_outlet_x + 0.010f;
    cylinder_hollow(stl,
        gas_flange_x, gas_flange_x + FLANGE_THICKNESS,
        gas_outlet_r + 0.010f, gas_outlet_r,
        SEGMENTS, gas_outlet_y, 0);
    
    for(int b = 0; b < 4; b++) {
        float ang = b * 90.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            gas_flange_x, gas_flange_x + FLANGE_THICKNESS,
            0.0035f, 0.0f, 16,
            gas_outlet_y + (gas_outlet_r + 0.007f) * sinf(ang),
            (gas_outlet_r + 0.007f) * cosf(ang));
    }
    
    // ==================== ВЫХОД СНЕГА (НИЖНИЙ) ====================
    float snow_outlet_x = cone_end;
    float snow_outlet_r = OUTLET_SNOW_DIAM / 2.0f;
    
    // Переходник
    float transition_len = 0.030f;
    cone_hollow(stl,
        snow_outlet_x, snow_outlet_x + transition_len,
        r_end, snow_outlet_r,
        WALL_THICKNESS, SEGMENTS, 0, 0);
    
    // Патрубок
    float snow_pipe_x = snow_outlet_x + transition_len;
    cylinder_hollow(stl,
        snow_pipe_x, snow_pipe_x + 0.040f,
        snow_outlet_r + WALL_THICKNESS, snow_outlet_r,
        SEGMENTS, 0, 0);
    
    // Фланец
    float snow_flange_x = snow_pipe_x + 0.040f;
    cylinder_hollow(stl,
        snow_flange_x, snow_flange_x + FLANGE_THICKNESS,
        snow_outlet_r + 0.015f, snow_outlet_r,
        SEGMENTS, 0, 0);
    
    for(int b = 0; b < 6; b++) {
        float ang = b * 60.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            snow_flange_x, snow_flange_x + FLANGE_THICKNESS,
            0.0045f, 0.0f, 16,
            (snow_outlet_r + 0.010f) * sinf(ang),
            (snow_outlet_r + 0.010f) * cosf(ang));
    }
    
    // ==================== ТЕРМОПАРА ====================
    float thermo_x = cone_start + cone_len * 0.3f;
    float thermo_y = r_start + (r_end - r_start) * 0.3f + WALL_THICKNESS + 0.005f;
    
    cylinder_hollow(stl,
        thermo_x - 0.005f, thermo_x + 0.005f,
        0.004f, 0.002f,
        16, thermo_y, 0);
    
    std::cout << "  030_mixer.stl — готово\n";
    stl.close();
}
