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

Le projet est centré sur un moteur de simulation C++ performant, avec une interface de contrôle et de visualisation unifiée en Python.

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

## 3. COMMENT UTILISER SED-LAB

Le projet a été unifié dans une seule application : **SED-Lab**. Elle gère la configuration, la compilation, l'exécution et la visualisation de manière transparente.

### A. Installation des Prérequis

#### Pour Windows

1.  **Installer MSYS2 & le Compilateur C++:**
    - Téléchargez et installez **MSYS2** depuis [msys2.org](https://www.msys2.org/).
    - Ouvrez le terminal **MSYS2 MinGW 64-bit** et exécutez `pacman -Syu` pour mettre à jour, puis `pacman -S --needed base-devel mingw-w64-x86_64-toolchain make` pour installer les outils de compilation.

2.  **Installer les Dépendances Python:**
    - Assurez-vous d'avoir Python 3. Ouvrez une invite de commandes (CMD) ou PowerShell et exécutez :
      ```bash
      pip install pandas matplotlib natsort imageio dearpygui
      ```

#### Pour Linux / macOS

1.  **Installer les Outils de Compilation:**
    - Utilisez votre gestionnaire de paquets (`apt`, `yum`, `brew`, etc.) pour installer `g++`, `make`, `python3` et `pip`.

2.  **Installer les Dépendances Python:**
    ```bash
    pip install pandas matplotlib natsort imageio dearpygui
    ```

### B. Lancement de SED-Lab

Il n'est plus nécessaire de compiler manuellement le projet. L'application s'en charge pour vous.

1.  **Ouvrez un terminal** à la racine du projet.
    - *Note pour Windows :* Utilisez un terminal standard (CMD ou PowerShell), **pas** le terminal MSYS2, pour lancer l'application Python.
2.  **Exécutez la commande suivante :**
    ```bash
    python3 sed_lab.py
    ```

### C. Utilisation de l'Interface

1.  **Configurer :** Utilisez les panneaux de gauche pour ajuster les paramètres de la simulation et les constantes des lois physiques.
2.  **Lancer :** Cliquez sur le bouton **"Lancer le Cycle Complet"**.
3.  **Observer :** L'application va automatiquement :
    - **Compiler** le moteur C++ en arrière-plan.
    - **Exécuter** la simulation avec les paramètres choisis.
    - **Générer** l'animation GIF à partir des données de sortie.
    - **Afficher** un aperçu de l'animation directement dans l'interface.

Le GIF final est sauvegardé dans le dossier `visualisations/`.