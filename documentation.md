# Documentation Technique du Simulateur d'Émergence Déterministe (SED)

---

## 1. Architecture et Compilation

### 1.1. Fichiers Sources
L'exécutable `sed_lab` est compilé à partir des fichiers suivants :
- **`app/main.cpp`**: Point d'entrée de l'application, gestion de la fenêtre, de la boucle principale, de l'interface utilisateur (UI) et du rendu 3D.
- **`src/MondeSED.cpp`**: Implémentation de la logique de simulation, y compris les lois physiques et comportementales des cellules.
- **`include/MondeSED.h`**: Fichier d'en-tête définissant les structures de données (`Cellule`, `ParametresGlobaux`) et l'interface de la classe `MondeSED`.
- **`imgui/`**: Contient les fichiers sources de la bibliothèque ImGui et son backend pour Raylib (`rlImGui`), utilisés pour construire l'interface graphique.

### 1.2. Processus de Compilation
- **Outil :** Le projet utilise **CMake** pour la configuration de la compilation. Le fichier `CMakeLists.txt` à la racine définit les cibles et les dépendances.
- **Exécutable :** La compilation génère un unique exécutable nommé **`sed_lab`**.

### 1.3. Dépendances Externes
Le projet repose sur trois bibliothèques externes principales :
- **Raylib :** Utilisée comme backend graphique pour la création de la fenêtre, le rendu 3D (sphères pour les cellules, grille) et la gestion des entrées utilisateur (souris, clavier).
- **ImGui :** Utilisée pour créer l'interface utilisateur (UI) de l'application. Tous les panneaux, boutons, sliders et graphiques sont gérés par ImGui.
- **OpenMP :** Utilisée pour la parallélisation des boucles intensives dans le moteur de simulation, notamment la phase de décision (`AvancerTemps`). Cela permet d'accélérer significativement les calculs en les répartissant sur plusieurs cœurs de processeur.

---

## 2. Moteur de Simulation : `MondeSED`

### 2.1. Rôle de la Classe `MondeSED`
La classe `MondeSED` est le cœur de la simulation. Elle a les responsabilités suivantes :
- **Gérer la Grille :** Elle contient une grille 3D (`std::vector<Cellule>`) qui représente l'univers de la simulation. L'accès à une cellule spécifique se fait via un index 1D pour des raisons de performance (`getIndex`).
- **Orchestrer le Cycle de Vie :** Elle contient la méthode `AvancerTemps()` qui exécute un cycle complet de simulation en appliquant les lois dans un ordre strict pour garantir le déterminisme.
- **Stocker l'État :** Elle maintient l'état global de la simulation, comme le numéro du cycle actuel (`cycle_actuel`) et la graine de génération (`current_seed`).
- **Gérer les Paramètres :** Elle possède une instance de `ParametresGlobaux` permettant de modifier les constantes des lois en temps réel.

### 2.2. Le Cycle de Simulation (`AvancerTemps`)
La méthode `AvancerTemps()` est conçue pour garantir un comportement déterministe, même en utilisant le parallélisme. Chaque cycle est divisé en trois phases distinctes :

- **Phase 1 : Décision (Lecture seule, Parallèle)**
  1. Une copie complète de la grille (`read_grid`) est créée.
  2. Le programme parcourt chaque cellule de la grille en parallèle (`#pragma omp parallel for`).
  3. Pour chaque cellule, les fonctions `AppliquerLoi...` de décision (Mouvement, Division, etc.) sont appelées.
  4. Ces fonctions lisent l'état du monde **uniquement** à partir de `read_grid`.
  5. Les actions décidées (par exemple, un mouvement souhaité) sont stockées dans des vecteurs temporaires (`mouvements_souhaites`, `divisions_souhaitees`, etc.).

- **Phase 2 : Action (Écriture, Séquentielle)**
  1. Les conflits potentiels sont résolus. Par exemple, si deux cellules veulent se déplacer vers la même case, une règle déterministe (celle avec la plus haute "Dette de Besoin") décide laquelle l'emporte.
  2. Les actions validées sont appliquées séquentiellement à la grille principale (`grille`). Cette phase n'est pas parallélisée pour éviter les "race conditions" lors de l'écriture.

- **Phase 3 : Mise à Jour de l'État (Écriture, Parallèle)**
  1. La fonction `AppliquerLoiZero` est appelée pour chaque cellule en parallèle.
  2. Cette loi gère les changements d'état passifs et internes à la cellule (consommation d'énergie, vieillissement, mémorisation) et vérifie les conditions de mort.

### 2.3. Gestion de l'État
La classe `MondeSED` fournit des méthodes pour sauvegarder et charger l'état complet de la simulation, permettant de suspendre et de reprendre des expériences.

- **Sauvegarde (`SauvegarderEtat`)**: Écrit le contenu de la grille et les métadonnées (taille, cycle actuel, paramètres) dans un fichier binaire (`.sed`).
- **Chargement (`ChargerEtat`)**: Lit un fichier binaire (`.sed`) et restaure l'état complet de la simulation, y compris la position et l'état de chaque cellule.

---

## 3. Analyse Détaillée des Lois de Simulation

### 3.1. Loi 0 : Survie et Métabolisme (`AppliquerLoiZero`)
Cette loi est appliquée à la fin de chaque cycle. Elle gère les changements d'état passifs et les conditions de mort.
- **Condition d'activation :** La cellule doit être vivante (`is_alive == true`).
- **Logique :**
  1. **Mémorisation (Loi 6) :** La cellule scanne ses voisins et met à jour sa variable `M` avec la plus haute valeur d'énergie (`E`) trouvée dans son voisinage.
  2. **Métabolisme :** Les valeurs internes de la cellule sont mises à jour :
     - `E -= 0.001` (Consommation d'énergie)
     - `D += 0.002` (Augmentation de la dette de besoin)
     - `L += params.TAUX_AUGMENTATION_ENNUI` (Augmentation de la dette de stimulus/ennui)
     - `A++` (Vieillissement)
  3. **Conditions de Mort :** La cellule meurt (`is_alive` passe à `false`) si l'une des conditions suivantes est remplie :
     - `E <= 0` (Mort par manque d'énergie)
     - `C > Sc` (Mort par surcharge de stress/émotionnelle)

### 3.2. Loi 1 : Mouvement (`AppliquerLoiMouvement`)
Cette loi détermine si une cellule doit se déplacer et où.
- **Condition d'activation :** La cellule doit être vivante.
- **Logique de décision :**
  1. La cellule évalue toutes les cases voisines **vides** (`is_alive == false`).
  2. Pour chaque case vide, un **Score d'Attractivité** est calculé selon la formule :
     `score = (K_D * D_cellule) - (K_C * C_cellule) + (K_M * (M_cellule / (A_cellule + 1)))`
  3. La cellule choisit la case qui maximise ce score.
- **Résolution de conflit :** Si plusieurs cellules choisissent la même case de destination, seule celle ayant la valeur de `D` (Dette de Besoin) la plus élevée est autorisée à se déplacer. Les autres restent sur place.
- **Conséquence de l'action :** La cellule est copiée vers la case de destination, et la case d'origine est vidée.

### 3.3. Loi 2 : Division (`AppliquerLoiDivision`)
Cette loi gère la reproduction cellulaire.
- **Condition d'activation :**
  - La cellule doit être vivante.
  - L'énergie de la cellule `E` doit être supérieure à `params.SEUIL_ENERGIE_DIVISION`.
  - Au moins une case voisine doit être vide.
- **Logique de décision :**
  1. La cellule évalue toutes les cases voisines **vides**.
  2. Elle choisit la case vide qui possède la plus haute valeur de `R` (Résistance Innée). Ce mécanisme favorise la croissance de groupes de cellules génétiquement similaires.
- **Résolution de conflit :** Si plusieurs cellules ciblent la même case pour se diviser, seule celle ayant la valeur d'énergie `E` la plus élevée gagne.
- **Conséquence de l'action :**
  1. L'énergie `E` de la cellule mère est divisée par deux.
  2. La cellule fille est créée dans la case cible, héritant des propriétés de la mère.
  3. Les champs `R` et `Sc` de la cellule fille subissent une **mutation déterministe**, calculée à partir des coordonnées de la case et de l'âge de la cellule.

### 3.4. Loi 4 : Échange Énergétique (`AppliquerLoiEchange`)
Cette loi permet aux cellules de partager des ressources.
- **Condition d'activation :** La cellule doit être vivante.
- **Logique de décision :**
  1. La cellule évalue tous ses voisins **vivants**.
  2. Un échange est planifié si **toutes** les conditions suivantes sont remplies :
     - La similarité génétique est suffisante : `abs(source.R - voisin.R) < params.SEUIL_SIMILARITE_R`.
     - La différence d'énergie est significative : `source.E - voisin.E > params.SEUIL_DIFFERENCE_ENERGIE`.
- **Conséquence de l'action :** Un montant d'énergie, proportionnel à la différence d'énergie (`diff_energie * params.FACTEUR_ECHANGE_ENERGIE`), est transféré de la cellule source vers la cellule voisine.

### 3.5. Loi 5 : Interaction Psychique (`AppliquerLoiPsychisme`)
Cette loi modélise les interactions sociales basées sur l'ennui et le stress.
- **Condition d'activation :** La cellule doit être vivante.
- **Logique de décision :**
  1. La cellule recherche le voisin vivant ayant la plus faible valeur de `L` (Dette de Stimulus).
- **Conséquence de l'action :**
  1. Une interaction est planifiée avec le voisin trouvé.
  2. Lors de l'application, le stress (`C`) des **deux** cellules augmente, tandis que l'ennui (`L`) des **deux** cellules diminue. La magnitude du changement est proportionnelle aux valeurs de `C` et `L` de la cellule initiatrice.

---

## 4. Application Principale et Interface Utilisateur (`sed_lab`)

### 4.1. Boucle Principale
Le fichier `app/main.cpp` contient la boucle `while (!WindowShouldClose())` qui constitue le cœur de l'application. À chaque itération, elle effectue les actions suivantes :
1.  **Avancement de la simulation :** Si la simulation est en cours (`simulation_running`), appelle `monde->AvancerTemps()` une ou plusieurs fois (selon `sim_cycles_per_frame`).
2.  **Gestion des entrées :** Traite les interactions de l'utilisateur, comme la sélection de cellules et les mouvements de caméra.
3.  **Rendu :** Appelle les fonctions de dessin pour la scène 3D (`BeginMode3D`) et l'interface utilisateur (`rlImGuiBegin`).

### 4.2. Visualisation 3D (`Draw3DVisualization`)
Cette fonction est responsable du rendu de l'état du monde dans la vue 3D.
- **Représentation des Cellules**: Chaque cellule vivante est représentée par une sphère. Les propriétés visuelles de la sphère sont mappées à partir de l'état de la cellule :
  - La **couleur** est mappée sur l'**Énergie (`E`)** de la cellule (Bleu pour une faible énergie, Rouge pour une haute énergie).
  - La **taille (rayon)** est mappée sur la **Charge Émotionnelle (`C`)** de la cellule.
- **Indicateur de Sélection**: Si une cellule est sélectionnée, un contour filaire blanc est dessiné autour d'elle.

### 4.3. Interface Utilisateur (`DrawUI`)
Cette fonction utilise ImGui pour dessiner tous les panneaux de contrôle.
- **Panneau de Contrôle**:
  - **Onglet "Contrôle"**: Fournit les boutons pour `Initialiser/Réinitialiser`, `Démarrer/Pause`, et avancer d'un `Step`. Affiche également les statistiques de la simulation (cycle, nombre de cellules) et un graphique de l'historique du nombre de cellules.
  - **Onglet "Configuration"**: Permet de définir les dimensions de la grille, la densité initiale et la graine de génération avant de lancer une nouvelle simulation.
  - **Onglet "Paramètres"**: Expose les champs de la structure `ParametresGlobaux` via des sliders, permettant de modifier en temps réel les constantes qui régissent les lois de la simulation.
- **Inspecteur de Cellule**: Une fenêtre qui s'affiche lorsqu'une cellule est sélectionnée, montrant en détail toutes les valeurs de ses champs (`E`, `D`, `C`, etc.).

### 4.4. Contrôles et Interactions
- **Sélection de Cellule (Ray Picking)**: Le clic gauche de la souris lance un rayon dans la scène 3D. Le code teste l'intersection de ce rayon avec la sphère de chaque cellule pour déterminer laquelle a été cliquée.
- **Contrôles de la Caméra**: La caméra 3D peut être manipulée comme suit :
  - **Orbite**: Clic molette maintenu.
  - **Panoramique**: `Shift` + Clic molette maintenu.
  - **Zoom**: Molette de la souris.

---

## 5. Annexe : Structures de Données

### 5.1. `struct Cellule`
Cette structure contient l'état complet d'une seule unité (Voxel) dans la grille du monde.

| Champ | Type | Description |
| :--- | :--- | :--- |
| `R` | `float` | **Résistance Innée :** Propriété génétique immuable, utilisée pour la similarité lors des échanges et le choix de cible pour la division. |
| `Sc`| `float` | **Seuil Critique :** Seuil de tolérance au stress. Si `C > Sc`, la cellule meurt. |
| `E` | `float` | **Énergie :** Ressource vitale, consommée à chaque cycle et nécessaire à la division. |
| `D` | `float` | **Dette de Besoin :** Représente la "faim" ou la motivation interne à se déplacer. |
| `C` | `float` | **Charge Émotionnelle :** Niveau de stress, qui augmente lors des interactions psychiques et influence la décision de mouvement. |
| `L` | `float` | **Dette de Stimulus :** Représente "l'ennui", motive la cellule à initier des interactions psychiques. |
| `M` | `float` | **Mémoire :** Stocke la plus haute valeur d'énergie vue dans le voisinage. |
| `A` | `int`   | **Âge :** Nombre de cycles depuis la naissance de la cellule. |
| `is_alive`| `bool` | **État de vie :** `true` si la cellule est vivante, `false` sinon. |

### 5.2. `struct ParametresGlobaux`
Cette structure regroupe toutes les constantes qui peuvent être ajustées via l'interface utilisateur pour modifier le comportement de la simulation.

| Champ | Description |
| :--- | :--- |
| `K_D` | Poids de la **Dette de Besoin (`D`)** dans le calcul du score de mouvement. |
| `K_C` | Poids de la **Charge Émotionnelle (`C`)** (facteur d'aversion) dans le calcul du score de mouvement. |
| `K_M` | Poids de la **Mémoire (`M`)** dans le calcul du score de mouvement. |
| `SEUIL_ENERGIE_DIVISION`| Énergie minimale requise pour qu'une cellule puisse se diviser. |
| `FACTEUR_ECHANGE_ENERGIE`| Pourcentage de la différence d'énergie transféré lors d'un échange. |
| `SEUIL_DIFFERENCE_ENERGIE`| Différence d'énergie minimale requise pour déclencher un échange. |
| `SEUIL_SIMILARITE_R`| Similarité génétique (`R`) minimale requise pour un échange d'énergie. |
| `TAUX_AUGMENTATION_ENNUI`| Vitesse à laquelle la **Dette de Stimulus (`L`)** augmente à chaque cycle. |
| `FACTEUR_ECHANGE_PSYCHIQUE`| Intensité de l'échange (gain de `C`, perte de `L`) lors d'une interaction psychique. |
| `intervalle_export` | Fréquence (en cycles) à laquelle l'état du monde est exporté en CSV (non utilisé dans l'UI actuelle). |
