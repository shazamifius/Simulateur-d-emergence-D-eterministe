# Guide d'Installation Simplifié pour Windows (SED)

Ce guide vous explique comment compiler et exécuter le projet SED sur Windows. La configuration a été simplifiée pour ne dépendre que d'un compilateur C++ (`g++`), que vous avez déjà.

### Étape 1 : Vérifier votre Compilateur (Vous l'avez déjà fait !)

Le plus important est d'avoir un compilateur C++ moderne. Vous avez confirmé que la commande `g++ --version` fonctionne dans votre terminal (PowerShell ou CMD). C'est parfait.

Pour tout nouvel utilisateur, la recommandation est d'installer **MinGW-w64** (par exemple, via l'installeur de [WinLibs](https://winlibs.com/)) et de s'assurer que le dossier `mingw64\bin` est ajouté au **PATH** de Windows.

### Étape 2 : Utiliser VS Code pour Tout Faire

Le projet est maintenant configuré pour que VS Code gère tout automatiquement, sans avoir besoin de `make`.

1.  **Compiler le Projet :**
    *   Ouvrez le dossier du projet dans VS Code.
    *   Appuyez sur `Ctrl+Shift+B` (ou `Terminal > Run Build Task...`).
    *   La tâche par défaut, **`Build C++ Engine (g++ Direct)`**, va compiler `main.cpp` et `MondeSED.cpp` et créer un exécutable nommé `sed_simulator.exe`.

2.  **Lancer la Simulation et la Visualisation (Tout-en-un) :**
    *   Appuyez sur `Ctrl+Shift+P` pour ouvrir la palette de commandes.
    *   Tapez `Tasks: Run Task` et sélectionnez cette option.
    *   Choisissez la tâche **`Run & Visualize SED Project`**.

    Cette tâche va automatiquement :
    1.  Compiler le projet.
    2.  Exécuter `sed_simulator.exe` pour générer les fichiers `equilibre.csv` et `rebelle.csv`.
    3.  Lancer le script `visualiseur_3D.py` pour créer les images 3D à partir de ces fichiers.

3.  **Déboguer le Code C++ :**
    *   Ouvrez le fichier `main.cpp` ou `MondeSED.cpp`.
    *   Cliquez dans la marge à gauche d'une ligne de code pour ajouter un point d'arrêt (un cercle rouge).
    *   Appuyez sur `F5` pour lancer le débogueur. Le code se compilera et l'exécution s'arrêtera au point d'arrêt, vous permettant d'inspecter les variables.

Le projet est maintenant entièrement pilotable depuis VS Code, sans commandes manuelles complexes.