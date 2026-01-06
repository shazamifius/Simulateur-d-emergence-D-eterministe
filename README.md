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

## 3. Architecture Technique V8 (CPU Edition)

Le projet repose sur une architecture **CPU Massivement Parallèle**, optimisée pour garantir un déterminisme bit-exact.

### A. Moteur "Full CPU" Déterministe
*   **Langage :** C++17 Standard.
*   **Parallélisme :** **OpenMP** est utilisé pour distribuer la charge sur tous les coeurs du processeur.
*   **Déterminisme :** Architecture stricte en 3 phases (Lecture / Décision / Écriture) avec tri déterministe des intentions et gestion explicite des threads pour assurer une reproductibilité parfaite (Bit-Exact) quel que soit le matériel.
*   **Double Horloge :** Séparation temporelle entre la boucle neurale rapide (Spikes) et la boucle physique lente (Mouvement).

### B. Rendu Optimisé
L'application utilise **raylib** et **ImGui** pour une visualisation 3D interactive, découplée du moteur de simulation.

---

## 4. Installation et Utilisation

### A. Windows (Recommandé) ✨

Nous avons créé un installateur universel qui gère tout (Dépendances, Compilation, Configuration).

1.  **Prérequis** :
    *   **Visual Studio** (2019 ou 2022) avec "Développement Desktop C++".
    *   **Git** installé.
2.  **Installation en 1 Clic** :
    *   Lancez le fichier `setup_all.bat`.
    *   Le script va automatiquement :
        *   Télécharger et compiler `vcpkg`.
        *   Installer `raylib`, `imgui`, et `rlimgui`.
        *   Compiler le projet en mode Release.
        *   Créer un lanceur `start_simulation.bat`.
3.  **Lancement** :
    *   Double-cliquez sur `start_simulation.bat`.

### B. Linux (Debian/Ubuntu)

Utilisez CMake et votre gestionnaire de paquets habituel :

```bash
# 1. Dépendances système
sudo apt-get install build-essential git cmake libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev

# 2. Cloner et Construire
git clone https://github.com/VOTRE_USER/Simulateur-d-emergence-D-eterministe.git
cd Simulateur-d-emergence-D-eterministe
mkdir build && cd build
cmake ..
cmake --build .
./sed_lab
```

---

*Auteur : Shazamifius*
*Version : 8.0 (CPU Reference Implementation)*
