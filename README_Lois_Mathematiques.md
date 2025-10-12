# Documentation des Lois Mathématiques du SED

Ce document détaille l'intention philosophique et la formulation mathématique de chaque loi régissant le Simulateur d'Émergence Déterministe (SED). Chaque loi est conçue pour être simple, déterministe et contrôlable via des paramètres globaux, permettant ainsi l'étude de l'émergence de la complexité à partir de règles fondamentales.

---

### Loi 0 : Survie, Vieillissement et Mort

*   **Intention :** Établir les besoins fondamentaux de toute entité : la consommation d'énergie pour exister et la vulnérabilité à la mort. Cette loi introduit la pression de survie de base.
*   **Règle Déterministe :**
    1.  **Consommation & Vieillissement :** À chaque cycle, pour une cellule vivante (`is_alive == true`):
        *   `E -= 0.001` (Consommation d'énergie)
        *   `D += 0.002` (Augmentation de la dette de besoin)
        *   `L += TAUX_AUGMENTATION_ENNUI` (Augmentation de la dette de stimulus)
        *   `A += 1` (Vieillissement)
    2.  **Mort Énergétique :** La cellule meurt si `E <= 0`.
    3.  **Mort Psychique :** La cellule meurt si `C > Sc`.
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   `TAUX_AUGMENTATION_ENNUI` : Contrôle la vitesse à laquelle l'ennui (`L`) augmente.

---

### Loi 1 : Mouvement par Recherche d'Équilibre

*   **Intention :** Simuler un comportement où une cellule se déplace pour soulager une pression interne (faim, stress), et non par hasard. Le mouvement est une décision calculée.
*   **Règle Déterministe :**
    1.  Une cellule vivante évalue toutes les cases voisines vides.
    2.  Pour chaque case vide, elle calcule un **Score d'Attractivité ($S_A$)**.
    3.  La formule de score inclut la mémoire (voir Loi 6): `S_A = (K_E * E_voisin) + (K_D * D_cellule) - (K_C * C_cellule) + (K_M * (M_cellule / (A_cellule + 1)))`.
    4.  La cellule choisit de se déplacer vers la case qui maximise ce score.
    5.  **Gestion des conflits :** Si plusieurs cellules visent la même case, seule celle avec la `D` (Dette de Besoin) la plus élevée gagne le droit de se déplacer.
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   `K_E` : Poids de l'attraction vers l'énergie.
    *   `K_D` : Poids de la motivation due à la faim (`D`).
    *   `K_C` : Poids de l'aversion au stress (`C`).
    *   `K_M` : Poids de l'influence de la mémoire (`M`).

---

### Loi 2 : Division Cellulaire et Croissance

*   **Intention :** Permettre la croissance et la formation de structures multicellulaires. La division favorise la cohésion génétique.
*   **Règle Déterministe :**
    1.  Une cellule se divise si `E > SEUIL_ENERGIE_DIVISION` et qu'il y a au moins une case voisine vide.
    2.  **Choix de la cible :** La cellule choisit la case voisine vide qui a la plus haute `R` (Résistance Innée), favorisant la création de "tissus" de cellules génétiquement similaires.
    3.  **Mécanisme :** L'énergie `E` de la mère est divisée par deux. La fille hérite des propriétés de la mère, mais ses gènes `R` et `Sc` subissent une mutation déterministe.
    4.  **Gestion des conflits :** Si plusieurs cellules visent la même case pour se diviser, celle avec l'`E` (Énergie) la plus élevée l'emporte.
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   `SEUIL_ENERGIE_DIVISION` : Le seuil d'énergie requis pour la reproduction.

---

### Loi 3 : Mort Psychique (Autodestruction)

*   **Intention :** Démontrer que le stress a un impact fatal. Une cellule peut s'autodétruire par surcharge émotionnelle.
*   **Règle Déterministe :**
    *   Intégrée dans la Loi 0, une cellule meurt si `C > Sc`.
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   Le `Sc` (Seuil Critique) est une propriété de la cellule, mais les facteurs qui augmentent `C` influencent indirectement cette loi.

---

### Loi 4 : Échange Énergétique et Stabilité des Tissus

*   **Intention :** Permettre aux organismes de survivre en tant qu'entité cohésive en partageant leurs ressources.
*   **Règle Déterministe :**
    1.  Une cellule évalue ses voisins vivants.
    2.  Elle ne peut échanger qu'avec des voisins génétiquement similaires (`abs(R_cellule - R_voisin) < SEUIL_SIMILARITE_R`).
    3.  Si sa propre énergie est significativement plus élevée (`E_cellule - E_voisin > SEUIL_DIFFERENCE_ENERGIE`), elle transfère une fraction de la différence.
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   `FACTEUR_ECHANGE_ENERGIE` : Le pourcentage de la différence d'énergie à transférer.
    *   `SEUIL_DIFFERENCE_ENERGIE` : Le seuil à partir duquel l'échange est déclenché.
    *   `SEUIL_SIMILARITE_R` : Le seuil de "parenté" génétique (`R`) requis.

---

### Loi 5 : Interaction Psychique et "Ennui"

*   **Intention :** Créer des comportements sociaux. L'ennui (`L`) pousse à interagir, mais l'interaction est stressante (`C`).
*   **Règle Déterministe :**
    1.  L'ennui (`L`) de chaque cellule augmente à chaque cycle (Loi 0).
    2.  Une cellule cherche à interagir avec son voisin vivant le plus "calme" (celui ayant le plus faible `L`).
    3.  L'interaction réduit `L` pour les deux, mais augmente `C` pour les deux.
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   `TAUX_AUGMENTATION_ENNUI` : La vitesse à laquelle une cellule s'ennuie.
    *   `FACTEUR_ECHANGE_PSYCHIQUE` : L'intensité du stress et du soulagement générés.

---

### Loi 6 : Mémorisation et Mouvement Intentionnel

*   **Intention :** Doter les cellules d'une mémoire primitive pour baser leurs décisions sur l'expérience passée.
*   **Règle Déterministe :**
    1.  **Mémorisation :** Au début de chaque cycle, une cellule scanne ses voisins et met à jour sa variable `M` si elle trouve une énergie (`E`) voisine supérieure à sa mémoire actuelle.
    2.  **Mouvement Intentionnel :** Le Score d'Attractivité ($S_A$) est modifié pour inclure la mémoire (voir Loi 1).
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   `K_M` : Le poids de l'influence de la mémoire sur la décision de mouvement.