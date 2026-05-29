#include "avkkv_common.h"
#include <iostream>
#include <sys/stat.h>

#define OUTPUT_DIR "output/"

void generate_010_confusor(const char* filename);
void generate_020_injector(const char* filename);
void generate_030_mixer(const char* filename);
void generate_040_cyclone(const char* filename);
void generate_050_turbine(const char* filename);
void generate_060_bunker(const char* filename);
void generate_070_fuelcell(const char* filename);

void ensure_output_dir() {
    #ifdef _WIN32
        mkdir(OUTPUT_DIR);
    #else
        mkdir(OUTPUT_DIR, 0755);
    #endif
}

int main() {
    std::cout << "=== AVKKV GEOMETRY GENERATOR ===\n";
    ensure_output_dir();
    
    generate_010_confusor(OUTPUT_DIR "010_confusor.stl");
    generate_020_injector(OUTPUT_DIR "020_injector.stl");
    generate_030_mixer(OUTPUT_DIR "030_mixer.stl");
    generate_040_cyclone(OUTPUT_DIR "040_cyclone.stl");
    generate_050_turbine(OUTPUT_DIR "050_turbine.stl");
    generate_060_bunker(OUTPUT_DIR "060_bunker.stl");
    generate_070_fuelcell(OUTPUT_DIR "070_fuelcell.stl");
    
    std::cout << "\nDone! Files in " OUTPUT_DIR "\n";
    return 0;
}
