
## 🔬 Projet SED : Simulateur d'Émergence Déterministe

Ce projet est une initiative de recherche-création visant à prouver que la complexité de la vie, de la psyché, et de la stabilité peut **émerger de lois mathématiques déterministes et traçables**. Le **SED** est la construction d'un univers où l'existence d'organismes stables est une nécessité mathématique.

---

## 1. VISION DU PROJET : L'Émergence par la Loi

### A. L'Ambition Fondamentale

Notre objectif est de créer un **outil universel de simulation** capable de générer des entités numériques qui possèdent une "âme" faite de code. Nous cherchons à :

1.  **Prouver le Déterminisme :** Démontrer que les comportements complexes (la survie, le stress, l'ennui) sont le **résultat unique et prévisible** de conditions initiales (la morphologie) et d'une succession d'événements (l'histoire), sans aucune variable aléatoire.
2.  **Créer la Vie Stable :** Construire des lois si fondamentales qu'elles garantissent la création d'organismes **multicellulaires stables** qui s'auto-entretiennent et se protègent du chaos environnant.

### B. Influence Clé : La Règle et l'Émergence

Notre fondation conceptuelle se base sur l'idée de la **simplicité générant la complexité**.

| Influence | Concept Appliqué au SED |
| :--- | :--- |
| **John Horton Conway (Jeu de la Vie)** | Le SED est un **Automate Cellulaire 3D**. Il utilise des règles de transition simples, appliquées localement à chaque Voxel, pour générer des structures globales complexes (les entités stables). |
| **Philosophie des Systèmes (Systèmes Auto-Organisés)** | L'**intelligence** et la **stabilité** ne sont pas codées directement. Elles sont des propriétés qui **émergent** naturellement de l'interaction des milliards de cellules selon nos lois. |

---

## 2. EXIGENCES TECHNIQUES & OPTIMISATION (Phase I : Le Moteur C++)

Le moteur de calcul (la "Soupe Primordiale") est la priorité absolue et sera séparé du moteur graphique.

### A. Architecture du Moteur

* **Langage :** **C++ Pur.** C'est le seul choix pour garantir l'**optimisation en béton** nécessaire à la gestion de la grande matrice 3D et des calculs matriciels complexes.
* **Structure du Monde :** **Voxel Grid 3D (Matrice 3D).** L'univers est un espace discret où chaque position est une case unique. **La taille de la grille (Volume)** sera définie et modulée par la suite en fonction des capacités optimales de la machine, sans valeur fixe initialement.
* **Performance :** L'algorithme de transition doit être conçu pour le **parallélisme** (multi-threading) afin d'utiliser la puissance maximale du processeur.

### B. Définition Mathématique de la `Cellule`

L'état de chaque Cellule est défini par sept paramètres. Ces variables sont le cœur de nos lois de transition.

| Variable | Type | Rôle Fondamental dans le Système | Catégorie |
| :--- | :--- | :--- | :--- |
| **$E$ (Énergie)** | `float` | La ressource vitale. L'absence d'Énergie mène à la Mort. | **Dynamique** |
| **$D$ (Dette Besoin)** | `float` | Pression des besoins fondamentaux (Faim, Repos). Pilote le déplacement. | **Dynamique** |
| **$C$ (Charge Émotionnelle)** | `float` | Niveau d'activation (Stress/Peur). Si trop haut, mène au comportement erratique ou à l'autodestruction. | **Dynamique** |
| **$L$ (Dette Stimulus)** | `float` | Niveau d'ennui. Force la cellule à chercher l'interaction (la "communication"). | **Dynamique** |
| **$A$ (Âge)** | `int` | Compteur de cycles. Utilise l'âge pour affecter l'efficacité des autres paramètres (affaiblissement). | **Dynamique** |
| **$R$ (Résistance Innée)** | `float` | **Constante de Naissance.** Mesure l'influence du voisinage sur la cellule (le facteur "Rebelle"). | **Constante** |
| **$S_c$ (Seuil Critique)** | `float` | **Constante de Naissance.** Tolérance maximale au stress. Définit la vulnérabilité psychique. | **Constante** |

---

## 3. PROJECTION FINALE : L'Œuvre Scientifique

### A. Le Rendu (Unreal Engine)

Le moteur Unreal Engine sera utilisé pour la **visualisation brute** des données C++.

* **Esthétique Voxel :** Le rendu sera basé sur la **géométrie discrète (Voxel)** de la grille pour respecter la nature mathématique du système.
* **Visualisation de la Psyché :** Les Voxels ne seront pas lisses (*smooth shading*). La couleur, la luminosité et les effets d'échelles de chaque Voxel seront directement pilotés par les valeurs de $C$ (Charge Émotionnelle) et $E$ (Énergie) pour rendre l'**état psychique instantané** visible.

### B. L'Héritage

Le produit final est une **simulation interactive** qui permet à l'utilisateur de modifier les Constantes $R$ et $S_c$ (les facteurs morphologiques de naissance) et d'observer en temps réel comment l'évolution de la **Soupe Primordiale** crée une espèce stable différente à chaque fois.

Ce projet est une quête de la **Loi de la Vie** dans sa forme mathématique la plus pure.
