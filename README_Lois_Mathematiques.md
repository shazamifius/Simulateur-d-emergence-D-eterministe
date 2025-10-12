# Documentation des Lois Mathématiques du SED

Ce document détaille l'intention philosophique et la formulation mathématique de chaque loi régissant le Simulateur d'Émergence Déterministe (SED). Chaque loi est conçue pour être simple, déterministe et contrôlable via des paramètres globaux, permettant ainsi l'étude de l'émergence de la complexité à partir de règles fondamentales.

---

### Loi 0 : Survie, Vieillissement et Mort

*   **Intention :** Établir les besoins fondamentaux de toute entité : la consommation d'énergie pour exister et la vulnérabilité à la mort. Cette loi introduit la pression de survie de base.
*   **Règle Déterministe :**
    1.  **Consommation & Vieillissement :** À chaque cycle, pour une cellule vivante :
        *   `reserve_energie -= 0.001`
        *   `dette_besoin += 0.002`
        *   `dette_stimulus += TAUX_AUGMENTATION_ENNUI`
        *   `age += 1`
    2.  **Mort Énergétique :** La cellule meurt si `reserve_energie <= 0`.
    3.  **Mort Psychique :** La cellule meurt si `charge_emotionnelle > seuil_critique`.
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   `TAUX_AUGMENTATION_ENNUI` : Contrôle la vitesse à laquelle l'ennui ou le besoin d'interaction augmente.

---

### Loi 1 : Mouvement par Recherche d'Équilibre

*   **Intention :** Simuler un comportement non aléatoire où une cellule ne se déplace pas par hasard, mais pour soulager une pression interne (faim, stress). Le mouvement est une décision calculée pour atteindre un état meilleur.
*   **Règle Déterministe :**
    1.  Une cellule vivante évalue toutes les cases voisines vides.
    2.  Pour chaque case vide, elle calcule un **Score d'Attractivité ($S_A$)** :
        *   `S_A = (K_E * E_voisin) + (K_D * D_cellule) - (K_C * C_cellule)`
    3.  La cellule choisit de se déplacer vers la case qui maximise ce score. En cas d'égalité, la case avec le plus petit index 1D est choisie pour garantir le déterminisme.
    4.  **Gestion des conflits :** Si plusieurs cellules visent la même case, seule celle avec la `dette_besoin` la plus élevée gagne le droit de se déplacer.
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   `K_E` : Poids de l'attraction vers l'énergie.
    *   `K_D` : Poids de la motivation due à la faim (`dette_besoin`).
    *   `K_C` : Poids de l'aversion au stress (`charge_emotionnelle`).

---

### Loi 2 : Division Cellulaire et Croissance

*   **Intention :** Permettre la croissance et la formation de structures multicellulaires. La division n'est pas une simple duplication, mais un processus qui favorise la cohésion génétique.
*   **Règle Déterministe :**
    1.  Une cellule se divise si `reserve_energie > SEUIL_ENERGIE_DIVISION` et qu'il y a au moins une case voisine vide.
    2.  **Choix de la cible :** La cellule choisit la case voisine vide qui a la plus haute `resistance_stress` (R), favorisant la création de "tissus" de cellules génétiquement similaires.
    3.  **Mécanisme :** L'énergie de la mère est divisée par deux. La fille hérite des propriétés de la mère, mais ses gènes (`resistance_stress` et `seuil_critique`) subissent une mutation déterministe basée sur un hash de ses coordonnées et de son âge.
    4.  **Gestion des conflits :** Si plusieurs cellules visent la même case pour se diviser, celle avec la `reserve_energie` la plus élevée l'emporte.
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   `SEUIL_ENERGIE_DIVISION` : Le seuil d'énergie requis pour la reproduction.

---

### Loi 3 : Mort Psychique (Autodestruction)

*   **Intention :** Démontrer que la "psyché" (l'état interne de stress) a un impact direct et fatal sur le corps. Une cellule peut s'autodétruire non pas par manque de ressources, mais par une surcharge émotionnelle.
*   **Règle Déterministe :**
    *   Intégrée dans la Loi 0, une cellule meurt si `charge_emotionnelle > seuil_critique`.
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   Le `seuil_critique` (Sc) est une propriété de la cellule, mais les facteurs qui augmentent la `charge_emotionnelle` (comme `FACTEUR_ECHANGE_PSYCHIQUE`) influencent indirectement cette loi.

---

### Loi 4 : Échange Énergétique et Stabilité des Tissus

*   **Intention :** Permettre aux organismes multicellulaires de survivre en tant qu'entité cohésive. Les cellules partagent leurs ressources, créant un système interdépendant qui est plus robuste qu'un simple agrégat.
*   **Règle Déterministe :**
    1.  Une cellule vivante évalue ses voisins vivants.
    2.  Elle ne peut échanger qu'avec des voisins génétiquement similaires (`abs(R_cellule - R_voisin) < SEUIL_SIMILARITE_R`).
    3.  Si sa propre énergie est significativement plus élevée que celle d'un voisin similaire (`E_cellule - E_voisin > SEUIL_DIFFERENCE_ENERGIE`), elle lui transfère une petite fraction de la différence d'énergie.
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   `FACTEUR_ECHANGE_ENERGIE` : Le pourcentage de la différence d'énergie à transférer.
    *   `SEUIL_DIFFERENCE_ENERGIE` : Le seuil à partir duquel l'échange est déclenché.
    *   `SEUIL_SIMILARITE_R` : Le seuil de "parenté" génétique requis pour l'échange.

---

### Loi 5 : Interaction Psychique et "Ennui"

*   **Intention :** Créer des comportements sociaux complexes. L'ennui (`dette_stimulus`) pousse les cellules à interagir, mais l'interaction elle-même est stressante. Cela crée une dynamique de recherche d'équilibre social.
*   **Règle Déterministe :**
    1.  L'ennui (`dette_stimulus`) de chaque cellule augmente à chaque cycle (voir Loi 0).
    2.  Une cellule cherche à interagir avec son voisin vivant le plus "calme" (celui ayant la plus faible `dette_stimulus`).
    3.  L'interaction consiste à échanger une fraction de `charge_emotionnelle` et de `dette_stimulus`. L'échange de `dette_stimulus` la fait baisser pour les deux (soulagement de l'ennui), tandis que l'échange de `charge_emotionnelle` l'augmente (le contact social est stressant).
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   `TAUX_AUGMENTATION_ENNUI` : La vitesse à laquelle une cellule s'ennuie.
    *   `FACTEUR_ECHANGE_PSYCHIQUE` : L'intensité du stress et du soulagement générés par une interaction.

---

### Loi 6 : Mémorisation et Mouvement Intentionnel

*   **Intention :** Doter les cellules d'une forme primitive de mémoire, leur permettant de baser leurs décisions de mouvement non seulement sur l'état actuel, mais aussi sur l'expérience passée. Cela transforme le mouvement réactif en une quête intentionnelle.
*   **Règle Déterministe :**
    1.  **Mémorisation :** Au début de chaque cycle, une cellule scanne ses voisins et met à jour sa `memoire_energie_max` si elle trouve une énergie voisine supérieure à sa mémoire actuelle.
    2.  **Mouvement Intentionnel :** Le Score d'Attractivité ($S_A$) est modifié pour inclure un terme de mémoire qui décroît avec l'âge :
        *   `S_A = (K_E * E_voisin) + (K_D * D_cellule) - (K_C * C_cellule) + (K_M * (memoire_energie_max / (age + 1)))`
*   **Leviers de Contrôle (`ParametresGlobaux`) :**
    *   `K_M` : Le poids de l'influence de la mémoire sur la décision de mouvement.