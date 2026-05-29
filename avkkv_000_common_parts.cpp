// ============================================================
// АВККВ-1.000.001–005 — УНИФИЦИРОВАННЫЕ УЗЛЫ
// Сохранить как: avkkv_000_common_parts.cpp
// ============================================================
// 001 — Вал моноблочный (детандер + 5 колёс)
// 002 — Подшипник газодинамический лепестковый
// 003 — Уплотнение вакуумное лабиринтное
// 004 — Фланец криогенный (T < −50°C)
// 005 — Змеевик рекуператора (параметрический)
// ============================================================

// ============================================================
// 001 — ВАЛ МОНОБЛОЧНЫЙ
// ============================================================
// Материал: Титан ВТ6
// Установка: бустер-вентилятор → детандер → компрессор (5 колёс)
// Диаметр базовый: 20 мм
#include "avkkv_common.h"

struct MonoShaftParams {
    float total_length = 0.500f;
    float shaft_diam = 0.020f;
    
    // Позиции элементов на валу (от левого торца)
    float booster_pos = 0.020f;
    float booster_width = 0.030f;
    float exp_wheel_pos = 0.100f;
    float exp_wheel_width = 0.025f;
    float comp_stages[5] = {0.280f, 0.320f, 0.360f, 0.400f, 0.440f};
    float comp_stage_width = 0.018f;
    
    float brg1_pos = 0.160f;  // подшипник 1
    float brg2_pos = 0.240f;  // подшипник 2
    float motor_pos = 0.170f;
    float motor_length = 0.060f;
};

void generate_000_001_shaft(const char* filename, const MonoShaftParams& p = MonoShaftParams()) {
    BinarySTL stl;
    stl.open(filename);
    
    const int SEG = 64;
    const float r = p.shaft_diam / 2.0f;
    
    // Базовый вал
    cylinder_solid(stl, 0.0f, p.total_length, r, SEG, 0, 0);
    
    // Утолщение под бустер
    float boost_r = r + 0.005f;
    cylinder_solid(stl,
        p.booster_pos - 0.003f, p.booster_pos + p.booster_width + 0.003f,
        boost_r, SEG, 0, 0);
    
    // Утолщение под колесо детандера
    float exp_r = r + 0.006f;
    cylinder_solid(stl,
        p.exp_wheel_pos - 0.003f, p.exp_wheel_pos + p.exp_wheel_width + 0.003f,
        exp_r, SEG, 0, 0);
    
    // Утолщения под колёса компрессора
    for(int i = 0; i < 5; i++) {
        float comp_r_local = r + 0.004f - i * 0.0005f;
        cylinder_solid(stl,
            p.comp_stages[i] - 0.002f, p.comp_stages[i] + p.comp_stage_width + 0.002f,
            comp_r_local, SEG, 0, 0);
    }
    
    // Шейки под подшипники (точёные, чистовые)
    float brg_r = r - 0.0005f;  // минус 0.5 мм для посадки
    cylinder_solid(stl,
        p.brg1_pos - 0.012f, p.brg1_pos + 0.012f,
        brg_r, SEG, 0, 0);
    cylinder_solid(stl,
        p.brg2_pos - 0.012f, p.brg2_pos + 0.012f,
        brg_r, SEG, 0, 0);
    
    // Шейка под ротор электродвигателя
    float motor_r = r + 0.015f;  // ротор толще вала
    cylinder_solid(stl,
        p.motor_pos, p.motor_pos + p.motor_length,
        motor_r, SEG, 0, 0);
    
    // Шпоночные пазы (упрощённо — лыски)
    for(int i = 0; i < 5; i++) {
        float cx = p.comp_stages[i] + p.comp_stage_width / 2;
        float key_depth = 0.003f;
        float key_width = 0.004f;
        
        // Лыска на валу
        fct(stl,
            {cx - p.comp_stage_width/3, r, -key_width/2},
            {cx + p.comp_stage_width/3, r, -key_width/2},
            {cx + p.comp_stage_width/3, r - key_depth, key_width/2});
        fct(stl,
            {cx - p.comp_stage_width/3, r, -key_width/2},
            {cx + p.comp_stage_width/3, r - key_depth, key_width/2},
            {cx - p.comp_stage_width/3, r - key_depth, key_width/2});
    }
    
    // Центровочные отверстия на торцах
    cylinder_solid(stl, 0.0f, 0.008f, 0.003f, 12, 0, 0);
    cylinder_solid(stl, p.total_length - 0.008f, p.total_length, 0.003f, 12, 0, 0);
    
    std::cout << "  000_001_shaft.stl — готово\n";
    stl.close();
}

// ============================================================
// 002 — ПОДШИПНИК ГАЗОДИНАМИЧЕСКИЙ ЛЕПЕСТКОВЫЙ
// ============================================================
// Материал лепестков: Инконель 718
// Смазка: не требуется (газодинамический)
// Рабочая температура: −196°C до +200°C

struct BearingParams {
    float shaft_diam = 0.020f;        // диаметр вала
    float outer_diam = 0.035f;        // внешний диаметр
    float width = 0.020f;             // ширина
    int petals = 8;                   // количество лепестков
    float petal_thickness = 0.0003f;  // толщина лепестка 0.3 мм
    float clearance = 0.00002f;       // зазор 20 мкм
};

void generate_000_002_bearing(const char* filename, const BearingParams& p = BearingParams()) {
    BinarySTL stl;
    stl.open(filename);
    
    const int SEG = 64;
    float r_shaft = p.shaft_diam / 2.0f;
    float r_outer = p.outer_diam / 2.0f;
    float x1 = -p.width / 2.0f;
    float x2 = p.width / 2.0f;
    
    // Внешнее кольцо
    cylinder_hollow(stl, x1, x2,
        r_outer, r_outer - 0.004f,
        SEG, 0, 0);
    
    // Лепестки
    for(int i = 0; i < p.petals; i++) {
        float ang = i * (360.0f / p.petals) * M_PI / 180.0f;
        
        // Лепесток — тонкая изогнутая пластина
        float base_r = r_shaft + p.clearance;
        float mid_r = base_r + 0.002f;
        float tip_r = base_r + 0.003f;
        
        float base_ang = ang;
        float mid_ang = ang + 0.15f;
        float tip_ang = ang + 0.25f;
        
        P base_inner = {x1 + 0.002f, base_r * sinf(base_ang), base_r * cosf(base_ang)};
        P base_outer = {x2 - 0.002f, base_r * sinf(base_ang), base_r * cosf(base_ang)};
        P tip_inner = {x1 + 0.002f, tip_r * sinf(tip_ang), tip_r * cosf(tip_ang)};
        P tip_outer = {x2 - 0.002f, tip_r * sinf(tip_ang), tip_r * cosf(tip_ang)};
        P mid_inner = {x1 + 0.002f, mid_r * sinf(mid_ang), mid_r * cosf(mid_ang)};
        P mid_outer = {x2 - 0.002f, mid_r * sinf(mid_ang), mid_r * cosf(mid_ang)};
        
        // Внутренняя поверхность лепестка
        fct(stl, base_inner, tip_inner, tip_outer);
        fct(stl, base_inner, tip_outer, base_outer);
        fct(stl, base_inner, mid_inner, tip_inner);
        fct(stl, base_inner, mid_outer, mid_inner);
        fct(stl, base_inner, base_outer, mid_outer);
        
        // Внешняя поверхность (смещена на толщину)
        P base_inner_t = {base_inner.x, base_inner.y + p.petal_thickness * sinf(base_ang + M_PI/2), base_inner.z + p.petal_thickness * cosf(base_ang + M_PI/2)};
        P tip_inner_t = {tip_inner.x, tip_inner.y + p.petal_thickness * sinf(tip_ang + M_PI/2), tip_inner.z + p.petal_thickness * cosf(tip_ang + M_PI/2)};
        
        fct(stl, base_inner, base_inner_t, tip_inner_t);
        fct(stl, base_inner, tip_inner_t, tip_inner);
    }
    
    // Крепёжные фланцы (по торцам)
    for(int side = -1; side <= 1; side += 2) {
        float fx = side * p.width / 2.0f;
        cylinder_hollow(stl,
            fx - 0.001f, fx + 0.001f * side,
            r_outer + 0.008f, r_outer - 0.002f,
            SEG, 0, 0);
        
        // Отверстия под винты (6 шт)
        for(int h = 0; h < 6; h++) {
            float ang = h * 60.0f * M_PI / 180.0f;
            cylinder_hollow(stl,
                fx - 0.003f, fx + 0.003f * side,
                0.002f, 0.0f, 12,
                (r_outer + 0.005f) * sinf(ang),
                (r_outer + 0.005f) * cosf(ang));
        }
    }
    
    std::cout << "  000_002_bearing.stl — готово\n";
    stl.close();
}

// ============================================================
// 003 — УПЛОТНЕНИЕ ВАКУУМНОЕ ЛАБИРИНТНОЕ
// ============================================================
// Назначение: герметизация вала без трения
// Принцип: лабиринт из чередующихся гребней и канавок
// Материал: AISI 316L или фторопласт

struct LabyrinthSealParams {
    float shaft_diam = 0.020f;
    float outer_diam = 0.030f;
    float length = 0.025f;
    int grooves = 5;                  // количество канавок
    float groove_depth = 0.002f;      // глубина канавки
    float groove_width = 0.002f;      // ширина канавки
    float clearance = 0.00015f;       // радиальный зазор 0.15 мм
};

void generate_000_003_labyrinth_seal(const char* filename, const LabyrinthSealParams& p = LabyrinthSealParams()) {
    BinarySTL stl;
    stl.open(filename);
    
    const int SEG = 64;
    float r_shaft = p.shaft_diam / 2.0f;
    float r_outer = p.outer_diam / 2.0f;
    float x1 = 0.0f;
    float x2 = p.length;
    
    // Внешнее кольцо
    cylinder_hollow(stl, x1, x2,
        r_outer, r_outer - 0.003f,
        SEG, 0, 0);
    
    // Гребни (выступы внутрь)
    float pitch = p.length / (p.grooves + 1);
    for(int g = 0; g < p.grooves; g++) {
        float gx1 = x1 + pitch * (g + 0.5f) - p.groove_width / 2;
        float gx2 = x1 + pitch * (g + 0.5f) + p.groove_width / 2;
        float gr = r_shaft + p.clearance;
        
        // Гребень
        cylinder_hollow(stl, gx1, gx2,
            gr + p.groove_depth, gr,
            SEG, 0, 0);
    }
    
    // Торцевые крышки
    for(int side = 0; side < 2; side++) {
        float sx = side == 0 ? x1 : x2;
        float sign = side == 0 ? 1.0f : -1.0f;
        
        cylinder_hollow(stl,
            sx, sx + 0.002f * sign,
            r_outer, r_shaft + p.clearance,
            SEG, 0, 0);
    }
    
    // Линия отсоса утечек
    float tap_x = x1 + p.length * 0.5f;
    float tap_r = r_outer + 0.005f;
    cylinder_hollow(stl,
        tap_x - 0.003f, tap_x + 0.003f,
        0.004f, 0.002f,
        16, tap_r, 0);
    
    std::cout << "  000_003_labyrinth_seal.stl — готово\n";
    stl.close();
}

// ============================================================
// 004 — ФЛАНЕЦ КРИОГЕННЫЙ
// ============================================================
// Применение: все стыки с температурой ниже −50°C
// Материал: AISI 316L
// Уплотнение: индиевая прокладка или фторопласт
// Особенность: удлинённая горловина для снижения теплопритока

struct CryoFlangeParams {
    float inner_diam = 0.050f;        // внутренний диаметр
    float bolt_circle_diam = 0.080f;  // диаметр окружности болтов
    int bolt_count = 6;               // количество болтов
    float bolt_hole_diam = 0.007f;    // Ø отверстий (M6 с зазором)
    float flange_thickness = 0.010f;  // толщина фланца
    float neck_length = 0.025f;       // длина горловины
    float wall_thickness = 0.004f;    // толщина стенки
    float groove_depth = 0.0015f;     // глубина канавки под уплотнение
    float groove_width = 0.003f;      // ширина канавки
};

void generate_000_004_cryo_flange(BinarySTL& stl, float pos_x, float pos_y, float pos_z,
                                   float dir_x, float dir_y, float dir_z,
                                   const CryoFlangeParams& p = CryoFlangeParams()) {
    // dir — направление оси трубы (от фланца)
    const int SEG = 64;
    float r_inner = p.inner_diam / 2.0f;
    float r_bolt = p.bolt_circle_diam / 2.0f;
    float r_outer = r_bolt + 0.010f;
    
    // Нормализуем направление
    float dlen = sqrtf(dir_x*dir_x + dir_y*dir_y + dir_z*dir_z);
    float nx = dir_x / dlen;
    float ny = dir_y / dlen;
    float nz = dir_z / dlen;
    
    // Для простоты (пока ось X — основная), используем offset
    float fx = pos_x;
    float fy = pos_y;
    float fz = pos_z;
    
    // Горловина
    cylinder_hollow(stl,
        fx, fx + p.neck_length,
        r_inner + p.wall_thickness, r_inner,
        SEG, fy, fz);
    
    // Диск фланца
    cylinder_hollow(stl,
        fx + p.neck_length, fx + p.neck_length + p.flange_thickness,
        r_outer, r_inner,
        SEG, fy, fz);
    
    // Канавка под уплотнение
    float groove_r = r_inner + 0.005f;
    cylinder_hollow(stl,
        fx + p.neck_length + p.flange_thickness * 0.4f,
        fx + p.neck_length + p.flange_thickness * 0.6f,
        groove_r + p.groove_width/2, groove_r - p.groove_width/2,
        SEG, fy, fz);
    
    // Отверстия под болты
    for(int b = 0; b < p.bolt_count; b++) {
        float ang = b * (360.0f / p.bolt_count) * M_PI / 180.0f;
        cylinder_hollow(stl,
            fx + p.neck_length, fx + p.neck_length + p.flange_thickness,
            p.bolt_hole_diam / 2.0f, 0.0f,
            24, fy + r_bolt * sinf(ang), fz + r_bolt * cosf(ang));
    }
    
    // Рёбра жёсткости (опционально)
    for(int rib = 0; rib < p.bolt_count; rib++) {
        float ang = rib * (360.0f / p.bolt_count) * M_PI / 180.0f;
        float ry = r_inner + p.wall_thickness;
        float roy = r_outer;
        
        fct(stl,
            {fx + 0.005f, fy + ry * sinf(ang), fz + ry * cosf(ang)},
            {fx + p.neck_length, fy + ry * sinf(ang), fz + ry * cosf(ang)},
            {fx + p.neck_length, fy + roy * sinf(ang), fz + roy * cosf(ang)});
        fct(stl,
            {fx + 0.005f, fy + ry * sinf(ang), fz + ry * cosf(ang)},
            {fx + p.neck_length, fy + roy * sinf(ang), fz + roy * cosf(ang)},
            {fx + 0.005f, fy + roy * sinf(ang), fz + roy * cosf(ang)});
    }
}

// Отдельный файл для криофланца (заглушка-образец)
void generate_000_004_cryo_flange_sample(const char* filename) {
    BinarySTL stl;
    stl.open(filename);
    
    CryoFlangeParams p;
    p.inner_diam = 0.050f;
    generate_000_004_cryo_flange(stl, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, p);
    
    std::cout << "  000_004_cryo_flange.stl — готово\n";
    stl.close();
}

// ============================================================
// 005 — ЗМЕЕВИК РЕКУПЕРАТОРА (ПАРАМЕТРИЧЕСКИЙ)
// ============================================================
// Назначение: теплообмен «газ → лёд/вода»
// Материал: AISI 304, трубка 12×1 мм
// Параметры: диаметр, высота, количество витков

struct CoilParams {
    float tube_od = 0.012f;           // внешний диаметр трубки
    float tube_id = 0.010f;           // внутренний диаметр
    float coil_diam = 0.250f;         // диаметр змеевика
    float turns = 8;                  // количество витков
    float pitch = 0.035f;             // шаг витка
    float inlet_extension = 0.050f;   // удлинение входа
    float outlet_extension = 0.050f;  // удлинение выхода
};

void generate_000_005_coil(BinarySTL& stl, float bottom_y, float center_y, float center_z,
                           const CoilParams& p = CoilParams()) {
    const int COIL_SEG = 72;
    float r_tube = p.tube_od / 2.0f;
    float r_coil = p.coil_diam / 2.0f;
    
    // Витки
    for(int t = 0; t < (int)p.turns; t++) {
        float y_base = bottom_y + t * p.pitch;
        
        for(int i = 0; i < COIL_SEG; i++) {
            float a1 = i * 2.0f * M_PI / COIL_SEG;
            float a2 = (i + 1) * 2.0f * M_PI / COIL_SEG;
            
            P p1 = {y_base, center_y + r_coil * sinf(a1), center_z + r_coil * cosf(a1)};
            P p2 = {y_base, center_y + r_coil * sinf(a2), center_z + r_coil * cosf(a2)};
            
            cable_segment(stl, p1, p2, r_tube, 16);
        }
    }
    
    // Подвод (верхний конец)
    float top_y = bottom_y + p.turns * p.pitch;
    cable_segment(stl,
        {top_y, center_y + r_coil, center_z},
        {top_y, center_y + r_coil + p.inlet_extension, center_z},
        r_tube, 16);
    
    // Отвод (нижний конец)
    cable_segment(stl,
        {bottom_y, center_y + r_coil, center_z},
        {bottom_y, center_y + r_coil + p.outlet_extension, center_z},
        r_tube, 16);
}

void generate_000_005_coil_sample(const char* filename) {
    BinarySTL stl;
    stl.open(filename);
    
    CoilParams p;
    generate_000_005_coil(stl, 0.0f, 0.0f, 0.0f, p);
    
    std::cout << "  000_005_coil.stl — готово\n";
    stl.close();
}