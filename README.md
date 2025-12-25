# 🔬 Projet SED : Simulateur d'Émergence Déterministe

> "Eternity is not just a concept, it compiles."

---

## 1. Objectif du Projet

Ce projet est une initiative de recherche-création visant à prouver que la complexité de la vie et de la psyché peut **émerger de lois mathématiques déterministes**. Le **SED** construit un univers où l'existence d'organismes stables est une nécessité mathématique, simulé désormais à **échelle massive** grâce au GPU.

---

## 2. Architecture "Titanic" (GPU Compute)

La version 8.0 introduit l'architecture **"Titanic"**, transformant le simulateur en un moteur massivement parallèle exécuté intégralement sur la carte graphique (GPU).

### 🚀 Nouveautés Techniques
*   **Moteur "Full GPU"** : La logique de simulation (Cellules, Synapses, Physique) est écrite en **GLSL Compute Shaders** (OpenGL 4.3).
*   **Rendu Instancié (Zero-Copy)** : Les millions de cellules sont dessinées directement depuis la mémoire GPU (SSBO) sans transfert vers le CPU, éliminant tout goulot d'étranglement.
*   **Double Horloge** : Séparation temporelle entre la boucle neurale rapide (Spikes, Potentiels) et la boucle physique lente (Métabolisme, Mouvement).
*   **Déterminisme Absolu** : Même sur GPU, la simulation reste 100% déterministe grâce à un pipeline strict (Lecture/Écriture séparées).

---

## 3. Installation et Utilisation

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

## 4. Fonctionnalités

### Contrôles
*   **Caméra Orbite** : `Clic Molette` + Souris.
*   **Panoramique** : `Shift` + `Clic Molette`.
*   **Zoom** : `Molette`.

### Visualisation
*   **Vibrant & Dynamic** : Les cellules pulsent et changent de couleur selon leur état interne (Énergie, Stress, Dette).
*   **Réseau Neuronal** : Visualisation des connexions synaptiques actives (Touche `N` ou via UI).
*   **Champs de Force** : Visualisation des gradients invisibles qui guident le mouvement (Touche `F`).

---

## 5. Philosophie

> "La simplicité est la sophistication suprême." - Leonardo da Vinci

Le SED ne code pas l'intelligence. Il code les **contraintes** (Faim, Douleur, Connexion). L'intelligence est ce qui émerge pour satisfaire ces contraintes.

---

*Auteur : Shazamifius*
*Version : 8.0 (Titanic Engine)*
