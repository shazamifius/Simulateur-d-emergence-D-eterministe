# 🔬 SED — Simulateur d'Émergence Déterministe

[![Build and Test](https://github.com/shazamifius/Simulateur-d-emergence-D-eterministe/actions/workflows/build.yml/badge.svg)](https://github.com/shazamifius/Simulateur-d-emergence-D-eterministe/actions/workflows/build.yml)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)

> Un univers 3D déterministe où la complexité biologique et les comportements multicellulaires stables émergent de lois physiques et neurales strictes.

---

## 1. Vision & Philosophie

Le **SED** (Simulateur d'Émergence Déterministe) est un automate cellulaire tridimensionnel conçu pour prouver que des comportements biologiques complexes (mouvement, recherche de ressources, gestion du stress, auto-organisation) peuvent émerger de manière unique, reproductible et prévisible sans aucune variable aléatoire.

### Principes Fondamentaux :
1. **Déterminisme Absolu (Bit-Exact)** : La simulation produit le même résultat au bit près, peu importe le processeur ou le système d'exploitation, à partir d'un état initial et d'une chronologie donnés.
2. **Émergence de la Stabilité** : Les cellules individuelles agissent selon des règles simples (physiques et électriques). En se regroupant, elles forment des organismes stables et résilients.
3. **Double Horloge Temporelle** : Coexistence d'une boucle rapide pour le traitement de l'information électrique (Spikes) et d'une boucle plus lente pour la dynamique métabolique et le mouvement.

---

## 2. Fonctionnalités de la Version Rust

La simulation inclut les outils interactifs suivants :
* **Pinceau & Édition 3D (Raycasting)** : Ajoutez ou supprimez des cellules directement en 3D dans le monde à l'aide de la souris. Outil d'ajout intelligent qui se base sur la face cliquée d'une cellule existante.
* **Inspecteur de Cellule en Temps Réel** : Cliquez sur n'importe quel voxel pour inspecter en détail son état physique (Énergie, Stress, Résistance, Osmose) et neural (Potentiel électrique, Historique de Spikes, Compteur réfractaire).
* **Sauvegarde & Restauration (Snapshots)** : Sauvegardez l'état complet de la simulation sous forme de fichier JSON (`snapshot.json`) pour la recharger à tout moment.
* **Enregistrement & Replay Déterministe** : Enregistrez vos sessions et rejouez-les à l'identique. Chaque action utilisateur (peinture, modification de paramètre) est réappliquée au cycle exact.
* **Export de Métriques CSV** : Enregistrement régulier de métriques statistiques temporelles de la simulation (population, énergie totale, taux de spikes, stress moyen).

---

## 3. Installation et Lancement

### 🖥️ Windows (Standard)
1. Téléchargez et installez la chaîne d'outils **Rust** via [rustup.rs](https://rustup.rs/).
2. Clonez le dépôt et naviguez à la racine du projet :
   ```bash
   git clone https://github.com/shazamifius/Simulateur-d-emergence-D-eterministe.git
   cd Simulateur-d-emergence-D-eterministe
   ```
3. Compilez et lancez l'application en mode optimisé :
   ```bash
   cargo run --release
   ```

### 🐧 Linux (Ubuntu / Debian)
Installez d'abord les bibliothèques système nécessaires pour le rendu graphique de Macroquad :
```bash
sudo apt-get update
sudo apt-get install -y pkg-config libasound2-dev libx11-dev libxi-dev libgl1-mesa-dev libxcursor-dev libxrandr-dev libxinerama-dev libxkbcommon-dev
cargo run --release
```

### ❄️ NixOS & Nix
Le projet fournit un **Flake** pour configurer l'environnement en une commande.

* Pour entrer dans le shell de développement pré-configuré (contenant toutes les dépendances X11/OpenGL nécessaires) :
  ```bash
  nix develop
  cargo run --release
  ```
* Pour compiler l'application directement avec Nix :
  ```bash
  nix build
  ```

---

## 4. Raccourcis et Interaction Caméra

* **Clic Droit + Déplacer la souris** : Orbiter la caméra autour de la simulation (Yaw & Pitch corrigés et fluides).
* **Molette de la souris** : Zoomer / Dézoomer (rapprocher ou éloigner la caméra).
* **Barre d'espace** : Play / Pause de la simulation.
* **Touche TAB** : Afficher ou masquer l'interface graphique (GUI).
* **Flèche Droite (→)** : Avancer la simulation d'exactement un cycle (utile pour déboguer pas à pas).

---

## 5. Licence

Ce projet est distribué sous la **Licence Apache 2.0**. Voir le fichier [LICENSE](LICENSE) pour plus de détails.
