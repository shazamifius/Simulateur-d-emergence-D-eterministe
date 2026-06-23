# Conception Mathématique et Architecture de Hashlife appliqué au SED

Ce document présente l'étude de faisabilité, la modélisation mathématique (en $\LaTeX$) et les structures de données requises pour implémenter l'algorithme **Hashlife** adapté aux spécificités tridimensionnelles, physiques et neurales du Simulateur d'Émergence Déterministe (SED).

---

## 1. Rappel de l'algorithme Hashlife standard
L'algorithme Hashlife (Bill Gosper, 1984) repose sur deux piliers :
1. **Quadtree** : Représentation récursive de l'espace en 2D où chaque nœud de niveau $k$ (de taille $2^k \times 2^k$) est composé de 4 sous-nœuds de niveau $k-1$.
2. **Mémoïsation (Table de Hachage)** : Pour chaque nœud de niveau $k$, on calcule et on met en cache l'état du sous-nœud central de niveau $k-1$ après $2^{k-2}$ générations.

---

## 2. Adaptation Tridimensionnelle : L'Octree de Hashlife (3D)

Dans le SED, l'espace est en 3D. Le Quadtree est donc remplacé par un **Octree**.

### Définition d'un Nœud ($Node_k$)
Un nœud de niveau $k$ représente un cube de taille $2^k \times 2^k \times 2^k$ voxels.
Pour $k > 0$, un $Node_k$ est défini par :
$$Node_k = \{C_{000}, C_{001}, C_{010}, C_{011}, C_{100}, C_{101}, C_{110}, C_{111}\}$$
Où chaque $C_{xyz}$ est un pointeur vers un sous-nœud $Node_{n-1}$.

Pour $k = 0$, le nœud représente une cellule individuelle (feuille de l'arbre) contenant l'état complet de la cellule du SED.

---

## 3. Problématique des États Continus : Hachage & Discrétisation

### A. La dérive des nombres à virgule flottante (Floating-point drift)
Le SED utilise des variables de type `f32` (énergie $E$, stress $C$, potentiel $P$, poids synaptiques $W$, etc.). En raison des erreurs d'arrondi de l'arithmétique IEEE 754, deux configurations de cellules mathématiquement identiques peuvent présenter d'infimes variations binaires, empêchant la collision de hachage nécessaire à la mémoïsation.

### B. Solution : La Discrétisation Temporelle pour le Hachage (Quantization)
Pour résoudre ce problème sans perdre le déterminisme ni altérer les calculs réels, nous appliquons une fonction de quantification $\mathcal{Q}$ lors de la génération de la clé de hachage du nœud feuille ($Node_0$). Les valeurs flottantes réelles sont stockées dans les cellules pour les calculs de transition, mais la clé de hachage du nœud utilise des valeurs entières discrétisées.

Soit une variable flottante $v \in [v_{min}, v_{max}]$. Sa représentation discrète sur $b$ bits $q_v \in \mathbb{N}$ est :
$$q_v = \text{round}\left( \frac{v - v_{min}}{v_{max} - v_{min}} \times (2^b - 1) \right)$$

Pour le SED, nous recommandons la répartition de précision suivante pour la clé du nœud :
- **Type Cellulaire ($T$)** : $2$ bits (exact, valeurs $\{0, 1, 2, 3\}$).
- **Énergie ($E$)** : $16$ bits (plage $[0, 20.0]$).
- **Dette ($D$) et Stress ($C$)** : $12$ bits chacun.
- **Potentiel Neural ($P$)** : $12$ bits (plage $[-1.0, 1.0]$).
- **Poids Synaptiques ($W$)** : $8$ bits par synapse (27 synapses $\rightarrow 216$ bits).

La clé de hachage d'un nœud de niveau $k > 0$ est générée en combinant les identifiants uniques de ses 8 sous-nœuds :
$$\text{Hash}(Node_k) = \text{SHA-256}(ID(C_{000}) \parallel ID(C_{001}) \parallel \dots \parallel ID(C_{111}))$$

---

## 4. Problématique du Rayon d'Interaction non-local ($R > 1$)

Dans le Jeu de la Vie de Conway, la transition d'une cellule ne dépend que de ses voisins immédiats (rayon $R=1$).
Dans le SED, la fonction `calculer_champs_locaux` dépend du paramètre `rayon_diffusion` ($R \approx 2.0$ ou $3.0$).

### Algorithme de Réduction de Bordure (Border Reduction)
Soit $R$ le rayon d'interaction maximal de la physique du SED.
Pour calculer l'état futur d'un cube de taille $L \times L \times L$, nous avons besoin d'une bordure de taille $R$ autour de ce cube.

Dans Hashlife standard, pour un nœud de taille $2^k$, on calcule le centre de taille $2^{k-1}$ après $2^{k-2}$ étapes.
Pour supporter un rayon $R > 1$, la taille minimale du nœud de niveau $k$ pour lequel on peut calculer un pas de temps futur $T$ doit satisfaire :
$$2^k - 2R \ge 2^{k-1}$$
$$2^{k-1} \ge 2R \implies k \ge \log_2(4R)$$

Pour $R = 2.0$, le niveau minimal $k$ pour appliquer la récursion temporelle est $k=3$ (taille $8 \times 8 \times 8$).

---

## 5. Architecture de Caching pour le Replay fluide (800 Steps)

Pour permettre à l'utilisateur de calculer 800 cycles en arrière-plan à vitesse maximale (Hashlife), puis de les revisionner sans aucun lag en 3D, nous mettons en place le **Replay Buffer de Snapshots**.

```
+------------------+                    +--------------------+
|  Moteur Hashlife | -- (Snapshot) --> | Ring Buffer        |
|  (Calcul GPU/CPU)|                    | Vector de 800 etats|
+------------------+                    +--------------------+
                                                  |
                                                  v
                                        +--------------------+
                                        | Visualisation 3D   |
                                        | (Replay direct)    |
                                        +--------------------+
```

### Structure du Cache Temporel
À chaque étape temporelle $t$ calculée par la macro-étape de Hashlife, l'état complet du monde est extrait et stocké sous une forme compressée :
```rust
struct CompressedWorldState {
    cycle: i32,
    // Stockage creux des cellules vivantes uniquement
    cells: Vec<(i32, i32, i32, Cell)>, 
}
```

Pendant la phase de **calcul** :
- Le moteur Hashlife calcule à une vitesse exponentielle les états $t \in [0, 800]$.
- Il pousse les structures `CompressedWorldState` dans un `Vec<CompressedWorldState>` pré-alloué de taille 800.

Pendant la phase de **Replay** :
- La simulation physique et le raycasting sont mis en pause.
- Le moteur graphique lit simplement l'index $i$ dans le vecteur pour dessiner instantanément les cellules sans aucun calcul métabolique ou neural, garantissant un rendu à 60 FPS constants.

---

## 6. Prévention des Plantages (Garbage Collection de la Table de Hachage)

L'un des plus grands risques de Hashlife tridimensionnel est la **consommation de mémoire RAM** en raison du stockage d'une infinité de nœuds dans la table de hachage.

### Stratégie de Nettoyage (GC)
Pour éviter le débordement de mémoire (out of memory crash) lors d'un calcul de 800 cycles :
1. **Compteur de Références** : Chaque nœud dans l'octree utilise un compteur de références (via des pointeurs de type `Rc` ou `Arc`).
2. **Nettoyage LRU (Least Recently Used)** : Lorsque la table de hachage dépasse un seuil de mémoire critique (ex: 2 Go), une phase de nettoyage parcourt la table de hachage et supprime tous les nœuds dont le compteur de références est égal à 1 (c'est-à-dire les nœuds qui ne font plus partie de l'état actuel ou immédiatement futur du monde).
