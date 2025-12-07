# Simulateur d'Émergence Déterministe (SED) - Version 2

Bienvenue dans le **SED V2**, un simulateur de vie artificielle déterministe basé sur des lois physiques unifiées (Champs, Thermodynamique, Interaction Forte). Ce projet explore l'émergence de structures complexes (organismes, colonies) à partir de règles locales simples.

## Vue d'Ensemble

Le SED ne simule pas la biologie directement, mais une **physique numérique** où les cellules sont des particules soumises à des forces :
- **Gravité (Dette)** : La faim agit comme une force attractive.
- **Pression (Stress)** : Le stress agit comme une force répulsive thermique.
- **Champs** : La matière rayonne de l'influence dans l'espace vide (Loi 3).
- **Conservation** : L'énergie n'est jamais créée ex-nihilo (sauf initialisation), elle se conserve strictement.

## Nouveautés V2

- **Champs d'influence** : Les cellules perçoivent leur environnement à distance (Loi 3).
- **Conservation Stricte** : La reproduction divise l'énergie par 2, garantissant un système thermodynamiquement cohérent.
- **Stabilité Numérique** : Introduction de clamps (bornes) et de limites de flux pour éviter les divergences mathématiques.
- **Contrôle en Temps Réel** : Tous les paramètres (Coût métabolique, Rayon de diffusion, etc.) sont modifiables en direct.

## Guide de Démarrage

### Installation
Double-cliquez sur `start.bat` à la racine. Le script installe les dépendances (Raylib via vcpkg, CMake) et lance la simulation.

### Interface Utilisateur
- **Vue 3D** :
    - Déplacement : ZQSD + Souris (Clic Droit).
    - Clic Gauche : Sélectionner une cellule pour voir ses propriétés internes ($E, D, C, R, A, M$).
- **Panneau de Contrôle** :
    - **Contrôle** : Pause/Lecture, Pas à pas, Sauvegarde/Chargement.
    - **Configuration** : Taille de la grille, densité initiale, Seed.
    - **Paramètres** : Ajustement fin des lois physiques (Thermodynamique, Champs, Mouvement...).

## Documentation Technique

Pour comprendre les formules exactes régissant ce monde :
👉 [Voir les Lois Mathématiques (Spécification V2)](lois_mathematiques.md)

Le code source principal se trouve dans `src/MondeSED.cpp` et suit strictement ces spécifications.

## Architecture

*   `src/` : Code C++ (Logique `MondeSED` et Interface `main`).
*   `include/` : En-têtes.
*   `docs/` : Documentation.
*   `build/` : Exécutables (après compilation).

L'architecture repose sur un principe de **Double-Buffering** pour garantir le déterminisme :
1.  **Phase Réflexion** (Parallèle) : Toutes les cellules lisent l'état $T$ et proposent des actions (Mouvement, Division).
2.  **Phase Résolution** (Séquentielle/Déterministe) : Les conflits sont résolus selon des règles strictes (priorité à la dette/énergie).
3.  **Phase Action** : L'état $T+1$ est écrit.
