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

Le projet est une **application C++ unifiée** qui intègre un moteur de simulation performant avec une interface de contrôle et de visualisation 3D en temps réel.

### A. Moteur de Simulation et Interface

*   **Langage :** **C++ Pur.** C'est le choix idéal pour garantir l'optimisation nécessaire à la gestion d'une grande matrice 3D et des calculs intensifs.
*   **Interface Graphique :** L'application utilise **raylib** pour le rendu 3D et **ImGui** pour l'interface de contrôle, offrant une expérience interactive et en temps réel.
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

Le projet a été consolidé en une seule application C++ nommée **SED-Lab**. Elle intègre la simulation, le contrôle des paramètres et la visualisation 3D en temps réel.

### A. Installation sur Windows

Pour les instructions détaillées sur la compilation et l'installation sur Windows, veuillez consulter le guide dédié :
➡️ **[Instructions pour Windows](./INSTRUCTIONS_WINDOWS.md)**

### B. Installation sur Linux (Debian/Ubuntu)

1.  **Outils de Compilation :**
    ```bash
    sudo apt-get update
    sudo apt-get install build-essential g++ cmake
    ```

2.  **Dépendances Graphiques (raylib & OpenGL) :**
    Le projet nécessite les bibliothèques de développement pour raylib et ses dépendances.
    ```bash
    sudo apt-get install libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
    ```
    Note : `raylib` sera compilé localement si `libraylib-dev` n'est pas disponible.

### C. Compilation et Lancement (avec CMake)

Le projet utilise maintenant CMake pour une compilation multiplateforme.

1.  **Créez un dossier de build :**
    ```bash
    mkdir build
    cd build
    ```

2.  **Générez les fichiers de build et compilez :**
    ```bash
    cmake ..
    make
    ```
    Cela va compiler le projet et créer un exécutable nommé `sed_lab` dans le dossier `build`.

3.  **Lancer l'application :**
    Depuis le dossier `build`, exécutez :
    ```bash
    ./sed_lab
    ```

### C. Utilisation de l'Interface

1.  **Configurer :** Utilisez le panneau de contrôle ImGui pour ajuster les paramètres de la simulation en temps réel.
2.  **Initialiser/Démarrer :**
    - Cliquez sur **"Initialiser/Réinitialiser"** pour créer un nouveau monde avec les paramètres actuels.
    - Cliquez sur **"Démarrer"** pour lancer la simulation. Vous pouvez la mettre en pause à tout moment.
3.  **Observer :** La visualisation 3D montre l'état du monde en temps réel.
    - La **couleur** des cellules représente leur énergie.
    - La **taille** des cellules représente leur charge émotionnelle.