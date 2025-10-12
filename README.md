# 🔬 Projet SED : Simulateur d'Émergence Déterministe (SED)

## 1. VISION DU PROJET : L'Émergence Déterministe

**Objectif Philosophique :** Le Simulateur d'Émergence Déterministe vise à prouver que la complexité de la vie, de la stabilité et des comportements cognitifs est le résultat **unique et entièrement traçable** (déterministe) de lois d'interaction mathématiques, sans intervention du hasard.

**Le Défi Ultime (Test de la Soupe Primordiale) :** Créer un ensemble de règles initiales garantissant qu'une structure multicellulaire stable et auto-entretenue émergera obligatoirement à partir de n'importe quel état initial aléatoire du système, après un certain nombre de cycles.

---

## 2. ARCHITECTURE TECHNIQUE (Phase I : Le Cœur du Calcul)

Ce projet exige une performance maximale. Le calcul est strictement séparé de la visualisation.

* **Langage Principal :** **C++** (Optimisation, gestion mémoire).
* **Structure du Monde :** **Voxel Grid 3D** (Matrice 3D) de taille 64x64x64.
* **L'Entité de Base :** `struct Cellule`.

### Définition de la `struct Cellule` (C++)

| Variable | Type | Rôle | Type (Constante/Dynamique) |
| :--- | :--- | :--- | :--- |
| `reserve_energie` | `float` | Carburant vital pour l'action et la survie (0.0 à 1.0). | Dynamique (N) |
| `dette_besoin` | `float` | Agrégat des besoins urgents (faim, repos). Plus il est haut, plus l'urgence est grande. | Dynamique (N) |
| `charge_emotionnelle` | `float` | Niveau d'activation (stress, peur). Si > `seuil_stress_critique`, risque de "psychose" ou de mort. | Dynamique (N) |
| `dette_stimulus` | `float` | Le besoin d'interagir ou de changer d'environnement (ennui). | Dynamique (N) |
| `age_cycles` | `int` | Le temps de vie. Utilisé comme facteur d'affaiblissement général. | Dynamique (N) |
| `resistance_innate` | `float` | Résistance innée à l'influence du voisinage (Facteur "Rebelle"). | Constante (M) |
| `seuil_stress_critique` | `float` | Seuil de tolérance au stress. Fixé à la naissance (ex: 0.8). | Constante (M) |

---

## 3. PROJECTION FINALE ET ŒUVRE D'ART (Phase II : Le Rendu)

**Le Logiciel Final :** Un programme C++ exécutant la simulation, communiquant ses données en temps réel à un moteur Unreal Engine.

**L'Œuvre d'Art :** Une animation 3D immersive où l'on observe l'univers Voxel 64x64x64. L'activité de l'agrégat multicellulaire est rendue visible par des **champs d'influence** (shading smooth) qui émanent des Voxels. Le spectateur observe la naissance d'une **conscience collective** (une entité stable) dont le comportement complexe est le résultat direct, mais imprévisible, de la simple loi mathématique codée.
