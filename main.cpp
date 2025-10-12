#include <iostream>
#include "MondeSED.h"
#include <string>

void run_simulation(const std::string& scenario_name, MondeSED& monde, int cycles) {
    std::cout << "\n--- Lancement du Scenario: " << scenario_name << " ---" << std::endl;
    monde.InitialiserMonde();

    std::cout << "Debut de la simulation..." << std::endl;
    for (int i = 0; i < cycles; ++i) {
        monde.AvancerTemps();
        if ((i + 1) % 10 == 0) {
            std::cout << "Cycle " << i + 1 << " termine." << std::endl;
        }
    }
    std::cout << "Simulation terminee." << std::endl;

    std::string output_file = scenario_name + ".csv";
    std::cout << "Exportation vers " << output_file << "..." << std::endl;
    monde.ExporterEtatMonde(output_file);
    std::cout << "Exportation terminee." << std::endl;
}

int main() {
    int size_x = 16;
    int size_y = 16;
    int size_z = 16;
    int cycles = 50;

    // --- Simulation 1: Balanced (Default Parameters) ---
    MondeSED monde_equilibre(size_x, size_y, size_z);
    run_simulation("equilibre", monde_equilibre, cycles);

    // --- Simulation 2: Rebel ---
    MondeSED monde_rebelle(size_x, size_y, size_z);
    monde_rebelle.params.K_D = 2.5f;
    monde_rebelle.params.K_E = 0.5f;
    monde_rebelle.params.SEUIL_SIMILARITE_R = 0.5f;
    run_simulation("rebelle", monde_rebelle, cycles);

    return 0;
}