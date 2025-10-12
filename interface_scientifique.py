import dearpygui.dearpygui as dpg
import subprocess
import os
import time
import threading

# --- Configuration ---
SIMULATOR_EXECUTABLE = "./sed_simulator"
VISUALIZER_SCRIPT = "visualiseur_3D.py"
OUTPUT_DIR = "visualisations"
GIF_BASENAME = "simulation_gui"

def setup_gui():
    """Crée et configure la fenêtre principale de l'interface."""
    dpg.create_context()

    with dpg.window(label="Panneau de Contrôle - Simulateur SED", tag="primary_window"):
        dpg.add_text("Bienvenue dans l'interface de contrôle du Simulateur d'Émergence Déterministe.")
        dpg.add_text("Configurez les paramètres ci-dessous et lancez la simulation.")
        dpg.add_separator()

        # --- Section des Paramètres de Simulation ---
        dpg.add_text("Paramètres de la Simulation", color=(255, 255, 0))
        with dpg.group(horizontal=True):
            dpg.add_input_int(label="Taille X", default_value=16, width=100, tag="sim_size_x")
            dpg.add_input_int(label="Taille Y", default_value=16, width=100, tag="sim_size_y")
            dpg.add_input_int(label="Taille Z", default_value=16, width=100, tag="sim_size_z")
        dpg.add_input_int(label="Cycles", default_value=50, width=100, tag="sim_cycles")
        dpg.add_input_float(label="Densité Initiale", default_value=0.1, min_value=0.0, max_value=1.0, format="%.2f", tag="sim_density")
        dpg.add_separator()

        # --- Section des Paramètres des Lois (ParametresGlobaux) ---
        dpg.add_text("Paramètres des Lois", color=(255, 255, 0))
        with dpg.collapsing_header(label="Loi 1: Mouvement"):
            dpg.add_slider_float(label="K_E (Attraction Énergie)", min_value=0.0, max_value=5.0, default_value=2.0, tag="param_K_E")
            dpg.add_slider_float(label="K_D (Motivation Faim)", min_value=0.0, max_value=5.0, default_value=1.0, tag="param_K_D")
            dpg.add_slider_float(label="K_C (Aversion Stress)", min_value=0.0, max_value=5.0, default_value=0.5, tag="param_K_C")
        with dpg.collapsing_header(label="Loi 2: Division"):
            dpg.add_slider_float(label="Seuil Énergie Division", min_value=0.1, max_value=5.0, default_value=1.8, tag="param_SEUIL_ENERGIE_DIVISION")
        with dpg.collapsing_header(label="Loi 4: Échange Énergétique"):
            dpg.add_slider_float(label="Facteur Échange Énergie", min_value=0.0, max_value=0.5, default_value=0.05, format="%.3f", tag="param_FACTEUR_ECHANGE_ENERGIE")
            dpg.add_slider_float(label="Seuil Différence Énergie", min_value=0.0, max_value=1.0, default_value=0.2, tag="param_SEUIL_DIFFERENCE_ENERGIE")
            dpg.add_slider_float(label="Seuil Similarité R", min_value=0.0, max_value=1.0, default_value=0.1, tag="param_SEUIL_SIMILARITE_R")
        with dpg.collapsing_header(label="Loi 5: Interaction Psychique"):
            dpg.add_slider_float(label="Taux Augmentation Ennui", min_value=0.0, max_value=0.01, default_value=0.001, format="%.4f", tag="param_TAUX_AUGMENTATION_ENNUI")
            dpg.add_slider_float(label="Facteur Échange Psychique", min_value=0.0, max_value=0.5, default_value=0.1, tag="param_FACTEUR_ECHANGE_PSYCHIQUE")
        with dpg.collapsing_header(label="Loi 6: Mémoire"):
            dpg.add_slider_float(label="K_M (Influence Mémoire)", min_value=0.0, max_value=5.0, default_value=0.5, tag="param_K_M")
        with dpg.collapsing_header(label="Exportation"):
             dpg.add_input_int(label="Intervalle d'Export", default_value=10, min_value=1, tag="param_intervalle_export")
        dpg.add_separator()

        # --- Section de Contrôle et Visualisation ---
        dpg.add_button(label="Lancer la Simulation et Visualiser", callback=run_simulation_callback, tag="run_button")
        dpg.add_separator()
        with dpg.group(tag="visualisation_group", show=False):
            dpg.add_text("Status: En attente", tag="status_text")
            # Le GIF (ou son premier frame) sera ajouté ici dynamiquement

    dpg.create_viewport(title='Interface Scientifique SED', width=800, height=600)
    dpg.setup_dearpygui()
    dpg.show_viewport()
    dpg.set_primary_window("primary_window", True)

# --- Constants ---
CONFIG_FILE = "gui_params.conf"

def write_config_file():
    """Lit les valeurs de l'interface et les écrit dans le fichier de config."""
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
    print(f"Fichier de configuration '{CONFIG_FILE}' écrit avec succès.")

def simulation_thread():
    """Thread qui gère l'exécution des processus externes pour ne pas geler l'UI."""
    dpg.set_value("status_text", "Étape 1/3: Écriture du fichier de configuration...")
    write_config_file()
    time.sleep(1)

    # --- Lancement du simulateur C++ ---
    dpg.set_value("status_text", "Étape 2/3: Simulation C++ en cours... (cela peut prendre du temps)")
    sim_command = [
        SIMULATOR_EXECUTABLE,
        str(dpg.get_value("sim_size_x")),
        str(dpg.get_value("sim_size_y")),
        str(dpg.get_value("sim_size_z")),
        str(dpg.get_value("sim_cycles")),
        str(dpg.get_value("sim_density")),
        GIF_BASENAME,
        CONFIG_FILE
    ]
    print(f"Exécution de la commande: {' '.join(sim_command)}")
    sim_process = subprocess.run(sim_command, capture_output=True, text=True)
    if sim_process.returncode != 0:
        print("Erreur lors de l'exécution du simulateur C++:")
        print(sim_process.stderr)
        dpg.set_value("status_text", f"Erreur du simulateur C++! Voir la console.")
        dpg.hide_item("visualisation_group")
        dpg.enable_item("run_button")
        return

    # --- Lancement du visualiseur Python ---
    dpg.set_value("status_text", "Étape 3/3: Génération du GIF de visualisation...")
    vis_command = ["python3", VISUALIZER_SCRIPT, GIF_BASENAME]
    print(f"Exécution de la commande: {' '.join(vis_command)}")
    vis_process = subprocess.run(vis_command, capture_output=True, text=True)
    if vis_process.returncode != 0:
        print("Erreur lors de l'exécution du visualiseur Python:")
        print(vis_process.stderr)
        dpg.set_value("status_text", f"Erreur du visualiseur! Voir la console.")
        dpg.hide_item("visualisation_group")
        dpg.enable_item("run_button")
        return

    # --- Affichage du GIF ---
    gif_path = os.path.join(OUTPUT_DIR, f"{GIF_BASENAME}_animation.gif")
    if os.path.exists(gif_path):
        # DearPyGui ne supporte pas les GIF animés directement.
        # On affiche le premier frame comme confirmation.
        width, height, channels, data = dpg.load_image(gif_path)
        if dpg.does_item_exist("gif_texture"):
            dpg.delete_item("gif_texture")
        if dpg.does_item_exist("gif_display"):
            dpg.delete_item("gif_display")

        with dpg.texture_registry(show=False):
            dpg.add_static_texture(width, height, data, tag="gif_texture")

        dpg.add_image("gif_texture", parent="visualisation_group", tag="gif_display")
        dpg.set_value("status_text", f"Simulation terminée! GIF sauvegardé dans {gif_path}")
    else:
        dpg.set_value("status_text", "Erreur: Le fichier GIF n'a pas été trouvé après la génération.")

    dpg.enable_item("run_button")


def run_simulation_callback():
    """Fonction appelée par le bouton pour lancer tout le processus."""
    dpg.disable_item("run_button")
    dpg.show_item("visualisation_group")

    # Nettoyer l'ancienne visualisation si elle existe
    if dpg.does_item_exist("gif_display"):
        dpg.delete_item("gif_display")
    if dpg.does_item_exist("gif_texture"):
        dpg.delete_item("gif_texture")

    # Lancer le processus dans un thread séparé
    thread = threading.Thread(target=simulation_thread)
    thread.start()

def main():
    """Fonction principale pour lancer l'application."""
    setup_gui()
    while dpg.is_dearpygui_running():
        dpg.render_dearpygui_frame()
    dpg.destroy_context()

if __name__ == '__main__':
    if not os.path.exists(SIMULATOR_EXECUTABLE):
        print(f"Erreur: L'exécutable du simulateur '{SIMULATOR_EXECUTABLE}' n'a pas été trouvé.")
        print("Veuillez compiler le projet C++ avec 'make' avant de lancer cette interface.")
    else:
        main()