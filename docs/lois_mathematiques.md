# Lois Physiques et Mathématiques du SED (Spécification V2)

Ce document décrit les règles fondamentales et les formules mathématiques régissant l'univers du SED en version 2. Ces lois traduisent des concepts physiques (Champs, Thermodynamique, Gravité) en algorithmes déterministes.

---

## 1. Variables d'État (La Cellule)

Chaque cellule est une particule possédant les propriétés suivantes :

| Symbole | Nom | Domaine | Description |
| :---: | :--- | :--- | :--- |
| **E** | Énergie | $[0, \infty[$ | Ressource vitale. Mort si $E \le 0$. |
| **D** | Dette (Gravité) | $[0, \infty[$ | "Poids" ou "Faim". Augmente avec le temps. Attire la matière. |
| **C** | Stress (Pression) | $[0, 1]$ | Charge répulsive. Mort si $C > Sc$. |
| **L** | Ennui (Vide) | $[0, \infty[$ | Besoin de stimulus. Augmente naturellement. |
| **M** | Mémoire | $[0, \infty[$ | Inertie. Mémorise l'énergie max vue. Décroit avec le temps. |
| **R** | Génétique | $[0, 1]$ | Fréquence de résonance. D determining compatibilité. |
| **Sc** | Seuil Critique | $[0, 1]$ | Tolérance max au Stress. |
| **A** | Âge | $\mathbb{N}$ | Nombre de cycles vécus. |

---

## 2. Constantes et Paramètres

Ces valeurs sont configurables en temps réel via l'interface (`ParametresGlobaux`).

*   **Thermodynamique** :
    *   `K_THERMO` (0.001) : Coût énergétique par cycle.
    *   `D_PER_TICK` (0.002) : Augmentation de la dette par cycle.
    *   `L_PER_TICK` (0.001) : Augmentation de l'ennui par cycle.
    *   `TAUX_OUBLI` (0.01) : Facteur de décroissance de la mémoire ($M \leftarrow M * (1 - Taux)$).
*   **Champs** :
    *   `RAYON_DIFFUSION` (2.0) : Portée des interactions à distance.
    *   `ALPHA_ATTENUATION` (1.0) : Décroissance du champ en fonction de la distance.
*   **Mouvement** :
    *   `K_D` (1.0), `K_C` (0.5), `K_M` (0.5).
    *   `COUT_MOUVEMENT` (0.001).
*   **Reproduction** :
    *   `SEUIL_ENERGIE_DIVISION` (1.8).
    *   `COUT_DIVISION` (0.0).

---

## 3. Les Lois Fondamentales

L'évolution suit un cycle strict : **Thermodynamique $\rightarrow$ Champs/Décision $\rightarrow$ Mouvement $\rightarrow$ Reproduction $\rightarrow$ Interactions**.

### Loi 0 : Thermodynamique et Survie
Tout système tend vers le désordre.
1.  **Métabolisme** : $E \leftarrow E - K_{THERMO}$
2.  **Entropie** : $D \leftarrow D + D\_PER\_TICK$
3.  **Refroidissement** : $L \leftarrow L + L\_PER\_TICK$
4.  **Oubli** : $M \leftarrow M \times (1 - TAUX\_OUBLI)$
5.  **Mémorisation** : $M \leftarrow \max(M, \max(E_{voisins}))$
6.  **Sécurité (Clamping)** : $C, R, Sc$ sont bornés dans $[0, 1]$. $E \ge 0$.
7.  **Mort** : Si $E \le 0$ OU $C > Sc$, la cellule meurt (devient vide).

### Loi 3 : Champs (Action à Distance)
La matière influence l'espace vide autour d'elle dans un rayon $R_{diff}$.
Pour une case vide cible $v$, on calcule les potentiels :
*   **Champ d'Opportunité ($Ch_E$)** : $\sum_{n \in R} E_n \cdot e^{-\alpha \cdot dist(v, n)}$
*   **Champ de Pression ($Ch_C$)** : $\sum_{n \in R} C_n \cdot e^{-\alpha \cdot dist(v, n)}$

### Loi 1 : Dynamique (Mouvement)
Une cellule cherche à se déplacer vers la case vide voisine maximisant son **Score Vectoriel** :
$$ \text{Score} = (K_D \cdot D) - (K_C \cdot C) + (K_M \cdot \frac{M}{A+1}) - \text{Coût} + (Ch_E - Ch_C) $$
*   **Interprétation** : La cellule est attirée par sa propre faim (Gravité interne), repoussée par son stress (Pression interne), retenue par ses habitudes (Mémoire), et influencée par les champs environnants (attirée par l'énergie, repoussée par le stress).

### Loi 2 : Reproduction (Mitose) & Conservation
Si $E > \text{SEUIL\_DIVISION}$ :
*   **Division** : La cellule choisit le voisin vide maximisant la compatibilité génétique (plus haute $R$ locale).
*   **Conservation Stricte** :
    $$ E_{mère\_final} = \frac{E_{mère\_initial}}{2} - \text{Coût} $$
    $$ E_{fille} = \frac{E_{mère\_initial}}{2} $$
*   **Mutation** : $R$ et $Sc$ subissent une variation infinitésimale déterministe ($\pm 0.01$).

### Loi 4 : Osmose (Échange Énergétique)
Équilibrage des potentiels entre voisins génétiquement compatibles ($|R_1 - R_2| < \text{SEUIL}$).
*   **Flux** : $\Delta E = (E_{source} - E_{cible}) \times \text{FACTEUR}$.
*   **Stabilité** : Le flux est borné par `MAX_FLUX_ENERGIE` pour éviter les oscillations numériques.

### Loi 5 : Interaction Forte (Psychique)
Échange direct modifiant la charge interne avec le voisin le plus "calme" (plus petit $L$).
*   **Contagion de Stress** : $C \leftarrow C + (0.1 \cdot C_{voisin})$
*   **Soulagement (Ennui)** : $L \leftarrow L - (0.1 \cdot L_{voisin})$
*   *Note : Le stress augmente pour les deux, mais l'ennui diminue (interaction sociale).*
