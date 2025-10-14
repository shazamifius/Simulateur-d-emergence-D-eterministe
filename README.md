# 🔬 Projet SED : Simulateur d'Émergence Déterministe

---

## 1. Objectif du Projet

Ce projet est une initiative personnelle de recherche-création visant à prouver que la complexité de la vie, de la psyché, et de la stabilité peut **émerger de lois mathématiques déterministes et traçables**. Le **SED** est la construction d'un univers où l'existence d'organismes stables est une nécessité mathématique.

---

## 2. Vision du Projet : L'Émergence par la Loi

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

## 3. Architecture Technique

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

## 4. Comment Utiliser SED-Lab

### A. Installation sur Windows

Pour les instructions détaillées sur la compilation et l'installation sur Windows, veuillez consulter le guide dédié :
➡️ **[Instructions pour Windows](./INSTRUCTIONS_WINDOWS.md)**

### B. Installation sur Linux (Debian/Ubuntu)

La méthode recommandée utilise le gestionnaire de paquets `vcpkg` pour une gestion des dépendances fiable et multi-plateforme.

1.  **Prérequis : Outils de Compilation**
    Assurez-vous d'avoir les outils de build essentiels, Git, et les bibliothèques de développement requises par le backend graphique de raylib.
    ```bash
    sudo apt-get update
    sudo apt-get install build-essential g++ cmake git libxinerama-dev libxcursor-dev xorg-dev libglu1-mesa-dev pkg-config
    ```

2.  **Installation des Dépendances avec vcpkg**
    `vcpkg` simplifie l'installation de bibliothèques C++ comme `raylib`.

    a. **Clonez le dépôt de vcpkg :**
       Nous recommandons un chemin simple, par exemple `~/vcpkg`.
       ```bash
       git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
       ```

    b. **Installez vcpkg :**
       ```bash
       cd ~/vcpkg
       ./bootstrap-vcpkg.sh
       ```

    c. **Installez raylib :**
       Cette commande va télécharger, compiler et installer `raylib`. Cela peut prendre plusieurs minutes.
       ```bash
       ./vcpkg install raylib
       ```

3.  **Compilation du Projet**
    a. **Naviguez vers le dossier du projet :**
       ```bash
       cd /chemin/vers/votre/projet/SED-Lab
       ```
    b. **Créez un dossier de build :**
       ```bash
       mkdir build
       cd build
       ```

    c. **Configurez le projet avec CMake :**
       Cette commande indique à CMake où trouver les bibliothèques installées par `vcpkg`.
       ```bash
       cmake .. -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake
       ```
       *(Adaptez le chemin si vous avez installé vcpkg ailleurs.)*

    d. **Compilez le projet :**
       ```bash
       cmake --build .
       ```
       L'exécutable `sed_lab` sera créé dans le dossier `build`.

### C. Lancement et Utilisation

1.  **Lancer l'application :**
    Depuis le dossier de build, lancez l'exécutable.
    ```bash
    ./sed_lab
    ```

2.  **Utilisation de l'Interface :**
    - **Configurer :** Utilisez le panneau de contrôle ImGui pour ajuster les paramètres de la simulation en temps réel.
    - **Initialiser/Démarrer :**
        - Cliquez sur **"Initialiser/Réinitialiser"** pour créer un nouveau monde avec les paramètres actuels.
        - Cliquez sur **"Démarrer"** pour lancer la simulation. Vous pouvez la mettre en pause à tout moment.
    - **Observer :** La visualisation 3D montre l'état du monde en temps réel.
        - La **couleur** des cellules représente leur énergie.
        - La **taille** des cellules représente leur charge émotionnelle.
