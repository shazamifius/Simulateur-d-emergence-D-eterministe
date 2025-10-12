import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import os
import sys
import glob
import imageio
from natsort import natsorted

def create_simulation_gif(basename, input_dir='.', output_dir='visualisations', gif_filename='simulation.gif'):
    """
    Generates a GIF from a sequence of CSV files produced by the simulation.
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Find all CSV files for the given basename
    csv_files = glob.glob(os.path.join(input_dir, f'{basename}_cycle_*.csv'))
    if not csv_files:
        print(f"Erreur: Aucun fichier CSV trouvé pour le nom de base '{basename}' dans le dossier '{input_dir}'.")
        print("Veuillez d'abord lancer une simulation avec le simulateur C++.")
        return

    # Sort files naturally (e.g., cycle_10 after cycle_2)
    sorted_files = natsorted(csv_files)

    image_paths = []

    # --- Determine global limits for consistent visualization ---
    max_dim_global = 0
    max_energy_global = 0
    max_charge_global = 0

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
        print(f"  - Image {i+1}/{len(sorted_files)} générée.")

    # Create GIF from the generated images
    if image_paths:
        output_gif_path = os.path.join(output_dir, gif_filename)
        print(f"\nAssemblage du GIF... Veuillez patienter.")
        with imageio.get_writer(output_gif_path, mode='I', duration=0.2, loop=0) as writer:
            for img_path in image_paths:
                image = imageio.imread(img_path)
                writer.append_data(image)
        print(f"--- Visualisation GIF créée avec succès ---")
        print(f"Fichier sauvegardé ici : {output_gif_path}")
    else:
        print("Aucune image n'a été générée, le GIF n'a pas pu être créé.")

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

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("--- Outil Visualisateur 3D du Projet SED ---")
        print("Cet outil génère un GIF animé à partir des fichiers CSV d'une simulation.")
        print("\nUsage: python visualiseur_3D.py <nom_de_base_simulation>")
        print("  <nom_de_base_simulation>: Le nom de base utilisé lors du lancement du simulateur C++.")
        print("\nExemple: Si vous avez lancé './sed_simulator 10 10 10 50 0.2 ma_sim',")
        print("         exécutez 'python visualiseur_3D.py ma_sim'")
        sys.exit(1)

    simulation_basename = sys.argv[1]
    gif_name = f"{simulation_basename}_animation.gif"

    create_simulation_gif(basename=simulation_basename, gif_filename=gif_name)