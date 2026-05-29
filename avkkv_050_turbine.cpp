// ============================================================
// АВККВ-1.050.000 — ТУРБОДЕТАНДЕР + КОМПРЕССОР
// Сохранить как: avkkv_050_turbine.cpp
// ============================================================
// Назначение: расширение газа с отбором работы, сжатие с нагревом
// Детандер: одноступенчатая радиально-осевая турбина, π=6.5
// Компрессор: 5 центробежных колёс, π=40
// Вал общий: титан ВТ6, подшипники лепестковые газодинамические
// ============================================================
#include "avkkv_common.h"

void generate_050_turbine(const char* filename) {
    BinarySTL stl;
    stl.open(filename);
    
    // ==================== КОНСТАНТЫ ====================
    const float SHAFT_DIAM = 0.020f;           // Ø вала 20 мм
    const float SHAFT_LENGTH = 0.500f;         // общая длина вала
    
    // Детандер (холодная сторона, слева)
    const float EXP_INLET_DIAM = 0.050f;       // вход Ø50 мм
    const float EXP_WHEEL_DIAM = 0.080f;       // Ø колеса 80 мм
    const float EXP_WHEEL_WIDTH = 0.025f;      // ширина колеса
    const float EXP_BLADES = 11;               // лопаток
    const float EXP_HUB_RATIO = 0.25f;         // втулочное отношение
    
    // Компрессор (тёплая сторона, справа)
    const float COMP_STAGES = 5;               // 5 ступеней
    const float COMP_WHEEL_DIAM_1 = 0.060f;    // Ø 1-й ступени 60 мм
    const float COMP_WHEEL_DIAM_LAST = 0.045f; // Ø последней 45 мм
    const float COMP_WHEEL_WIDTH = 0.018f;     // ширина колеса
    const float COMP_BLADES = 14;              // лопаток (12 основных + 2 сплиттера)
    const float COMP_STAGE_SPACING = 0.040f;   // расстояние между ступенями
    
    // Электродвигатель
    const float MOTOR_DIAM = 0.100f;           // Ø100 мм
    const float MOTOR_LENGTH = 0.120f;         // длина 120 мм
    const float MOTOR_AIRGAP = 0.001f;         // зазор 1 мм
    
    // Подшипники
    const float BEARING_DIAM = 0.035f;         // Ø35 мм
    const float BEARING_WIDTH = 0.020f;        // ширина 20 мм
    
    const float WALL_THICKNESS = 0.005f;
    const float FLANGE_THICKNESS = 0.008f;
    const int SEGMENTS = 64;
    const int BLADE_SEG = 36;
    
    const float r_shaft = SHAFT_DIAM / 2.0f;
    
    // ==================== КОМПОНОВКА ПО X ====================
    // X=0: вход детандера
    // Положительный X: вправо (к компрессору)
    
    float exp_inlet_x = 0.0f;
    float exp_wheel_x = 0.060f;
    float shaft_start = 0.090f;
    float brg1_x = 0.120f;
    float motor_start = 0.180f;
    float motor_end = motor_start + MOTOR_LENGTH;
    float brg2_x = motor_end + 0.030f;
    float comp_start = brg2_x + 0.040f;
    
    // ==================== ВХОД ДЕТАНДЕРА ====================
    float r_exp_in = EXP_INLET_DIAM / 2.0f;
    
    // Входной патрубок
    cylinder_hollow(stl,
        exp_inlet_x, exp_inlet_x + 0.040f,
        r_exp_in + WALL_THICKNESS, r_exp_in,
        SEGMENTS, 0, 0);
    
    // Фланец входа
    cylinder_hollow(stl,
        exp_inlet_x, exp_inlet_x + FLANGE_THICKNESS,
        r_exp_in + 0.012f, r_exp_in,
        SEGMENTS, 0, 0);
    
    for(int b = 0; b < 4; b++) {
        float ang = b * 90.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            exp_inlet_x, exp_inlet_x + FLANGE_THICKNESS,
            0.0045f, 0.0f, 16,
            (r_exp_in + 0.008f) * sinf(ang),
            (r_exp_in + 0.008f) * cosf(ang));
    }
    
    // ==================== КОЛЕСО ДЕТАНДЕРА ====================
    float exp_r_hub = EXP_WHEEL_DIAM / 2.0f * EXP_HUB_RATIO;
    float exp_r_tip = EXP_WHEEL_DIAM / 2.0f;
    
    // Ступица
    cylinder_solid(stl,
        exp_wheel_x - 0.005f, exp_wheel_x + EXP_WHEEL_WIDTH + 0.005f,
        exp_r_hub, SEGMENTS, 0, 0);
    
    // Лопатки (радиально-осевые, 11 шт)
    for(int b = 0; b < EXP_BLADES; b++) {
        float ang = b * (360.0f / EXP_BLADES) * M_PI / 180.0f;
        
        // Корень лопатки (на ступице, входит радиально)
        float root_x1 = exp_wheel_x;
        float root_r1 = exp_r_hub;
        float root_y1 = root_r1 * sinf(ang);
        float root_z1 = root_r1 * cosf(ang);
        
        // Середина (выходит радиально и загибается)
        float mid_x = exp_wheel_x + EXP_WHEEL_WIDTH * 0.5f;
        float mid_r = exp_r_hub + (exp_r_tip - exp_r_hub) * 0.7f;
        float mid_y = mid_r * sinf(ang - 0.15f);  // закрутка
        float mid_z = mid_r * cosf(ang - 0.15f);
        
        // Кончик (осевой выход)
        float tip_x = exp_wheel_x + EXP_WHEEL_WIDTH;
        float tip_r = exp_r_tip * 0.95f;
        float tip_y = tip_r * sinf(ang - 0.30f);
        float tip_z = tip_r * cosf(ang - 0.30f);
        
        // Строим лопатку как треугольники между точками
        for(int s = 0; s < 10; s++) {
            float t1 = (float)s / 10.0f;
            float t2 = (float)(s + 1) / 10.0f;
            
            float rx1 = root_x1 + (tip_x - root_x1) * t1;
            float ry1 = root_y1 + (tip_y - root_y1) * t1;
            float rz1 = root_z1 + (tip_z - root_z1) * t1;
            float rr1 = sqrtf(ry1*ry1 + rz1*rz1);
            
            float rx2 = root_x1 + (tip_x - root_x1) * t2;
            float ry2 = root_y1 + (tip_y - root_y1) * t2;
            float rz2 = root_z1 + (tip_z - root_z1) * t2;
            float rr2 = sqrtf(ry2*ry2 + rz2*rz2);
            
            // Толщина лопатки ~1.5 мм
            float thick = 0.0015f;
            float ny1 = ry1 / rr1 * thick;
            float nz1 = rz1 / rr1 * thick;
            float ny2 = ry2 / rr2 * thick;
            float nz2 = rz2 / rr2 * thick;
            
            // Передняя поверхность
            fct(stl,
                {rx1, ry1, rz1},
                {rx2, ry2, rz2},
                {rx2, ry2 + ny2, rz2 + nz2});
            fct(stl,
                {rx1, ry1, rz1},
                {rx2, ry2 + ny2, rz2 + nz2},
                {rx1, ry1 + ny1, rz1 + nz1});
            // Задняя поверхность
            fct(stl,
                {rx1, ry1, rz1},
                {rx1, ry1 + ny1, rz1 + nz1},
                {rx2, ry2 + ny2, rz2 + nz2});
            fct(stl,
                {rx1, ry1, rz1},
                {rx2, ry2 + ny2, rz2 + nz2},
                {rx2, ry2, rz2});
        }
    }
    
    // ==================== КОРПУС ДЕТАНДЕРА (УЛИТКА) ====================
    float volute_x = exp_wheel_x + EXP_WHEEL_WIDTH * 0.3f;
    float volute_r = exp_r_tip + 0.015f;
    float volute_width = 0.035f;
    
    // Улитка (упрощённая — спираль)
    for(int i = 0; i < 36; i++) {
        float a1 = i * 10.0f * M_PI / 180.0f;
        float a2 = (i + 1) * 10.0f * M_PI / 180.0f;
        float r1 = volute_r + 0.010f * (float)i / 36.0f;  // расширяется
        float r2 = volute_r + 0.010f * (float)(i + 1) / 36.0f;
        
        cylinder_hollow(stl,
            volute_x, volute_x + volute_width,
            r1, r1 - 0.003f,
            SEGMENTS, 0, 0);
    }
    
    // Выходной патрубок улитки
    float volute_outlet_x = volute_x + volute_width;
    float volute_outlet_r = 0.030f;
    
    cylinder_hollow(stl,
        volute_outlet_x, volute_outlet_x + 0.030f,
        volute_outlet_r + WALL_THICKNESS, volute_outlet_r,
        SEGMENTS, 0, 0);
    
    // ==================== ВАЛ ====================
    // Титан ВТ6, сплошной
    cylinder_solid(stl,
        shaft_start, motor_end + 0.020f,
        r_shaft, SEGMENTS, 0, 0);
    
    // Утолщение под колёса
    cylinder_solid(stl,
        exp_wheel_x - 0.005f, shaft_start,
        r_shaft + 0.003f, SEGMENTS, 0, 0);
    
    // ==================== ЛЕПЕСТКОВЫЕ ПОДШИПНИКИ ====================
    // 2 шт: перед детандером и после компрессора
    float brg_r_outer = BEARING_DIAM / 2.0f;
    
    for(int bi = 0; bi < 2; bi++) {
        float bx = (bi == 0) ? brg1_x : brg2_x;
        
        // Внешнее кольцо
        cylinder_hollow(stl,
            bx - BEARING_WIDTH/2, bx + BEARING_WIDTH/2,
            brg_r_outer, brg_r_outer - 0.004f,
            SEGMENTS, 0, 0);
        
        // Лепестки (8 шт)
        for(int p = 0; p < 8; p++) {
            float ang = p * 45.0f * M_PI / 180.0f;
            float lr = r_shaft + 0.0005f;  // зазор 0.5 мм
            
            fct(stl,
                {bx - BEARING_WIDTH/2 + 0.002f, lr*sinf(ang), lr*cosf(ang)},
                {bx + BEARING_WIDTH/2 - 0.002f, lr*sinf(ang), lr*cosf(ang)},
                {bx + BEARING_WIDTH/2 - 0.002f, (lr + 0.003f)*sinf(ang + 0.1f), (lr + 0.003f)*cosf(ang + 0.1f)});
            fct(stl,
                {bx - BEARING_WIDTH/2 + 0.002f, lr*sinf(ang), lr*cosf(ang)},
                {bx + BEARING_WIDTH/2 - 0.002f, (lr + 0.003f)*sinf(ang + 0.1f), (lr + 0.003f)*cosf(ang + 0.1f)},
                {bx - BEARING_WIDTH/2 + 0.002f, (lr + 0.003f)*sinf(ang + 0.1f), (lr + 0.003f)*cosf(ang + 0.1f)});
        }
    }
    
    // ==================== ЭЛЕКТРОДВИГАТЕЛЬ ====================
    float motor_r = MOTOR_DIAM / 2.0f;
    
    // Статор
    cylinder_hollow(stl,
        motor_start, motor_end,
        motor_r, motor_r - 0.020f,
        SEGMENTS, 0, 0);
    
    // Обмотки (упрощённо — зубцы)
    for(int t = 0; t < 12; t++) {
        float ang = t * 30.0f * M_PI / 180.0f;
        float tr = motor_r - 0.010f;
        float ty = tr * sinf(ang);
        float tz = tr * cosf(ang);
        
        cylinder_hollow(stl,
            motor_start + 0.005f, motor_end - 0.005f,
            0.006f, 0.0f, 12, ty, tz);
    }
    
    // Ротор (на валу)
    cylinder_hollow(stl,
        motor_start + 0.003f, motor_end - 0.003f,
        r_shaft + 0.019f, r_shaft,
        SEGMENTS, 0, 0);
    
    // Корпус мотора
    cylinder_hollow(stl,
        motor_start - 0.005f, motor_end + 0.005f,
        motor_r + 0.005f, motor_r,
        SEGMENTS, 0, 0);
    
    // ==================== КОМПРЕССОР (5 СТУПЕНЕЙ) ====================
    for(int s = 0; s < COMP_STAGES; s++) {
        float stage_x = comp_start + s * COMP_STAGE_SPACING;
        float t = (float)s / (COMP_STAGES - 1);
        float wheel_r = COMP_WHEEL_DIAM_1/2 + (COMP_WHEEL_DIAM_LAST/2 - COMP_WHEEL_DIAM_1/2) * t;
        float hub_r = wheel_r * 0.3f;
        float wheel_width = COMP_WHEEL_WIDTH * (1.0f - t * 0.3f);
        
        // Ступица
        cylinder_solid(stl,
            stage_x, stage_x + wheel_width,
            hub_r, SEGMENTS, 0, 0);
        
        // Лопатки (14 шт, со сплиттерами)
        int total_blades = COMP_BLADES;
        for(int b = 0; b < total_blades; b++) {
            float ang = b * (360.0f / total_blades) * M_PI / 180.0f;
            bool is_splitter = (b % 2 == 1);
            float b_start_r = is_splitter ? hub_r * 1.8f : hub_r * 1.2f;
            float b_end_r = wheel_r * 0.92f;
            
            float bx1 = stage_x + 0.003f;
            float bx2 = stage_x + wheel_width - 0.003f;
            
            // Простая радиальная лопатка
            P root1 = {bx1, b_start_r*sinf(ang), b_start_r*cosf(ang)};
            P root2 = {bx2, b_start_r*sinf(ang + 0.05f), b_start_r*cosf(ang + 0.05f)};
            P tip1 = {bx1, b_end_r*sinf(ang + 0.10f), b_end_r*cosf(ang + 0.10f)};
            P tip2 = {bx2, b_end_r*sinf(ang + 0.15f), b_end_r*cosf(ang + 0.15f)};
            
            fct(stl, root1, tip1, tip2);
            fct(stl, root1, tip2, root2);
            fct(stl, root1, root2, {root2.x, root2.y + 0.001f*sinf(ang), root2.z + 0.001f*cosf(ang)});
            fct(stl, root1, {root2.x, root2.y + 0.001f*sinf(ang), root2.z + 0.001f*cosf(ang)}, {root1.x, root1.y + 0.001f*sinf(ang), root1.z + 0.001f*cosf(ang)});
        }
        
        // Корпус ступени (диффузор)
        float diffuser_r = wheel_r + 0.012f;
        cylinder_hollow(stl,
            stage_x - 0.003f, stage_x + wheel_width + 0.003f,
            diffuser_r, diffuser_r - 0.003f,
            SEGMENTS, 0, 0);
        
        // Боковые стенки
        cylinder_hollow(stl,
            stage_x - 0.003f, stage_x - 0.001f,
            diffuser_r, hub_r + 0.001f,
            SEGMENTS, 0, 0);
        cylinder_hollow(stl,
            stage_x + wheel_width + 0.001f, stage_x + wheel_width + 0.003f,
            diffuser_r, hub_r + 0.001f,
            SEGMENTS, 0, 0);
    }
    
    // ==================== ВЫХОД КОМПРЕССОРА ====================
    float comp_out_x = comp_start + (COMP_STAGES - 1) * COMP_STAGE_SPACING + COMP_WHEEL_WIDTH + 0.020f;
    float comp_out_r = 0.015f;
    
    cylinder_hollow(stl,
        comp_out_x, comp_out_x + 0.030f,
        comp_out_r + WALL_THICKNESS, comp_out_r,
        SEGMENTS, 0, 0);
    
    // Фланец
    cylinder_hollow(stl,
        comp_out_x + 0.030f, comp_out_x + 0.030f + FLANGE_THICKNESS,
        comp_out_r + 0.010f, comp_out_r,
        SEGMENTS, 0, 0);
    
    for(int b = 0; b < 4; b++) {
        float ang = b * 90.0f * M_PI / 180.0f;
        cylinder_hollow(stl,
            comp_out_x + 0.030f, comp_out_x + 0.030f + FLANGE_THICKNESS,
            0.0035f, 0.0f, 16,
            (comp_out_r + 0.007f) * sinf(ang),
            (comp_out_r + 0.007f) * cosf(ang));
    }
    
    // ==================== ВЫХОД ДЕТАНДЕРА → ВХОД КОМПРЕССОРА ====================
    // Труба от улитки к первой ступени компрессора
    cable_segment(stl,
        {volute_outlet_x + 0.030f, 0, 0},
        {comp_start - 0.010f, 0, 0},
        volute_outlet_r, SEGMENTS);
    
    // ==================== ТЕПЛОИЗОЛЯЦИЯ ХОЛОДНОЙ ЧАСТИ ====================
    // Вакуумная рубашка вокруг детандера
    float insulation_start = exp_inlet_x - 0.010f;
    float insulation_end = shaft_start + 0.020f;
    float insulation_r = exp_r_tip + 0.025f;
    
    cylinder_hollow(stl,
        insulation_start, insulation_end,
        insulation_r, insulation_r - 0.002f,
        48, 0, 0);
    
    // Торцы изоляции
    cylinder_hollow(stl,
        insulation_start, insulation_start + 0.002f,
        insulation_r, r_exp_in + WALL_THICKNESS,
        48, 0, 0);
    cylinder_hollow(stl,
        insulation_end - 0.002f, insulation_end,
        insulation_r, r_shaft + 0.005f,
        48, 0, 0);

    // ==================== БУСТЕР-ВЕНТИЛЯТОР ====================
// Осевой вентилятор на входе, на общем валу
// Поджимает воздух с 1.0 до 1.65 бар
const float BOOSTER_DIAM = 0.100f;           // Ø100 мм
const float BOOSTER_HUB_DIAM = 0.035f;       // Ø ступицы 35 мм
const float BOOSTER_BLADES = 9;              // лопастей
const float BOOSTER_WIDTH = 0.030f;          // ширина
const float BOOSTER_CASING_LENGTH = 0.060f;  // длина корпуса

float booster_x = exp_inlet_x - 0.080f;      // перед входом детандера
float booster_r_tip = BOOSTER_DIAM / 2.0f;
float booster_r_hub = BOOSTER_HUB_DIAM / 2.0f;

// Ступица вентилятора
cylinder_solid(stl,
    booster_x, booster_x + BOOSTER_WIDTH,
    booster_r_hub, SEGMENTS, 0, 0);

// Лопасти (9 шт, осевые)
for(int b = 0; b < BOOSTER_BLADES; b++) {
    float ang = b * (360.0f / BOOSTER_BLADES) * M_PI / 180.0f;
    
    // Лопасть: от ступицы к периферии, с аэродинамической закруткой
    for(int s = 0; s < 8; s++) {
        float t1 = (float)s / 8.0f;
        float t2 = (float)(s + 1) / 8.0f;
        
        float r1 = booster_r_hub + (booster_r_tip - booster_r_hub) * t1;
        float r2 = booster_r_hub + (booster_r_tip - booster_r_hub) * t2;
        
        float twist1 = t1 * 0.4f;  // закрутка увеличивается к периферии
        float twist2 = t2 * 0.4f;
        
        float bx1 = booster_x + BOOSTER_WIDTH * t1;
        float bx2 = booster_x + BOOSTER_WIDTH * t2;
        
        P p1 = {bx1, r1 * sinf(ang + twist1), r1 * cosf(ang + twist1)};
        P p2 = {bx2, r2 * sinf(ang + twist2), r2 * cosf(ang + twist2)};
        P p1b = {bx1, r1 * sinf(ang + twist1 + 0.08f), r1 * cosf(ang + twist1 + 0.08f)};
        P p2b = {bx2, r2 * sinf(ang + twist2 + 0.08f), r2 * cosf(ang + twist2 + 0.08f)};
        
        // Передняя поверхность
        fct(stl, p1, p2, p2b);
        fct(stl, p1, p2b, p1b);
        // Задняя поверхность
        fct(stl, p1, p1b, p2b);
        fct(stl, p1, p2b, p2);
    }
}

// Корпус вентилятора (цилиндр)
cylinder_hollow(stl,
    booster_x - 0.005f, booster_x + BOOSTER_WIDTH + 0.005f,
    booster_r_tip + 0.004f, booster_r_tip + 0.002f,
    SEGMENTS, 0, 0);

// Входной конфузор вентилятора (забор воздуха)
float intake_x = booster_x - 0.040f;
cone_hollow(stl,
    intake_x, booster_x,
    0.080f, booster_r_tip + 0.003f,
    0.002f, SEGMENTS, 0, 0);

// Защитная сетка на входе
for(float my = -0.070f; my <= 0.070f; my += 0.008f) {
    for(float mz = -0.070f; mz <= 0.070f; mz += 0.008f) {
        if(my*my + mz*mz <= 0.065f*0.065f) {
            cable_segment(stl,
                {intake_x, my, mz},
                {intake_x, my + 0.006f, mz},
                0.0008f, 4);
            cable_segment(stl,
                {intake_x, my, mz},
                {intake_x, my, mz + 0.006f},
                0.0008f, 4);
        }
    }
}

// Выходной диффузор (к детандеру)
cone_hollow(stl,
    booster_x + BOOSTER_WIDTH, exp_inlet_x,
    booster_r_tip + 0.003f, r_exp_in + WALL_THICKNESS,
    0.003f, SEGMENTS, 0, 0);
    
    std::cout << "  050_turbine.stl — готово\n";
    stl.close();
}