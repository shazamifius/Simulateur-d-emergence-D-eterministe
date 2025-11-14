# Rapport de Comparaison : Documentation vs. Implémentation

## Introduction
Ce document a pour objectif de présenter une analyse comparative détaillée entre la documentation officielle du projet SED (`README.md`, `lois_mathematiques.md`) et l'état actuel de son implémentation dans le code source.

L'analyse est structurée en deux sections principales :
1.  **Divergences :** Les points où le code ne correspond pas ou contredit la documentation.
2.  **Extensions :** Les fonctionnalités, logiques ou optimisations présentes dans le code mais non mentionnées dans la documentation.

## I. Divergences (Le code ne correspond pas à la documentation)
Cette section met en lumière les écarts entre les intentions décrites et la réalité du code.

| Thème | Documentation (`lois_mathematiques.md`) | Implémentation (`src/MondeSED.cpp`) | Notes et Analyse |
| :--- | :--- | :--- | :--- |
| **Loi 1: Mouvement** | La formule du score d'attractivité utilise `(M_cellule / (A_cellule + 1))` pour le bonus de mémoire. | La formule appliquée est `(source_cell.M / source_cell.A)`. La condition `(source_cell.A > 0)` empêche une division par zéro. | Le `+1` au dénominateur a été omis dans le code. La formule de la documentation est plus robuste car elle gère le cas des cellules nouveau-nées (`A=0`) de manière plus élégante. L'implémentation actuelle donne un bonus mémoire infini (théoriquement) à une cellule d'âge 0, ce qui est évité uniquement par une condition `if`. |
| **Loi 1: Mouvement** | Le score d'attractivité inclut un terme `(K_E * E_voisin)`, suggérant une attraction vers l'énergie d'une case voisine. | Le mouvement ne cible que les cases vides (`!voisin_cell.is_alive`), où l'énergie `voisin_cell.E` est toujours `0.0`. Le terme `(params.K_E * voisin_cell.E)` est donc systématiquement nul. | Il n'y a pas de contradiction directe, mais une divergence d'intention. La documentation laisse penser qu'une cellule pourrait être attirée par une "trace" d'énergie, alors que le code ne prend en compte que la faim, le stress et la mémoire de la cellule elle-même pour se déplacer vers le vide. |
| **Loi 2: Division** | La cellule choisit la case voisine vide qui a la plus haute `R` (Résistance Innée). | Le code choisit la case voisine vide (`!voisin_cell.is_alive`) où la cellule (actuellement morte) possède la plus haute `R`. | C'est une nuance subtile mais importante. Les voxels vides n'ont pas de `R` intrinsèque. Le code choisit en réalité l'emplacement d'une ancienne cellule morte qui avait une `R` élevée. Cette stratégie favorise la "récupération" d'un territoire génétiquement similaire, ce qui est une interprétation intéressante de la loi, mais qui n'est pas explicitée dans la documentation. |
| **Loi 5: Interaction Psychique** | L'interaction réduit `L` pour les deux, mais augmente `C` pour les deux. L'échange est présenté comme mutuel. | L'échange est asymétrique. Les montants `montant_C` et `montant_L` sont calculés uniquement à partir des valeurs `C` et `L` de la cellule initiatrice (`source`). La cellule "calme" (`destination`) subit passivement l'interaction. | Cette implémentation crée une dynamique où une cellule "ennuyée" exporte son état psychique vers son voisin le plus calme, sans que l'état de ce dernier n'influence l'intensité de l'échange. La documentation devrait refléter cette relation à sens unique. |

## II. Extensions (Le code va au-delà de la documentation)
Cette section liste les fonctionnalités et les détails d'implémentation qui existent dans le projet mais ne sont pas documentés.

### A. Extensions de la Simulation (`src/MondeSED.cpp` & `include/MondeSED.h`)

*   **Mutation Déterministe Détaillée:**
    *   **Ce qui est documenté:** "ses gènes R et Sc subissent une mutation déterministe."
    *   **Ce qui est implémenté:** Une fonction `deterministic_mutation` a été créée. Elle utilise un algorithme de hachage simple basé sur les coordonnées de la cellule fille (`x`, `y`, `z`) et son âge (`A`) pour décider d'une mutation de `+0.01`, `-0.01`, ou `0.0`. Le calcul pour `Sc` utilise `A + 1` pour garantir une mutation différente de celle de `R`.
    *   **Note:** C'est une excellente concrétisation du concept de déterminisme. Les détails précis de l'algorithme, cruciaux pour la reproductibilité, mériteraient d'être documentés.

*   **Sauvegarde et Chargement d'État:**
    *   **Ce qui est documenté:** Rien.
    *   **Ce qui est implémenté:** Le projet supporte la sauvegarde (`SauvegarderEtat`) et le chargement (`ChargerEtat`) de l'état complet de la simulation (grille, cycle, paramètres) dans un fichier binaire (`simulation_state.sed`).
    *   **Note:** C'est une fonctionnalité majeure pour la recherche et l'analyse à long terme, totalement absente de la documentation.

*   **Chargement des Paramètres depuis un Fichier:**
    *   **Ce qui est documenté:** Les paramètres (`ParametresGlobaux`) sont décrits comme des "Leviers de Contrôle", mais sans mention d'une méthode de persistance.
    *   **Ce qui est implémenté:** Une fonction `ChargerParametresDepuisFichier` permet de charger les `ParametresGlobaux` depuis un fichier de configuration externe (`clé=valeur`).
    *   **Note:** Très utile pour lancer des simulations avec des configurations spécifiques sans passer par l'interface graphique.

### B. Extensions de l'Interface Utilisateur et de l'Expérience (`app/main.cpp`)

Le `README.md` est très minimaliste. Les fonctionnalités suivantes, pourtant essentielles à l'utilisation du logiciel, ne sont pas documentées.

*   **Inspecteur de Cellule:**
    *   **Description:** Cliquer sur une cellule dans la vue 3D ouvre une fenêtre "Inspecteur de Cellule" qui affiche en temps réel toutes les données de cette cellule (position, état, barres de progression pour E, D, C, L, et valeurs numériques pour A, M, R, Sc).
    *   **Note:** C'est l'outil de débogage et d'observation le plus puissant de l'interface. Son absence dans la documentation est une omission majeure.

*   **Graphique d'Historique:**
    *   **Description:** Le panneau de contrôle affiche un graphique traçant l'évolution du nombre de cellules vivantes au fil du temps.
    *   **Note:** Offre un aperçu immédiat de la dynamique globale de la simulation (croissance, effondrement, stabilité).

*   **Contrôles de Caméra Avancés:**
    *   **Description:** Le `README.md` ne mentionne aucun contrôle de caméra.
    *   **Implémentation:**
        *   **Orbite:** Clic molette + glisser.
        *   **Panoramique:** `Shift` + Clic molette + glisser.
        *   **Zoom:** Molette de la souris.
    *   **Note:** Indispensable pour naviguer dans l'environnement 3D.

*   **Gestion d'État (Sauvegarder/Charger) via l'UI:**
    *   **Description:** Des boutons "Sauvegarder" et "Charger" permettent d'accéder aux fonctionnalités de persistance de l'état de la simulation.
    *   **Note:** Rend cette fonctionnalité majeure accessible à l'utilisateur final.

*   **Contrôle Précis de la Simulation:**
    *   **Description:** En plus de "Démarrer" et "Pause", un bouton "Step" permet d'avancer la simulation d'un seul cycle.
    *   **Note:** Très utile pour l'analyse pas à pas de comportements émergents.

*   **Configuration de la Graine (Seed) de Génération:**
    *   **Description:** L'interface permet de définir une graine manuellement, d'en utiliser une aléatoire, et d'en générer une nouvelle.
    *   **Note:** La gestion de la graine est fondamentale pour le principe de déterminisme du projet et devrait être clairement expliquée.

*   **Thème Visuel Personnalisé:**
    *   **Description:** Le `README.md` mentionne ImGui, mais le code dans `main.cpp` implémente un thème visuel sombre et à fort contraste pour une meilleure lisibilité.
    *   **Note:** C'est un détail de finition qui améliore l'expérience utilisateur.
