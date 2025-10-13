# --- Imports ---
import dearpygui.dearpygui as dpg
import subprocess
import os
import time
import threading
import glob
import sys

# Imports from visualiseur_3D.py
import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import imageio
from natsort import natsorted

# --- Global Configuration ---
SIMULATOR_EXECUTABLE = "./sed_simulator"
OUTPUT_DIR = "visualisations"
CONFIG_FILE = "sed_lab_params.conf"
SIM_BASENAME = "sed_lab_sim" # Basename for simulation output files

# =============================================================================
# SECTION 1: VISUALIZATION LOGIC (from visualiseur_3D.py)
# =============================================================================

def generate_3d_plot_frame(file_path, output_dir, max_dim, max_e, max_c):
    """
    Generates a single frame (PNG image) for a given simulation state (CSV file).
    """
    try:
        df = pd.read_csv(file_path)
    except FileNotFoundError:
        print(f"Erreur : Fichier non trouvé à {file_path}")
        return None

    base_name = os.path.basename(file_path).replace('.csv', '')
    img_path = os.path.join(output_dir, f'{base_name}_visualisation.png')

    df = df[df['E'] > 0.0]

    fig = plt.figure(figsize=(12, 12))
    ax = fig.add_subplot(111, projection='3d')

    if not df.empty:
        # Normalize colors and sizes based on global maxima for consistency
        colors = plt.cm.viridis(df['E'] / max_e)
        sizes = 10 + (df['C'] / max_c) * 50
        ax.scatter(df['x'], df['y'], df['z'], c=colors, s=sizes, alpha=0.8, marker='o')

    cycle_num = base_name.split('_')[-1]
    ax.set_title(f'Simulation SED - Cycle: {cycle_num}\n({len(df)} cellules vivantes)', fontsize=16)

    # Set consistent axis limits
    ax.set_xlim(0, max_dim + 1)
    ax.set_ylim(0, max_dim + 1)
    ax.set_zlim(0, max_dim + 1)
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')

    # Set a consistent viewing angle
    ax.view_init(elev=25, azim=45)

    # --- Add legends ---
    # Energy colorbar
    sm = plt.cm.ScalarMappable(cmap=plt.cm.viridis, norm=plt.Normalize(vmin=0, vmax=max_e))
    sm.set_array([])
    cbar = fig.colorbar(sm, ax=ax, pad=0.1, shrink=0.7)
    cbar.set_label('Énergie (E)')

    # Stress size legend
    stress_legend_handles = [
        plt.scatter([], [], s=10, c='gray', label=f'C = 0'),
        plt.scatter([], [], s=60, c='gray', label=f'C = {max_c:.2f}')
    ]
    ax.legend(handles=stress_legend_handles, loc='upper right', title='Charge (C)')

    plt.savefig(img_path)
    plt.close(fig)
    return img_path

def create_simulation_gif(basename, input_dir='.', output_dir='visualisations'):
    """
    Generates a GIF from a sequence of CSV files produced by the simulation.
    This is the full implementation.
    """
    dpg.set_value("status_text", "Visualisation: Recherche des fichiers CSV...")
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    csv_files = glob.glob(os.path.join(input_dir, f'{basename}_cycle_*.csv'))
    if not csv_files:
        dpg.set_value("status_text", f"Erreur: Aucun fichier CSV trouvé pour '{basename}'.")
        return None

    sorted_files = natsorted(csv_files)
    image_paths = []

    max_dim_global, max_energy_global, max_charge_global = 0, 0, 0
    dpg.set_value("status_text", "Visualisation: Pré-calcul des échelles...")
    print("Pré-traitement des fichiers pour déterminer les échelles globales...")
    for file_path in sorted_files:
        try:
            df = pd.read_csv(file_path)
            if not df.empty:
                max_dim_global = max(max_dim_global, df['x'].max(), df['y'].max(), df['z'].max())
                max_energy_global = max(max_energy_global, df['E'].max())
                max_charge_global = max(max_charge_global, df['C'].max())
        except Exception as e:
            print(f"Avertissement: Impossible de lire {file_path}. Erreur: {e}")

    # Ensure maxima are not zero to avoid division by zero
    if max_energy_global == 0: max_energy_global = 1.0
    if max_charge_global == 0: max_charge_global = 1.0

    print(f"Génération des images pour {len(sorted_files)} cycles...")
    for i, file_path in enumerate(sorted_files):
        img_path = generate_3d_plot_frame(file_path, output_dir, max_dim_global, max_energy_global, max_charge_global)
        if img_path:
            image_paths.append(img_path)
        dpg.set_value("status_text", f"Visualisation: Image {i+1}/{len(sorted_files)} générée.")
        print(f"  - Image {i+1}/{len(sorted_files)} générée.")


    if image_paths:
        gif_filename = f"{basename}_animation.gif"
        output_gif_path = os.path.join(output_dir, gif_filename)
        dpg.set_value("status_text", "Visualisation: Assemblage du GIF...")
        print(f"\nAssemblage du GIF... Veuillez patienter.")
        with imageio.get_writer(output_gif_path, mode='I', duration=0.2, loop=0) as writer:
            for img_path in image_paths:
                image = imageio.imread(img_path)
                writer.append_data(image)
        print(f"GIF créé : {output_gif_path}")
        # Clean up the PNG frames
        for img_path in image_paths:
            os.remove(img_path)
        return output_gif_path
    else:
        dpg.set_value("status_text", "Erreur: Aucune image générée, GIF annulé.")
        return None


# =============================================================================
# SECTION 2: GUI and SIMULATION LOGIC
# =============================================================================

def write_config_file():
    """Reads parameter values from the GUI and writes them to the config file."""
    try:
        with open(CONFIG_FILE, "w") as f:
            f.write(f"K_E={dpg.get_value('param_K_E')}\n")
            f.write(f"K_D={dpg.get_value('param_K_D')}\n")
            f.write(f"K_C={dpg.get_value('param_K_C')}\n")
            f.write(f"SEUIL_ENERGIE_DIVISION={dpg.get_value('param_SEUIL_ENERGIE_DIVISION')}\n")
            f.write(f"FACTEUR_ECHANGE_ENERGIE={dpg.get_value('param_FACTEUR_ECHANGE_ENERGIE')}\n")
            f.write(f"SEUIL_DIFFERENCE_ENERGIE={dpg.get_value('param_SEUIL_DIFFERENCE_ENERGIE')}\n")
            f.write(f"SEUIL_SIMILARITE_R={dpg.get_value('param_SEUIL_SIMILARITE_R')}\n")
            f.write(f"TAUX_AUGMENTATION_ENNUI={dpg.get_value('param_TAUX_AUGMENTATION_ENNUI')}\n")
            f.write(f"FACTEUR_ECHANGE_PSYCHIQUE={dpg.get_value('param_FACTEUR_ECHANGE_PSYCHIQUE')}\n")
            f.write(f"K_M={dpg.get_value('param_K_M')}\n")
            f.write(f"intervalle_export={dpg.get_value('param_intervalle_export')}\n")
        return True
    except Exception as e:
        print(f"Erreur lors de l'écriture du fichier de configuration : {e}")
        return False

def simulation_thread_worker():
    """
    The worker thread that runs the entire simulation and visualization pipeline.
    This prevents the GUI from freezing.
    """
    # --- Step 1: Write Config File ---
    dpg.set_value("status_text", "Étape 1/4: Écriture de la configuration...")
    if not write_config_file():
        dpg.set_value("status_text", "Erreur: Impossible d'écrire le fichier de configuration.")
        dpg.enable_item("run_button")
        return
    time.sleep(0.5)

    # --- Step 2: Compile the C++ Core ---
    dpg.set_value("status_text", "Étape 2/4: Compilation du moteur C++ (make)...")
    make_process = subprocess.run(["make"], capture_output=True, text=True)
    if make_process.returncode != 0:
        dpg.set_value("status_text", "Erreur de compilation! Vérifiez la console.")
        print("--- ERREUR DE COMPILATION MAKE ---")
        print(make_process.stderr)
        print(make_process.stdout)
        print("---------------------------------")
        dpg.enable_item("run_button")
        return
    time.sleep(0.5)

    # --- Step 3: Run the C++ Simulator ---
    dpg.set_value("status_text", "Étape 3/4: Simulation C++ en cours...")
    sim_command = [
        SIMULATOR_EXECUTABLE,
        str(dpg.get_value("sim_size_x")),
        str(dpg.get_value("sim_size_y")),
        str(dpg.get_value("sim_size_z")),
        str(dpg.get_value("sim_cycles")),
        str(dpg.get_value("sim_density")),
        SIM_BASENAME,
        CONFIG_FILE
    ]
    print(f"Exécution de la commande: {' '.join(sim_command)}")
    sim_process = subprocess.run(sim_command, capture_output=True, text=True)
    if sim_process.returncode != 0:
        dpg.set_value("status_text", "Erreur du simulateur C++! Vérifiez la console.")
        print("--- ERREUR DU SIMULATEUR C++ ---")
        print(sim_process.stderr)
        print(sim_process.stdout)
        print("---------------------------------")
        dpg.enable_item("run_button")
        return

    # --- Step 4: Generate Visualization ---
    dpg.set_value("status_text", "Étape 4/4: Génération de la visualisation...")
    gif_path = create_simulation_gif(basename=SIM_BASENAME, output_dir=OUTPUT_DIR)

    # --- Final Step: Display Results ---
    if gif_path and os.path.exists(gif_path):
        # DearPyGui doesn't support animated GIFs, so we show the first frame.
        width, height, channels, data = dpg.load_image(gif_path)

        with dpg.texture_registry(show=False):
            dpg.add_static_texture(width, height, data, tag="gif_texture")

        dpg.add_image("gif_texture", parent="visualisation_group", tag="gif_display")
        dpg.set_value("status_text", f"Terminé! GIF sauvegardé dans {gif_path}")
    else:
        dpg.set_value("status_text", "Erreur: Le fichier GIF n'a pas été généré.")
        dpg.show_item("viz_placeholder") # Show placeholder again on error

    dpg.enable_item("run_button")

def run_full_pipeline_callback():
    """Callback function for the main 'Run' button."""
    dpg.disable_item("run_button")

    # Clean up previous results and hide placeholder
    if dpg.does_item_exist("gif_display"):
        dpg.delete_item("gif_display")
    if dpg.does_item_exist("gif_texture"):
        dpg.delete_item("gif_texture")
    dpg.hide_item("viz_placeholder")

    # Run the entire process in a separate thread to keep the GUI responsive
    thread = threading.Thread(target=simulation_thread_worker)
    thread.start()

def setup_gui():
    """Creates and configures the main GUI window with a custom theme."""
    dpg.create_context()

    # --- Custom Theme ---
    with dpg.theme() as global_theme:
        with dpg.theme_component(dpg.mvAll):
            # Colors
            dpg.add_theme_color(dpg.mvThemeCol_WindowBg, (20, 20, 20), category=dpg.mvThemeCat_Core)
            dpg.add_theme_color(dpg.mvThemeCol_ChildBg, (25, 25, 25), category=dpg.mvThemeCat_Core)
            dpg.add_theme_color(dpg.mvThemeCol_Button, (50, 50, 100), category=dpg.mvThemeCat_Core)
            dpg.add_theme_color(dpg.mvThemeCol_ButtonHovered, (70, 70, 130), category=dpg.mvThemeCat_Core)
            dpg.add_theme_color(dpg.mvThemeCol_ButtonActive, (40, 40, 90), category=dpg.mvThemeCat_Core)
            dpg.add_theme_color(dpg.mvThemeCol_FrameBg, (40, 40, 40), category=dpg.mvThemeCat_Core)
            dpg.add_theme_color(dpg.mvThemeCol_TitleBgActive, (50, 50, 100), category=dpg.mvThemeCat_Core)
            dpg.add_theme_color(dpg.mvThemeCol_Header, (50, 50, 100), category=dpg.mvThemeCat_Core)
            dpg.add_theme_color(dpg.mvThemeCol_HeaderHovered, (70, 70, 130), category=dpg.mvThemeCat_Core)
            # Styling
            dpg.add_theme_style(dpg.mvStyleVar_FrameRounding, 5, category=dpg.mvThemeCat_Core)
            dpg.add_theme_style(dpg.mvStyleVar_WindowRounding, 5, category=dpg.mvThemeCat_Core)
            dpg.add_theme_style(dpg.mvStyleVar_ChildRounding, 5, category=dpg.mvThemeCat_Core)
            dpg.add_theme_style(dpg.mvStyleVar_GrabRounding, 5, category=dpg.mvThemeCat_Core)
            dpg.add_theme_style(dpg.mvStyleVar_WindowTitleAlign, 0.5, 0.5, category=dpg.mvThemeCat_Core)

    dpg.bind_theme(global_theme)

    # --- Main Window Layout ---
    with dpg.window(label="SED-Lab", tag="primary_window"):
        with dpg.group(horizontal=True):
            # Left Panel: Controls
            with dpg.child_window(width=400):
                dpg.add_text("Configuration de la Simulation", color=(230, 230, 0))
                dpg.add_separator()
                dpg.add_text("Taille de la Grille")
                with dpg.group(horizontal=True):
                    dpg.add_input_int(label="X", default_value=16, width=90, tag="sim_size_x")
                    dpg.add_input_int(label="Y", default_value=16, width=90, tag="sim_size_y")
                    dpg.add_input_int(label="Z", default_value=16, width=90, tag="sim_size_z")
                dpg.add_input_int(label="Cycles de Simulation", default_value=50, tag="sim_cycles")
                dpg.add_input_float(label="Densité Initiale", default_value=0.1, min_value=0.0, max_value=1.0, format="%.2f", tag="sim_density")
                dpg.add_input_int(label="Intervalle d'Export CSV", default_value=10, min_value=1, tag="param_intervalle_export")
                dpg.add_separator()

                with dpg.collapsing_header(label="Paramètres Avancés des Lois"):
                    dpg.add_slider_float(label="K_E (Attraction Énergie)", min_value=0.0, max_value=5.0, default_value=2.0, tag="param_K_E")
                    dpg.add_slider_float(label="K_D (Motivation Faim)", min_value=0.0, max_value=5.0, default_value=1.0, tag="param_K_D")
                    dpg.add_slider_float(label="K_C (Aversion Stress)", min_value=0.0, max_value=5.0, default_value=0.5, tag="param_K_C")
                    dpg.add_slider_float(label="Seuil Énergie Division", min_value=0.1, max_value=5.0, default_value=1.8, tag="param_SEUIL_ENERGIE_DIVISION")
                    dpg.add_slider_float(label="Facteur Échange Énergie", min_value=0.0, max_value=0.5, default_value=0.05, format="%.3f", tag="param_FACTEUR_ECHANGE_ENERGIE")
                    dpg.add_slider_float(label="Seuil Différence Énergie", min_value=0.0, max_value=1.0, default_value=0.2, tag="param_SEUIL_DIFFERENCE_ENERGIE")
                    dpg.add_slider_float(label="Seuil Similarité R", min_value=0.0, max_value=1.0, default_value=0.1, tag="param_SEUIL_SIMILARITE_R")
                    dpg.add_slider_float(label="Taux Augmentation Ennui", min_value=0.0, max_value=0.01, default_value=0.001, format="%.4f", tag="param_TAUX_AUGMENTATION_ENNUI")
                    dpg.add_slider_float(label="Facteur Échange Psychique", min_value=0.0, max_value=0.5, default_value=0.1, tag="param_FACTEUR_ECHANGE_PSYCHIQUE")
                    dpg.add_slider_float(label="K_M (Influence Mémoire)", min_value=0.0, max_value=5.0, default_value=0.5, tag="param_K_M")

            # Right Panel: Visualization and Status
            with dpg.child_window(width=-1): # -1 fills remaining space
                dpg.add_text("Statut & Visualisation")
                dpg.add_separator()
                dpg.add_button(label="Lancer le Cycle Complet", callback=run_full_pipeline_callback, tag="run_button", width=-1, height=40)
                dpg.add_separator()
                with dpg.group(tag="visualisation_group", show=True):
                    dpg.add_text("Status: Prêt", tag="status_text")
                    dpg.add_spacer(height=10)
                    # The GIF's first frame will be dynamically added here.
                    dpg.add_text("La visualisation apparaîtra ici après la simulation.", tag="viz_placeholder")

    dpg.create_viewport(title='SED-Lab | Simulateur d\'Émergence Déterministe', width=1000, height=720)
    dpg.setup_dearpygui()
    dpg.show_viewport()
    dpg.set_primary_window("primary_window", True)

def main():
    """Main function to launch the SED-Lab application."""
    # A check to ensure the C++ files are there, compilation is handled by the app
    if not os.path.exists("app/main.cpp"):
        print("Erreur: Le répertoire 'app' ou le fichier 'main.cpp' est manquant.")
        print("Veuillez vous assurer que la structure du projet C++ est correcte.")
        return

    setup_gui()
    while dpg.is_dearpygui_running():
        dpg.render_dearpygui_frame()
    dpg.destroy_context()

if __name__ == '__main__':
    main()