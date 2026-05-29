// ============================================================
// AVKKV HARDCORE SOLVER v1.0
// Полная физическая симуляция генератора воды
// ============================================================

#include <cstdio>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==================== ВЕЩЕСТВА ====================
struct Substance {
    const char* name;
    double M;          // молярная масса
    double T_crit;     // критическая температура
    double P_crit;     // критическое давление
    double cp_T[3];    // коэффициенты cp(T)
    double H_form;     // энтальпия образования
};

const Substance AIR   = {"Воздух",    0.029,  132.5, 3.77e6, {1005.0, 0.0, 0.0}, 0.0};
const Substance H2    = {"Водород",   0.002,  33.2,  1.30e6, {14300.0, 0.0, 0.0}, 0.0};
const Substance O3    = {"Озон",      0.048,  261.0, 5.57e6, {820.0, 0.0, 0.0}, 142.0e6};
const Substance H2O_g = {"Вода(пар)", 0.018,  647.1, 22.06e6, {1860.0, 0.0, 0.0}, -13.4e6};
const Substance H2O_l = {"Вода(жид)", 0.018,  647.1, 22.06e6, {4180.0, 0.0, 0.0}, -15.9e6};
const Substance H2O_s = {"Лёд",       0.018,  647.1, 22.06e6, {2100.0, 0.0, 0.0}, -15.9e6};

// ==================== КОНСТАНТЫ ====================
const double R_UNIV = 8.314462618;
const double SIGMA_SB = 5.670374419e-8;  // Стефана-Больцмана

// Геометрия (из генератора)
const double V_BUNKER = 0.050;        // 50 л
const double A_COIL = 0.0942;         // поверхность змеевика, м²
const double L_MIXER = 0.800;         // длина камеры
const double D_MIXER_IN = 0.040;
const double D_MIXER_OUT = 0.140;
const double V_CYCLONE = 0.0085;
const double EFF_CYCLONE = 0.95;      // эффективность сепарации

// Турбомашина
const double PI_EXP = 6.5;
const double PI_COMP = 40.0;
const double ETA_EXP = 0.85;
const double ETA_COMP_POLY = 0.85;
const double ETA_MECH = 0.98;

// PEM топливный элемент
const double PEM_CELLS = 20;
const double PEM_AREA = 0.03;         // м² на ячейку
const double PEM_EFF = 0.55;          // КПД
const double PEM_VOLTAGE_NOM = 0.70;  // В

// Озон
const double O3_T_HALF = 1800.0;      // период полураспада при 20°C, сек
const double O3_ACTIVATION = 8500.0;  // энергия активации, Дж/моль

// ==================== МАТЕМАТИКА ====================

// Аррениус
inline double arrhenius(double A, double Ea, double T) {
    return A * exp(-Ea / (R_UNIV * T));
}

// Теплоёмкость (кусочно-линейная)
inline double cp_T(const Substance& s, double T) {
    return s.cp_T[0] + s.cp_T[1] * T + s.cp_T[2] * T * T;
}

// Давление насыщенного пара (Гоффа-Гратча)
inline double p_sat_H2O(double T) {
    if (T < 273.16) {
        // Над льдом
        return 611.15 * exp(22.452 * (T - 273.16) / (T - 0.6));
    } else {
        // Над водой
        return 611.21 * exp(17.502 * (T - 273.16) / (T - 32.18));
    }
}

// Абсолютная влажность
inline double abs_humidity(double T, double RH, double P = 101325.0) {
    double psat = p_sat_H2O(T);
    double pv = RH * psat;
    return 0.622 * pv / (P - pv);
}

// Число Нуссельта (Dittus-Boelter)
inline double nusselt(double Re, double Pr, bool heating = true) {
    if (heating) return 0.023 * pow(Re, 0.8) * pow(Pr, 0.4);
    else         return 0.023 * pow(Re, 0.8) * pow(Pr, 0.3);
}

// ==================== КЛАССЫ ====================

// --- АТМОСФЕРА ---
class AtmosphereAVKKV {
public:
    double T = 298.15;      // K
    double P = 101325.0;    // Па
    double RH = 0.80;       // отн. влажность
    double abs_hum = 0.016; // кг/кг
    
    void update(double T_new, double RH_new) {
        T = T_new;
        RH = RH_new;
        abs_hum = abs_humidity(T, RH, P);
    }
};

// --- КОНФУЗОР + БУСТЕР ---
class BoosterInlet {
public:
    double m_dot = 0.0545;   // кг/с
    double T_out = 298.0;
    double P_out = 101325.0;
    double power = 0.0;
    
    void compute(const AtmosphereAVKKV& atm, double pi = 1.65, double eta = 0.80) {
        double k = 1.4;
        T_out = atm.T * pow(pi, (k-1)/k);
        double T_real = atm.T + (T_out - atm.T) / eta;
        power = m_dot * cp_T(AIR, atm.T) * (T_real - atm.T);
        P_out = atm.P * pi;
        T_out = T_real;
    }
};

// --- КАМЕРА СМЕШЕНИЯ + ДЕСУБЛИМАТОР ---
class MixerDesublimator {
public:
    double T_gas = 190.0;     // после смешения с LH₂
    double P_gas = 150000.0;
    double m_water_ice = 0.0; // кг/с выпавшего льда
    double conversion_H2 = 0.65;  // конверсия H₂ в камере
    double m_H2_reacted = 0.0;
    double Q_reaction = 0.0;  // тепло реакции
    
void compute(double m_air, double m_H2, double m_O3, double T_in, double P_in) {
    // СБРОС
    m_water_ice = 0.0;
    m_H2_reacted = 0.0;
    Q_reaction = 0.0;
    
    // 1. Объём камеры (из геометрии)
    double V_mixer = L_MIXER * M_PI * pow((D_MIXER_IN + D_MIXER_OUT) / 4.0, 2);
    
    // 2. Плотность газа
    double R_air = R_UNIV / AIR.M;
    double rho = P_in / (R_air * T_in);
    
    // 3. Объёмный расход
    double V_dot = (m_air + m_H2) / rho;
    
    // 4. Время пребывания
    double tau = V_mixer / V_dot;
    tau = std::max(0.1, std::min(2.0, tau));
    
    // 5. Концентрации (моль/м³)
    double conc_H2 = (m_H2 / H2.M) / V_dot;
    double conc_O3 = (m_O3 / O3.M) / V_dot;
    
    // 6. Кинетика (Аррениус)
    double k0 = 1.0e6;
    double Ea = 15000.0;
    double k = arrhenius(k0, Ea, T_in);
    
    // 7. Скорость реакции
    double rate = k * conc_H2 * conc_H2 * conc_H2 * conc_O3;
    
    // 8. Лимитирующий реагент
    double max_H2_by_O3 = 3.0 * conc_O3;
    double max_conversion = max_H2_by_O3 / (conc_H2 + 1e-10);
    max_conversion = std::min(1.0, max_conversion);
    
    // 9. Конверсия
    conversion_H2 = 1.0 - exp(-rate * tau / (conc_H2 + 1e-10));
    conversion_H2 = std::min(max_conversion, conversion_H2);
    conversion_H2 = std::max(0.0, std::min(0.95, conversion_H2));
    
    m_H2_reacted = m_H2 * conversion_H2;
    m_water_ice = m_H2_reacted * 9.0;
    Q_reaction = m_H2_reacted * 142.0e6;
    
    double cp_mix = cp_T(AIR, T_in) * m_air / (m_air + m_H2 + 1e-10);
    T_gas = T_in - (m_H2 * 450000.0) / ((m_air + m_H2) * cp_mix + 1e-10) 
                  + Q_reaction / ((m_air + m_H2) * cp_mix + 1e-10);
    T_gas = std::max(80.0, std::min(300.0, T_gas));
    P_gas = P_in * 0.95;
}
};

// --- ЦИКЛОН ---
class CycloneSeparator {
public:
    double m_gas_out = 0.0;
    double m_ice_out = 0.0;
    double T_out = 0.0;
    double P_out = 0.0;
    double efficiency = 0.95;
    
    void compute(double m_gas_in, double m_ice_in, double T_in, double P_in) {
        // Расчёт эффективности по размеру частиц
        double d50 = 5.0e-6;  // 5 мкм — граничный размер
        double d_particle = 20.0e-6;  // средний размер кристаллов
        
        efficiency = 1.0 - exp(-pow(d_particle / d50, 1.5));
        efficiency = std::min(0.99, std::max(0.80, efficiency));
        
        m_ice_out = m_ice_in * efficiency;
        m_gas_out = m_gas_in;
        T_out = T_in;
        P_out = P_in * 0.98;
    }
};

// --- ТУРБОДЕТАНДЕР + КОМПРЕССОР (v2 — 5 ступеней с охлаждением) ---
class TurboExpanderCompressor {
public:
    double T_exp_out = 0.0;
    double P_exp_out = 0.0;
    double W_exp = 0.0;
    double T_comp_out = 0.0;
    double P_comp_out = 0.0;
    double W_comp = 0.0;
    double W_motor = 0.0;
    double T_after_cooler = 0.0;
    
    void compute(double m_dot, double T_in, double P_in, double T_ambient) {
        double k = 1.4;
        double cp = cp_T(AIR, T_in);
        
        // ==================== ДЕТАНДЕР ====================
        double T_exp_s = T_in * pow(1.0 / PI_EXP, (k-1)/k);
        T_exp_out = T_in - ETA_EXP * (T_in - T_exp_s);
        P_exp_out = P_in / PI_EXP;
        W_exp = m_dot * cp * (T_in - T_exp_out);
        
        // ==================== КОМПРЕССОР (5 СТУПЕНЕЙ) ====================
        double n_poly = 1.0 / (1.0 - (k-1)/(k * ETA_COMP_POLY));
        double pi_stage = pow(PI_COMP, 1.0/5.0);
        
        double T_stage = T_exp_out;
        double P_stage = P_exp_out;
        W_comp = 0.0;
        
        for(int s = 0; s < 5; s++) {
            double T_stage_s = T_stage * pow(pi_stage, (n_poly-1)/n_poly);
            double T_stage_real = T_stage + (T_stage_s - T_stage) / ETA_COMP_POLY;
            
            W_comp += m_dot * cp * (T_stage_real - T_stage);
            T_stage = T_stage_real;
            P_stage *= pi_stage;
            
            // Промежуточное охлаждение после 2-й и 4-й ступени
            if(s == 1 || s == 3) {
                double T_cooled = T_stage - (T_stage - T_ambient) * 0.65;
                T_stage = T_cooled;
                // Лёгкое падение давления в охладителе
                P_stage *= 0.98;
            }
        }
        
        T_comp_out = T_stage;
        P_comp_out = P_stage;
        
        // ==================== ЭЛЕКТРОДВИГАТЕЛЬ ====================
        double W_net = W_comp - W_exp * ETA_MECH;
        // Бустер-вентилятор: 1.5% от массового расхода в кВт
        double W_booster = 150.0 * m_dot;
        // Потери в подшипниках (два лепестковых подшипника)
        double W_bearings = 50.0;
        // Контроллер + вентильный привод
        double W_controller = 100.0;
        
        W_motor = (W_net + W_booster) / 0.92 + W_bearings + W_controller;
        W_motor = std::max(0.0, W_motor);
        
        T_after_cooler = T_comp_out - (T_comp_out - T_ambient) * 0.5;
    }
};

// --- БУНКЕР-РЕКУПЕРАТОР (v2 — реалистичный теплообмен) ---
class BunkerRecuperator {
public:
    double m_water = 0.0;
    double T_water_out = 283.0;
    double T_gas_out = 0.0;
    double Q_melt = 0.0;
    double Q_avail = 0.0;
    double level = 0.0;
    double m_ice_stored = 0.0;
    
    void compute(double m_ice, double m_gas, double T_gas_in, double P_gas, double dt) {
        // Накопление льда
        m_ice_stored += m_ice * dt;
        
        // ==================== ТЕПЛООБМЕН В ЗМЕЕВИКЕ ====================
        double cp_gas = cp_T(AIR, T_gas_in);
        
        // Параметры змеевика
        const double D_TUBE = 0.010;   // внутренний диаметр трубки (10 мм)
        const double L_COIL = 6.3;     // общая длина змеевика
        const double A_COIL = M_PI * D_TUBE * L_COIL;  // поверхность теплообмена
        
        // Скорость газа
        double rho_gas = P_gas / (R_UNIV / AIR.M * T_gas_in);
        double A_tube = M_PI * D_TUBE * D_TUBE / 4.0;
        double v_gas = m_gas / (rho_gas * A_tube);
        v_gas = std::max(0.1, std::min(30.0, v_gas));
        
        // Число Рейнольдса
        double mu_gas = 1.8e-5;  // вязкость воздуха
        double Re = rho_gas * v_gas * D_TUBE / mu_gas;
        double Pr = 0.71;
        
        // Число Нуссельта (охлаждение газа)
        double Nu = 0.023 * pow(Re, 0.8) * pow(Pr, 0.3);
        double k_gas = 0.026;  // теплопроводность воздуха
        double h_gas = Nu * k_gas / D_TUBE;
        
        // Коэффициент теплопередачи (через стенку)
        double k_wall = 15.0;  // нержавейка
        double h_wall = k_wall / 0.001;  // 1 мм стенка
        double h_ice = 500.0;  // со стороны льда/воды
        
        double U = 1.0 / (1.0/h_gas + 1.0/h_wall + 1.0/h_ice);
        
        // Тепловой поток (метод среднего логарифмического напора)
        double T_ice = 273.15;  // температура льда/воды
        double dT1 = T_gas_in - T_ice;
        double dT_guess = 20.0;  // оценка охлаждения газа
        double dT2 = (T_gas_in - dT_guess) - T_ice;
        dT2 = std::max(1.0, dT2);
        
        double LMTD = (dT1 - dT2) / log(dT1 / dT2);
        LMTD = std::max(1.0, LMTD);
        
        Q_avail = U * A_COIL * LMTD;
        
        // ==================== ПЛАВЛЕНИЕ ЛЬДА ====================
        // Теплота: плавление (334 кДж/кг) + нагрев до +10°C (42 кДж/кг)
        double melt_enthalpy = 334000.0 + 42000.0;
        
        double melt_rate = Q_avail / melt_enthalpy;
        melt_rate = std::min(melt_rate, m_ice_stored / dt + m_ice);
        
        m_water = melt_rate;
        m_ice_stored -= melt_rate * dt;
        m_ice_stored = std::max(0.0, m_ice_stored);
        
        Q_melt = melt_rate * 334000.0;
        
        // ==================== ТЕМПЕРАТУРА ГАЗА НА ВЫХОДЕ ====================
        T_gas_out = T_gas_in - Q_melt / (m_gas * cp_gas + 0.001);
        T_gas_out = std::max(T_ice + 2.0, std::min(T_gas_in - 1.0, T_gas_out));
        
        // Уровень заполнения (лёд + вода)
        double V_ice = m_ice_stored / 920.0;  // плотность льда
        double V_water = (m_water * dt) / 1000.0;  // накопленная вода
        level = (V_ice + V_water) / V_BUNKER;
        level = std::min(0.95, level);
    }
};
// --- PEM ТОПЛИВНЫЙ ЭЛЕМЕНТ ---
class PEMFuelCell {
public:
    double power = 0.0;          // Вт
    double voltage = 0.0;        // В
    double current = 0.0;        // А
    double m_H2_consumed = 0.0;  // кг/с
    double m_water = 0.0;        // кг/с воды
    double efficiency = 0.55;
    double degradation = 0.0;    // 0-1, деградация мембраны
    double T_stack = 333.0;      // 60°C
    
    void compute(double m_H2_avail, double m_O3_residual, double dt) {
        // Озон сначала разлагается на CeO₂
        double O3_survived = m_O3_residual * exp(-dt / 0.01);  // быстрое разложение
        
            m_water = 0.0;         
            m_H2_consumed = 0.0;   
            power = 0.0;           
        
        // Если озон прошёл — ускоренная деградация
        if (O3_survived > 1e-9) {
            degradation += O3_survived * 1e6 * dt;  // мгновенная деградация
        }
        
        // Нормальная работа PEM
        m_H2_consumed = m_H2_avail * (1.0 - degradation);
        m_H2_consumed = std::max(0.0, m_H2_consumed);
        
        double n_H2 = m_H2_consumed / 0.002016;  // моль/с
        double n_e = n_H2 * 2.0 * 96485.0;       // ток в Амперах
        current = n_e;
        
        voltage = 0.70 - 0.05 * degradation - 0.03 * log(current / 10.0 + 1.0);
        voltage = std::max(0.50, std::min(0.95, voltage));
        
        power = voltage * current;
        efficiency = voltage / 1.23;  // относительно термонейтрального
        m_water = m_H2_consumed * 9.0;
        
        // Нагрев стека
        double Q_waste = m_H2_consumed * 142.0e6 * (1.0 - efficiency);
        double C_stack = 5000.0;  // теплоёмкость стека
        T_stack += Q_waste * dt / C_stack;
        T_stack = std::max(333.0, std::min(363.0, T_stack));
    }
};

// --- ТЭГ (Пельтье) ---
class TEGenerator {
public:
    double power = 0.0;
    double eta = 0.05;
    
    void compute(double T_hot, double T_cold, double Q_avail) {
        double dT = T_hot - T_cold;
        if (dT < 20.0) { power = 0.0; return; }
        
        double ZT = 0.8;  // добротность
        eta = dT / T_hot * (sqrt(1.0 + ZT) - 1.0) / (sqrt(1.0 + ZT) + T_cold / T_hot);
        eta = std::min(0.12, eta);
        
        power = Q_avail * eta;
        power = std::min(1000.0, power);  // макс 1 кВт
    }
};

// --- КАТАЛИЗАТОР CeO₂ (ОБНОВЛЁННЫЙ) ---
class CeriaCatalyst {
public:
    double conversion = 1.0;
    double degradation = 0.0;
    double temperature = 333.0;
    
    void compute(double m_O3_in, double m_air, double T_gas, double P_gas, double dt) {
        // Геометрия катализатора
        const double D_CAT = 0.040;      // Ø40 мм
        const double L_CAT = 0.140;      // 140 мм
        const double CELL_SIZE = 0.002;  // ячейка 2×2 мм
        const double WALL_THICKNESS = 0.0005; // стенка 0.5 мм
        
        // Площадь поперечного сечения (за вычетом стенок)
        double A_cat = M_PI * pow(D_CAT / 2.0, 2);
        double cell_area = CELL_SIZE * CELL_SIZE;
        double wall_area = pow(CELL_SIZE + WALL_THICKNESS, 2) - cell_area;
        double porosity = cell_area / (cell_area + wall_area); // ~0.64
        double A_flow = A_cat * porosity;
        
        // Объём катализатора
        double V_cat = A_flow * L_CAT;
        
        // Плотность газа
        double R_air = R_UNIV / AIR.M;
        double rho = P_gas / (R_air * T_gas);
        double V_dot = m_air / rho;
        
        // Время контакта
        double tau = V_cat / V_dot;
        tau = std::max(0.01, std::min(0.5, tau));
        
        // Кинетика разложения O₃ на CeO₂ (Аррениус)
        double k0_cat = 5.0e7;   // 1/с (гетерогенный катализ)
        double Ea_cat = 25000.0; // Дж/моль
        double k_cat = arrhenius(k0_cat, Ea_cat, temperature);
        
        // Конверсия за время контакта (реакция 1-го порядка)
        double conversion_ideal = 1.0 - exp(-k_cat * tau);
        conversion = conversion_ideal * (1.0 - degradation);
        conversion = std::min(1.0, std::max(0.0, conversion));
        
        // Деградация катализатора от влаги
        degradation += 1e-8 * dt * (1.0 + 10.0 * (1.0 - conversion));
        degradation = std::min(1.0, degradation);
        
        // Разогрев от реакции
        double Q = m_O3_in * conversion * 3.0e6; // теплота разложения O₃
        double C_cat = 500.0; // теплоёмкость (CeO₂ + корпус)
        temperature += Q * dt / C_cat;
        temperature = std::max(300.0, std::min(600.0, temperature));
    }
};
// === СОЛНЕЧНАЯ ПАНЕЛЬ (РЕАЛИСТИЧНАЯ) ===
class SolarPanel {
public:
    double area = 50.0;           // м²
    double efficiency = 0.18;     // 18%
    double inverter_loss = 0.92;  // 8% потери
    double cable_loss = 0.95;     // 5% потери
    double power = 0.0;
    
    void compute(double hour_of_day, double cloud_cover = 0.0) {
        // Солнечная инсоляция по синусоиде (день)
        double insolation = 0.0;
        if (hour_of_day > 6.0 && hour_of_day < 20.0) {
            insolation = 1000.0 * sin((hour_of_day - 6.0) * M_PI / 14.0);
            insolation *= (1.0 - cloud_cover * 0.7);  // облачность
        }
        
        power = insolation * area * efficiency * inverter_loss * cable_loss;
        power = std::max(0.0, power);
    }
};

// === ВЕТРОГЕНЕРАТОР (РЕАЛИСТИЧНЫЙ) ===
class WindTurbine {
public:
    double rotor_diam = 3.0;       // м
    double efficiency = 0.35;      // КПД (Бетц + потери)
    double power = 0.0;
    
    void compute(double wind_speed) {
        double area = M_PI * rotor_diam * rotor_diam / 4.0;
        double rho = 1.225;  // плотность воздуха
        
        if (wind_speed < 3.0) power = 0.0;  // стартовая скорость
        else if (wind_speed > 25.0) power = 0.0;  // защита от урагана
        else {
            power = 0.5 * rho * area * pow(wind_speed, 3.0) * efficiency;
        }
        power = std::min(5000.0, power);  // макс 5 кВт
    }
};

// ==================== ГЛАВНЫЙ КЛАСС АВККВ ====================
class AVKKV_System {
public:
    AtmosphereAVKKV atm;
    BoosterInlet booster;
    MixerDesublimator mixer;
    CycloneSeparator cyclone;
    TurboExpanderCompressor turbo;
    BunkerRecuperator bunker;
    PEMFuelCell pem;
    TEGenerator teg;
    CeriaCatalyst catalyst;
    
    double m_air = 0.0545;
    double m_H2 = 0.00005;  // 50 мг/с
    double m_O3 = 0.00025;  // 250 мг/с — реалистичный поток озона
    
    double total_water = 0.0;  // литров
    double total_energy_in = 0.0;
    double total_energy_out = 0.0;
    double time = 0.0;
    
    double water_from_air = 0.0;
    double water_from_reaction = 0.0;
    double power_balance = 0.0;
    
    void simulate(double duration_hours = 24.0, double dt = 0.1) {
        printf("\n");
        printf("╔══════════════════════════════════════════════════════╗\n");
        printf("║       АВККВ — HARDCORE SIMULATION v1.0              ║\n");
        printf("║   Генератор воды из атмосферы + H₂ + O₃            ║\n");
        printf("╚══════════════════════════════════════════════════════╝\n\n");
        
        printf("Time(h) | T_amb | RH%%  | H2O_air | H2O_rxn | Total(L) | P_motor | P_pem | T_stack | Mode\n");
        printf("--------+-------+------+---------+---------+----------+---------+-------+--------+------\n");

            SolarPanel solar;
            solar.area = 350.0;
            WindTurbine wind;
            wind.rotor_diam = 12.0;
        
        atm.update(298.15, 0.80);
        
        int steps = (int)(duration_hours * 3600.0 / dt);
        for (int step = 0; step <= steps; step++) {
            time = step * dt;
            
            // 1. Моделируем суточные колебания
            double hour = fmod(time / 3600.0, 24.0);
            double T_amb = 293.15 + 10.0 * sin((hour - 6.0) * M_PI / 12.0);  // +15..+35°C
            double RH_amb = 0.50 + 0.40 * cos((hour - 4.0) * M_PI / 12.0);   // 50-90%
            RH_amb = std::max(0.20, std::min(0.98, RH_amb));
            atm.update(T_amb, RH_amb);
            
            // 2. Бустер
            booster.m_dot = m_air;
            booster.compute(atm);
            
            // 3. Камера смешения
            mixer.compute(m_air, m_H2, m_O3, booster.T_out, booster.P_out);
            
            // 4. Циклон
            cyclone.compute(m_air + m_H2, mixer.m_water_ice, mixer.T_gas, mixer.P_gas);
            
            // 5. Турбодетандер + компрессор
            turbo.compute(cyclone.m_gas_out, cyclone.T_out, cyclone.P_out, atm.T);
            
            // 6. Бункер
            bunker.compute(cyclone.m_ice_out, cyclone.m_gas_out, turbo.T_comp_out, turbo.P_comp_out, dt);
            
// 7. Катализатор (ОБНОВЛЁННЫЙ ВЫЗОВ)
catalyst.compute(
    m_O3 * (1.0 - mixer.conversion_H2),  // остаток O₃ после камеры
    cyclone.m_gas_out,                     // расход воздуха
    cyclone.T_out,                         // температура газа
    cyclone.P_out,                         // давление
    dt                                     // шаг времени
);

            
            // 8. PEM
            double H2_for_PEM = m_H2 * (1.0 - mixer.conversion_H2);
            double O3_residual = m_O3 * (1.0 - mixer.conversion_H2) * (1.0 - catalyst.conversion);
            pem.compute(H2_for_PEM, O3_residual, dt);
            
            // 8.5. ВИЭ
            double cloud = 0.3 + 0.2 * sin(hour * M_PI / 12.0);
            double wind_speed = 5.0 + 3.0 * cos(hour * M_PI / 8.0);
            solar.compute(hour, cloud);
            wind.compute(wind_speed);
            
            // 9. ТЭГ
            teg.compute(turbo.T_comp_out, turbo.T_exp_out, 500.0);
            
            // 10. Баланс
            water_from_air = (atm.abs_hum - abs_humidity(turbo.T_exp_out, 1.0, turbo.P_exp_out)) * m_air;
            water_from_air = std::max(0.0, water_from_air);
            water_from_reaction = mixer.m_water_ice + pem.m_water;
            
            double water_rate = water_from_air + water_from_reaction;
            total_water += water_rate * dt;
            
            double P_renewable = solar.power + wind.power;
            power_balance = pem.power + teg.power + P_renewable - turbo.W_motor;
            if (step % 3600 == 0) {
    printf("DEBUG: Solar=%.0fW Wind=%.0fW PEM=%.0fW Motor=%.0fW Bal=%.0fW\n",
           solar.power, wind.power, pem.power, turbo.W_motor, power_balance);
}
            
            // 11. Вывод
            if (step % 3600 == 0) {  // раз в час
                const char* mode = "NORM";
                if (pem.degradation > 0.5) mode = "DEGRADED";
                if (bunker.level > 0.9) mode = "FULL";
                if (catalyst.degradation > 0.3) mode = "CAT_LOW";
                
                printf("%6.1f h | %4.0fK | %3.0f%% | %5.2fL/h| %5.2fL/h| %6.1fL | %5.0fW | %4.0fW | %4.0fK | %s\n",
                       time/3600.0, atm.T, atm.RH*100,
                       water_from_air*3600, water_from_reaction*3600,
                       total_water,
                       turbo.W_motor, pem.power, pem.T_stack, mode);
            }
        }
        
        printf("--------+-------+------+---------+---------+----------+---------+-------+--------+------\n");
        
        // Итоговый отчёт
        print_report(duration_hours);
    }
    
    void print_report(double hours) {
        printf("\n");
        printf("╔══════════════════════════════════════════════════════╗\n");
        printf("║              ИТОГОВЫЙ ОТЧЁТ АВККВ                   ║\n");
        printf("╚══════════════════════════════════════════════════════╝\n\n");
        
        printf("  Время симуляции:         %.1f часов (%.1f суток)\n", hours, hours/24.0);
        printf("  Всего произведено воды:  %.1f литров\n", total_water);
        printf("  Средняя производительность: %.1f л/ч (%.0f л/сут)\n",
               total_water / hours, total_water / hours * 24.0);
        printf("\n");
        printf("  --- ИСТОЧНИКИ ВОДЫ ---\n");
        printf("  Из атмосферы:           %.1f%%\n", water_from_air / (water_from_air + water_from_reaction + 0.001) * 100);
        printf("  Из реакции H₂ + O₃:    %.1f%%\n", water_from_reaction / (water_from_air + water_from_reaction + 0.001) * 100);
        printf("\n");
        printf("  --- ЭНЕРГЕТИКА ---\n");
        printf("  Потребление мотора:     %.0f Вт\n", turbo.W_motor);
        printf("  PEM мощность:           %.0f Вт\n", pem.power);
        printf("  ТЭГ мощность:           %.0f Вт\n", teg.power);
        printf("  Баланс:                 %+.0f Вт\n", power_balance);
        printf("\n");
        printf("  --- СОСТОЯНИЕ УЗЛОВ ---\n");
        printf("  PEM деградация:         %.1f%%\n", pem.degradation * 100);
        printf("  Катализатор CeO₂:       %.1f%%\n", catalyst.degradation * 100);
        printf("  Бункер заполнен:        %.1f%%\n", bunker.level * 100);
        printf("  Конверсия H₂ в камере:  %.1f%%\n", mixer.conversion_H2 * 100);
        printf("\n");

        // В print_report():
        printf("  Из реакции H₂ + O₃:    %.1f%%\n", water_from_reaction / (total_water / hours) * 100);
        // Добавить:
        printf("  Из PEM (H₂ + воздух):   %.1f%%\n", (total_water/hours - water_from_air - water_from_reaction) / (total_water/hours) * 100);
        
        double COP = total_water / (turbo.W_motor * hours / 1000.0 + 0.001);
        printf("  --- ЭФФЕКТИВНОСТЬ ---\n");
        printf("  COP:                    %.1f л/кВт*ч\n", COP);
        printf("  Удельные затраты H₂:    %.2f кг/л воды\n", m_H2 * hours * 3600.0 / (total_water + 0.001));
        
        printf("\n  ╔════════════════════════════════════════════╗\n");
        if (COP > 5.0)
            printf("  ║  СТАТУС: ПРЕВОСХОДНО — всё в норме       ║\n");
        else if (pem.degradation < 0.1 && catalyst.degradation < 0.1)
            printf("  ║  СТАТУС: ОТЛИЧНО — узлы в норме          ║\n");
        else if (pem.degradation > 0.3)
            printf("  ║  СТАТУС: ТРЕВОГА — замените PEM-мембрану ║\n");
        else
            printf("  ║  СТАТУС: НОРМА — требуется наблюдение    ║\n");
        printf("  ╚════════════════════════════════════════════╝\n");
    }
};

// ==================== MAIN ====================
#include <windows.h>
int main() {
    SetConsoleOutputCP(65001);  // UTF-8
    SetConsoleCP(65001);
    
    AVKKV_System avkkv;
    avkkv.simulate(24.0, 0.1);  // 24 часа с шагом 0.1 сек

    
    printf("\nНажмите Enter для выхода...");
    std::cin.get();
    return 0;
}
