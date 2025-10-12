import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import os
import sys

def visualize_3d_state(file_path, output_dir='visualisations'):
    """
    Reads a SED simulation state file (CSV) and generates a 3D scatter plot.
    Color represents Energy (E), Size represents Emotional Charge (C).
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    try:
        df = pd.read_csv(file_path)
    except FileNotFoundError:
        print(f"Erreur : Fichier non trouvé à {file_path}")
        return

    base_name = os.path.basename(file_path).replace('.csv', '')

    df = df[df['E'] > 0.0]
    if df.empty:
        print(f"Avertissement : Aucune cellule vivante à visualiser dans {file_path}")
        return

    colors = plt.cm.RdYlGn(df['E'] / df['E'].max())

    C_max = df['C'].max() if df['C'].max() > 0 else 1.0
    sizes = 5 + (df['C'] / C_max) * 40

    X_max, Y_max, Z_max = df['x'].max(), df['y'].max(), df['z'].max()

    fig = plt.figure(figsize=(12, 12))
    ax = fig.add_subplot(111, projection='3d')

    scatter = ax.scatter(
        df['x'], df['y'], df['z'],
        c=colors,
        s=sizes,
        alpha=0.7,
        marker='o'
    )

    ax.set_xlabel('X Coordinate')
    ax.set_ylabel('Y Coordinate')
    ax.set_zlabel('Z Coordinate')
    ax.set_title(f'Visualisation SED : {base_name.upper()} ({len(df)} cellules vivantes)')

    max_dim = max(X_max, Y_max, Z_max) + 1
    ax.set_xlim(0, max_dim)
    ax.set_ylim(0, max_dim)
    ax.set_zlim(0, max_dim)

    sm = plt.cm.ScalarMappable(cmap=plt.cm.RdYlGn, norm=plt.Normalize(vmin=0, vmax=1))
    sm.set_array([])
    cbar = fig.colorbar(sm, ax=ax, pad=0.05)
    cbar.set_label('Énergie (E) [0 = Rouge, 1 = Vert]')

    stress_legend_handles = [
        ax.scatter([], [], [], s=5, c='gray', alpha=0.7, label='Calme (C=Min)'),
        ax.scatter([], [], [], s=45, c='gray', alpha=0.7, label='Stress (C=Max)')
    ]
    ax.legend(handles=stress_legend_handles, loc='upper right')

    ax.view_init(elev=20, azim=45)

    output_filename = os.path.join(output_dir, f'{base_name}_visualisation.png')
    plt.savefig(output_filename)
    plt.close(fig)
    print(f"Visualisation sauvegardée dans : {output_filename}")

if __name__ == '__main__':
    if len(sys.argv) > 1:
        files_to_visualize = sys.argv[1:]
    else:
        files_to_visualize = ['equilibre.csv', 'rebelle.csv']

    print("--- Outil Visualisateur 3D du Projet SED ---")
    for f in files_to_visualize:
        if os.path.exists(f):
            visualize_3d_state(f)
        else:
            print(f"Avertissement : Le fichier {f} n'a pas été trouvé. Assurez-vous que le moteur C++ l'a exporté.")