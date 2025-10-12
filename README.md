## 🔬 Projet SED : Simulateur d'Émergence Déterministe

Ce projet est une initiative personnelle de recherche-création visant à prouver que la complexité de la vie, de la psyché, et de la stabilité peut **émerger de lois mathématiques déterministes et traçables**. Le **SED** est la construction d'un univers où l'existence d'organismes stables est une nécessité mathématique.

---

## 1. VISION DU PROJET : L'Émergence par la Loi

### A. L'Ambition Fondamentale

Mon objectif est de créer un **outil de simulation** capable de générer des entités numériques qui possèdent une "âme" faite de code. Je cherche à :

1.  **Prouver le Déterminisme :** Démontrer que les comportements complexes (la survie, le stress, l'ennui) sont le **résultat unique et prévisible** de conditions initiales (la morphologie) et d'une succession d'événements (l'histoire), sans aucune variable aléatoire.
2.  **Créer la Vie Stable :** Construire des lois si fondamentales qu'elles garantissent la création d'organismes **multicellulaires stables** qui s'auto-entretiennent et se protègent du chaos environnant.

### B. Influence Clé : La Règle et l'Émergence

Ma fondation conceptuelle se base sur l'idée de la **simplicité générant la complexité**.

| Influence | Concept Appliqué au SED |
| :--- | :--- |
| **John Horton Conway (Jeu de la Vie)** | Le SED est un **Automate Cellulaire 3D**. Il utilise des règles de transition simples, appliquées localement à chaque Voxel, pour générer des structures globales complexes. |
| **Philosophie des Systèmes (Systèmes Auto-Organisés)** | L'**intelligence** et la **stabilité** ne sont pas codées directement. Elles sont des propriétés qui **émergent** naturellement de l'interaction des cellules selon les lois définies. |

---

## 2. ARCHITECTURE TECHNIQUE

Le projet est centré sur un moteur de simulation C++ performant, avec une visualisation simple et efficace en Python.

### A. Moteur de Simulation (C++)

*   **Langage :** **C++ Pur.** C'est le choix idéal pour garantir l'optimisation nécessaire à la gestion d'une grande matrice 3D et des calculs intensifs.
*   **Structure du Monde :** **Voxel Grid 3D.** L'univers est un espace discret où chaque position est une case unique.
*   **Performance :** L'algorithme de transition est conçu pour le **parallélisme** (multi-threading avec OpenMP) afin d'utiliser la puissance maximale du processeur.

### B. Définition de la `Cellule`

L'état de chaque Cellule est défini par un ensemble de variables. Ces paramètres sont le cœur des lois de transition et sont directement représentés dans le code pour une lisibilité maximale.

| Variable | Symbole | Type | Rôle Fondamental dans le Système | Catégorie |
| :--- | :--- | :--- | :--- | :--- |
| **Énergie** | `E` | `float` | La ressource vitale. L'absence d'Énergie mène à la Mort. | **Dynamique** |
| **Dette Besoin** | `D` | `float` | Pression des besoins (Faim, Repos). Pilote le déplacement. | **Dynamique** |
| **Charge Émotionnelle** | `C` | `float` | Niveau de stress. Si trop haut, mène à la mort psychique. | **Dynamique** |
| **Dette Stimulus** | `L` | `float` | Niveau d'ennui. Force la cellule à chercher l'interaction. | **Dynamique** |
| **Âge** | `A` | `int` | Compteur de cycles. Affecte d'autres paramètres (ex: mémoire). | **Dynamique** |
| **Mémoire Énergie**| `M` | `float` | Mémorise la plus haute énergie vue dans le voisinage. | **Dynamique** |
| **Résistance Innée** | `R` | `float` | **Constante de Naissance.** Facteur "Rebelle", influence les interactions. | **Constante** |
| **Seuil Critique** | `Sc` | `float` | **Constante de Naissance.** Tolérance maximale au stress. | **Constante** |


---

## 3. VISUALISATION (Python)

Pour visualiser les résultats de la simulation, un script Python est fourni. Il lit les fichiers de données `.csv` générés par le simulateur et crée une animation 3D au format GIF.

*   **Esthétique Voxel :** Le rendu respecte la nature discrète de la grille.
*   **Visualisation de l'État :** La couleur et la taille des voxels dans le GIF sont directement liées aux états internes des cellules (Énergie et Charge Émotionnelle), rendant leur "psyché" visible.

---

## 4. COMMENT L'UTILISER

Ce projet utilise un `Makefile` pour simplifier la compilation et l'exécution.

### A. Prérequis

- Un compilateur C++ compatible C++14 (ex: `g++`).
- L'outil `make`.
- OpenMP pour la parallélisation.
- Python 3 et les bibliothèques `pandas`, `matplotlib`, `natsort`, `imageio`.

### B. Compilation

Pour compiler le simulateur, ouvrez un terminal à la racine du projet et exécutez :
```bash
make
```
Cela créera un exécutable nommé `sed_simulator`.

### C. Lancer une Simulation

L'exécutable prend les arguments suivants en ligne de commande :
```
Usage: ./sed_simulator <size_x> <size_y> <size_z> <cycles> <initial_density> <output_basename>
```
**Exemple :**
```bash
./sed_simulator 30 30 30 100 0.15 sim1
```
La simulation générera des fichiers `sim1_cycle_10.csv`, `sim1_cycle_20.csv`, etc.

### D. Générer une Visualisation Animée

Utilisez le script `visualiseur_3D.py` en lui passant le nom de base de votre simulation :
```bash
python3 visualiseur_3D.py sim1
```
Le script créera un fichier `sim1_animation.gif` dans un nouveau dossier `visualisations/`.