# Guide de contribution au projet SED (Rust Edition)

Merci de l'intérêt que vous portez au Simulateur d'Émergence Déterministe ! Les contributions sont ce qui fait de la communauté open source un endroit incroyable pour apprendre, s'inspirer et créer.

---

## 1. Processus de Contribution

1. **Signaler un bug ou proposer une fonctionnalité** : Avant de coder, veuillez ouvrir un ticket (Issue) pour en discuter.
2. **Forker le dépôt** : Créez une copie du dépôt sur votre propre compte GitHub.
3. **Créer une branche** : Créez une branche thématique à partir de `main` (ex: `feature/amelioration-visualisation` ou `bugfix/correction-spikes`).
4. **Faire vos changements** : Codez proprement en respectant l'architecture existante.
5. **Lancer les tests** : Assurez-vous que tout passe localement en exécutant `cargo test`.
6. **Soumettre une Pull Request** : Expliquez vos changements en remplissant le modèle de PR fourni.

---

## 2. Environnement de Développement

### Sous Windows
- Installez la chaîne d'outils **Rust** standard via [rustup](https://rustup.rs/).
- Clonez le dépôt et lancez :
  ```bash
  cargo run --release
  ```

### Sous Linux (Ubuntu/Debian)
Vous devez installer les dépendances système requises pour Macroquad avant de compiler :
```bash
sudo apt-get update
sudo apt-get install -y pkg-config libasound2-dev libx11-dev libxi-dev libgl1-mesa-dev libxcursor-dev libxrandr-dev libxinerama-dev libxkbcommon-dev
cargo run --release
```

### Sous NixOS / Nix
Utilisez directement le **Flake** inclus dans le projet pour charger l'environnement pré-configuré :
```bash
# Pour entrer dans le shell de développement
nix develop

# Pour compiler l'application directement
nix build
```

---

## 3. Normes de Code & Déterminisme

> [!IMPORTANT]
> Le SED repose sur un déterminisme bit-exact garanti.
> - Aucune fonction aléatoire (`rand`) ne doit être utilisée en dehors de l'initialisation du monde.
> - L'état d'un cycle ne doit jamais dépendre de l'ordre d'évaluation parallèle. Tout calcul d'état doit s'effectuer via le système en double-tampon (`world_map` et `read_map`).
> - Ne modifiez pas les structures de données fondamentales sans mettre à jour et valider la suite de tests unitaires (`tests/test_simulation.rs`).
