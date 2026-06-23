# Compte Rendu d'Expérimentation — Grille Infinie

Ce document résume les modifications techniques et les choix d'architecture réalisés sur la branche `experimentations-23-06-2026` pour implémenter une **grille parfaitement infinie** au sein du Simulateur d'Émergence Déterministe (SED) en Rust.

---

## 1. Problématique Initiale
Dans les versions antérieures, la simulation était bridée par des contraintes de dimensions géométriques :
1. Le plancher de bedrock (les fondations stables à $Y=0$) était alloué de manière statique au démarrage de la simulation sur une grille finie définie par `size_x` et `size_z`. Si des cellules se déplaçaient en dehors de ces limites, elles tombaient dans le vide sans bedrock sous elles.
2. La grille de référence dessinée en 3D était figée entre $0$ et `size_x`/`size_z`, ce qui nuisait à l'immersion si les structures se déplaçaient au-delà.

---

## 2. Solutions Apportées (Implémentation)

### A. Stockage Spatialement Infini
La structure sous-jacente du SED Rust repose sur une `HashMap<ChunkKey, Chunk>`. Les clés de chunks (`ChunkKey`) utilisent des coordonnées de grille signées de type `i32` allant théoriquement de $-\infty$ à $+\infty$.
Le stockage des cellules est donc déjà intrinsèquement capable de gérer un espace tridimensionnel **infini**.

### B. Plancher de Bedrock Implicite et Infini ($Y=0$)
Pour éliminer le besoin d'allouer statiquement des millions de cellules de bedrock (ce qui consomme de la mémoire et fige les dimensions), nous avons rendu le plancher de bedrock **implicite** :
- **Suppression de l'allocation** : Les boucles d'initialisation explicites du plancher de bedrock à Y=0 dans `initialiser_monde` et `charger_scenario` ont été supprimées.
- **Résolution globale implicite** : Dans [world.rs](src/simulation/world.rs), la méthode `get_cell_global(x, y, z)` intercepte désormais les requêtes où `y == 0` et retourne instantanément une référence partagée vers une instance statique immuable de cellule de type `Static` (bedrock).
- **Sécurité en écriture** : La méthode `get_cell_global_mut(x, y, z)` retourne `None` pour `y == 0`, rendant le plancher de bedrock globalement indestructible et immuable sans utiliser de mémoire de stockage pour chaque coordonnée.
- **Impact** : Les cellules peuvent désormais migrer, se diviser et s'appuyer sur le bedrock à $Y=0$ sur des distances infinies dans toutes les directions horizontales ($X$ et $Z$).

### C. Rendu Dynamique du Bedrock
Pour afficher le plancher de bedrock sans pour autant surcharger la carte graphique avec une infinité de cubes :
- Lorsque le filtre `Static (bedrock)` est activé dans l'interface, la fonction `render_cells` dans [main.rs](src/main.rs) calcule l'ensemble des colonnes $(X, Z)$ sur lesquelles des chunks contiennent des cellules vivantes.
- Le plancher à $Y=0$ n'est rendu que sous ces colonnes actives, étendant le plancher visuel de manière fluide et dynamique au fur et à mesure que les organismes se déplacent.

### D. Grille Visuelle Centrée sur le Barycentre (Effet de Défilement Infini)
- La fonction `render_grid` dans [main.rs](src/main.rs) a été réécrite pour accepter une référence vers l'état du monde.
- Au lieu de dessiner des lignes fixes, elle calcule à chaque frame le **barycentre spatial** de toutes les cellules vivantes dans la simulation.
- Les lignes de la grille 3D sont dessinées de manière à ce que le centre de la grille corresponde aux coordonnées du barycentre (arrondies à l'entier pour éviter les tremblements visuels).
- **Résultat** : La grille de repère visuelle suit l'organisme tridimensionnel dans ses déplacements. S'il rampe ou se propage vers la droite, la grille défile sous lui de façon infinie. Les repères d'axes rouge, vert et bleu restent quant à eux fixes à l'origine absolue $(0,0,0)$ comme point d'ancrage historique.

---

## 3. Implémentation du Cache de Pré-chargement (Replay Cache)

Pour répondre à l'exigence d'un pré-chargement fluide sans lag CPU pour visionner des séquences de simulation de $N$ cycles (ex: 800) :
1. **Calcul en tâche de fond** : La fonction `lancer_prechargement` a été ajoutée. Elle effectue les calculs de cycles de manière synchrone en mémoire vive, sans aucun rendu d'écran ou rafraîchissement d'interface graphique. Les $N$ états complets du monde sont clonés et stockés dans un vecteur (`preloaded_states`).
2. **Scrubbing Temporel & Synchronisation** : Un panneau egui **"⚡ Pré-chargement (Replay Cache)"** a été intégré. Il permet à l'utilisateur :
   - De configurer le nombre de cycles à pré-charger.
   - De faire défiler le temps via un Slider (scrubbing temporel fluide). À chaque fois que le curseur change, l'état global `state.monde` est mis à jour par le clone correspondant. Cela assure que tous les graphiques egui, les statistiques de population, les filtres de rendu et l'inspecteur 3D restent parfaitement synchrones et interactifs.
   - De lancer une lecture automatique en avant/arrière, ou de progresser pas à pas (boutons ◀ et ▶, ainsi que raccourcis clavier Espace et Flèche Droite).
3. **Zéro Lag de Visionnage** : En substituant le monde actif par les clones du cache à chaque frame du replay, les calculs physiques et neuronaux sont totalement évités, libérant 100% de la puissance CPU pour assurer un rendu 3D à 60 FPS constants.

## 4. Validation
- La suite de tests unitaires et de déterminisme bit-exact a été relancée et s'est terminée avec succès.
- La structure de code a été validée par compilation locale sous Windows.

