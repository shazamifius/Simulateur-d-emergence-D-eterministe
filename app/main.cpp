#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "MondeSED.h"

// --- Helper function to print usage instructions ---
void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " <size_x> <size_y> <size_z> <cycles> <initial_density> <output_basename> [config_file]" << std::endl;
    std::cerr << "  <size_x>, <size_y>, <size_z>: Dimensions of the world grid (integers)." << std::endl;
    std::cerr << "  <cycles>: Number of simulation cycles to run (integer)." << std::endl;
    std::cerr << "  <initial_density>: Probability (0.0 to 1.0) for a cell to be alive at start." << std::endl;
    std::cerr << "  <output_basename>: Base name for the output CSV files (string)." << std::endl;
    std::cerr << "  [config_file]: (Optional) Path to a configuration file for simulation parameters." << std::endl;
}

// --- Main application entry point ---
int main(int argc, char* argv[]) {
    // --- Argument Parsing ---
    if (argc < 7 || argc > 8) {
        print_usage(argv[0]);
        return 1;
    }

    int size_x, size_y, size_z, cycles;
    float initial_density;
    std::string output_basename;
    std::string config_file = "";

    try {
        size_x = std::stoi(argv[1]);
        size_y = std::stoi(argv[2]);
        size_z = std::stoi(argv[3]);
        cycles = std::stoi(argv[4]);
        initial_density = std::stof(argv[5]);
        output_basename = argv[6];

        if (argc == 8) {
            config_file = argv[7];
        }

        if (size_x <= 0 || size_y <= 0 || size_z <= 0 || cycles <= 0 || initial_density < 0.0f || initial_density > 1.0f) {
            throw std::invalid_argument("Invalid argument value.");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    // --- Simulation Setup & Execution ---
    std::cout << "--- Initialisation du Simulateur d'Émergence Déterministe ---" << std::endl;
    std::cout << "Paramètres:" << std::endl;
    std::cout << "  - Taille du monde: " << size_x << "x" << size_y << "x" << size_z << std::endl;
    std::cout << "  - Cycles: " << cycles << std::endl;
    std::cout << "  - Densité initiale: " << initial_density * 100 << "%" << std::endl;
    std::cout << "  - Fichier de sortie: " << output_basename << "_cycle_*.csv" << std::endl;

    try {
        MondeSED monde(size_x, size_y, size_z);

        // Load parameters from file if provided
        if (!config_file.empty()) {
            monde.ChargerParametresDepuisFichier(config_file);
        }

        // Pass the density to the initialization method
        monde.InitialiserMonde(initial_density);

        std::cout << "\nDebut de la simulation..." << std::endl;
        for (int i = 0; i < cycles; ++i) {
            monde.AvancerTemps(); // La bibliothèque avance d'un pas

            // La logique d'export est maintenant gérée par l'application
            if (monde.params.intervalle_export > 0 && (i + 1) % monde.params.intervalle_export == 0) {
                std::cout << "Exporting state at cycle " << i + 1 << "..." << std::endl;
                monde.ExporterEtatMonde(output_basename);
            }

            // Log de progression
            if ((i + 1) % 10 == 0 || i == cycles - 1) {
                std::cout << "Cycle " << i + 1 << "/" << cycles << " termine." << std::endl;
            }
        }
        std::cout << "Simulation terminee." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred during simulation: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}