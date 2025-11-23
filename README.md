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

### B. Cycle de Vie de la Simulation (Ordre des Opérations)

Le déterminisme du simulateur repose sur un ordre d'exécution strict des lois à chaque cycle (`AvancerTemps`). Ce cycle se déroule en trois phases principales pour éviter les conflits d'accès et garantir que toutes les décisions sont basées sur l'état du monde au même instant `t`.

1.  **Phase 1 : Décision (Parallèle)**
    *   Une copie en lecture seule de la grille est créée.
    *   Toutes les cellules évaluent leur environnement et prennent leurs décisions **en même temps** sur la base de cette copie.
    *   Les actions souhaitées (mouvement, division, échanges) sont enregistrées dans des listes temporaires.

2.  **Phase 2 : Action (Séquentielle)**
    *   Les conflits (ex: deux cellules visant la même case) sont résolus de manière déterministe.
    *   Les actions validées sont appliquées à la grille principale.

3.  **Phase 3 : Mise à Jour de l'État (Parallèle)**
    *   Les lois passives comme le vieillissement, la consommation d'énergie et la mémorisation sont appliquées à toutes les cellules.
    *   Les conditions de mort sont vérifiées.

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

2.  **Interface et Fonctionnalités Principales**
    Le SED-Lab est conçu pour l'expérimentation interactive.

    *   **Contrôles de la Simulation :**
        *   **Initialiser/Réinitialiser :** Crée un nouveau monde en utilisant les paramètres de taille, densité et graine définis dans l'onglet "Configuration".
        *   **Démarrer/Pause :** Lance ou arrête l'écoulement du temps.
        *   **Step :** Fait avancer la simulation d'un seul cycle, pour une analyse pas à pas.
        *   **Gestion de la Graine :** Dans l'onglet "Configuration", vous pouvez fixer une graine pour la reproductibilité ou en utiliser une aléatoire.

    *   **Visualisation et Analyse :**
        *   **Inspecteur de Cellule :** **Cliquez sur n'importe quelle cellule** dans la vue 3D pour ouvrir une fenêtre affichant toutes ses statistiques en temps réel. C'est l'outil principal pour comprendre les comportements locaux.
        *   **Graphique d'Historique :** Le panneau de contrôle principal affiche un graphique de l'évolution du nombre de cellules vivantes, permettant de visualiser la dynamique globale de la simulation.
        *   **Légende Visuelle :** La **couleur** des cellules représente leur Énergie (`E`), tandis que leur **taille** représente leur Charge Émotionnelle (`C`).

    *   **Sauvegarde et Chargement :**
        *   Les boutons **"Sauvegarder"** et **"Charger"** permettent de sauvegarder l'état complet de la simulation dans un fichier `simulation_state.sed` et de le recharger plus tard pour reprendre une expérience.

    *   **Navigation 3D :**
        *   **Orbite :** Maintenez le **clic molette** et déplacez la souris.
        *   **Panoramique :** Maintenez `Shift` + **clic molette** et déplacez la souris.
        *   **Zoom :** Utilisez la **molette** de la souris.

---

## 5. Structure du Code

Pour les développeurs souhaitant comprendre ou contribuer au projet, voici une vue d'ensemble des fichiers clés :

*   `app/main.cpp`: C'est le **point d'entrée de l'application**. Il gère la création de la fenêtre, l'initialisation de `raylib` et `ImGui`, la boucle principale de rendu, et les interactions de l'utilisateur (clavier, souris). Il contient également le code de l'interface graphique (UI).

*   `include/MondeSED.h`: Le **fichier d'en-tête principal de la simulation**. Il définit la structure des `Cellule`, les `ParametresGlobaux`, et l'interface publique de la classe `MondeSED`. C'est le meilleur endroit pour comprendre quelles données sont stockées et quelles actions peuvent être effectuées sur le monde de la simulation.

*   `src/MondeSED.cpp`: L'**implémentation de la logique de simulation**. Ce fichier contient le "cœur" du projet, y compris l'implémentation détaillée de toutes les lois mathématiques qui régissent le comportement des cellules (mouvement, division, interaction, etc.).
