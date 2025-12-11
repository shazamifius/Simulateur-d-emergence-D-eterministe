# Spécification Technique SED V2.0 (Architecture Cognitive Unifiée)

**Statut :** Documentation de Production.
**Garanties :** Conservation d'énergie, Déterminisme bit-exact, Stabilité numérique.

-----

## 1\. Architecture Système & Topologie

  * **Espace :** Grille 2D (Voxels). Dimensions $W \times H$.
  * **Conditions aux Limites :**
      * **Périodique (Torique)** : Recommandé par défaut. $(x+W) \% W$. Évite les effets de bord et simule un espace infini.
      * *Optionnel :* **Mur (Absorbant)** : Les entités ne traversent pas.
  * **Double-Buffering & Atomicité :**
      * **Cycle Physique (Lent) :** `Grid_Phys_Read` (T) $\to$ `Grid_Phys_Write` (T+1).
      * **Cycle Neural (Rapide) :** `Buffer_P_Read` $\to$ `Buffer_P_Write` (interne à la sous-boucle).
  * **Déterminisme (RNG) :**
      * Aucun `rand()` standard.
      * Utilisation d'une fonction de hachage sans état (Stateless Hash) pour tout événement stochastique.
      * **Fonction recommandée :** `xxHash64` ou `PCG-Hash`.
      * Signature : `FloatRand(SeedGlobal, Tick, CellID, Salt)`.

-----

## 2\. Structure de l'Entité (Cellule)

Alignement mémoire optimisé (ex: `struct alignas(64) Cell`).

| Catégorie | Variable | Symbole | Type | Init | Description & Invariant |
| :--- | :--- | :---: | :---: | :---: | :--- |
| **Identité** | **Type** | $T$ | `uint8` | *Rand* | $0$=Souche, $1$=Soma, $2$=Neurone. |
| **Physique** | **Énergie** | $E$ | `float` | $1.0$ | Vitalité. Si $E \le 0 \to$ Mort. |
| | **Dette** | $D$ | `float` | $0.0$ | Priorité motrice. |
| | **Stress** | $C$ | `float` | $0.0$ | Dommage structurel $[0,1]$. |
| | **Seuil Crit.**| $Sc$ | `float` | $0.5$ | Résistance au Stress $[0,1]$. |
| | **Génétique** | $R$ | `float` | *Rand* | Compatibilité Osmose $[0,1]$. |
| **Psychique** | **Ennui** | $L$ | `float` | $0.0$ | Pulsion stimulus. |
| | **Mémoire** | $M$ | `float` | $0.0$ | Max Énergie perçue. |
| **Neural** | **Potentiel** | $P$ | `float` | $0.0$ | Charge $[-1, 1]$. |
| | **Réfractaire**| $Ref$ | `uint8` | $0$ | Compteur de ticks de repos post-spike. |
| | **Coût Pend.** | $E_{cost}$| `float` | $0.0$ | Accumulateur de coût énergétique neural. |
| | **Synapses** | $W[8]$ | `float` | $\sim 0.01$ | Poids connexions $[0, 1]$. Normale Initiale. |
| | **Historique** | $H$ | `uint32`| $0$ | Bitfield (32 ticks). |
| **Spatial** | **Gradient** | $G$ | `float` | $0.0$ | Position relative $[0, 1]$. |

-----

## 3\. Configuration Universelle (`config.json`)

```json
{
  "// GRID": "---",
  "width": 256,            "height": 256,
  "boundary": "periodic",  "global_seed": 123456789,

  "// PHYSIQUE": "---",
  "k_thermo": 0.001,       "cout_mvt": 0.01,
  "dD_per_tick": 0.002,    "dL_per_tick": 0.001,
  "seuil_division": 1.8,   "facteur_echange": 0.1,
  "max_flux": 0.05,        "r_diff": 2,

  "// NEURAL": "---",
  "ticks_neural_per_phys": 5,
  "seuil_fire": 0.85,      "cost_spike": 0.005,
  "refract_period": 2,     "// en ticks neuraux": "",
  "decay_synapse": 0.999,  "learn_rate": 0.05,
  "r_ignition": 4,         "r_inhibition": 1
}
```

-----

## 4\. Initialisation du Monde (Genèse)

Au tick 0, l'univers ne doit pas être vide pour démarrer.

1.  **Remplissage :** Remplir $X\%$ de la grille (ex: 20%) avec des cellules vivantes.
2.  **Distribution :**
      * $E \sim \text{Uniform}(0.5, 1.5)$
      * $W[i] \sim \text{Uniform}(0.01, 0.05)$ (Connexions faibles au départ).
      * $Type$: 80% Souche, 20% Neurone (ou tout Souche pour laisser faire la morphogenèse).
      * $R, Sc \sim \text{Uniform}(0.0, 1.0)$.
3.  **Bootstrapping :** Exécuter 1 tick sans mort ni mouvement pour stabiliser les champs $G$.

-----

## 5\. Les Lois (Logique Mathématique)

### Groupe A : Boucle Neurale (Temps Rapide)

*S'exécute $N$ fois par tick physique. Utilise `Buffer_P_Read` et écrit dans `Buffer_P_Write`.*

#### Loi 1 : Intégration Synaptique Normalisée

Pour chaque Neurone ($Type=2$) :

1.  **Gestion Réfractaire :**
      * Si $Ref > 0$ : $Ref \leftarrow Ref - 1$, $P_{new} \leftarrow -0.5$. (Inhibé).
      * Sinon : Continuer.
2.  **Sommation Normalisée :**
    $$S_W = \sum_{v} W_v, \quad I = \frac{\sum_{v} (P_v \cdot W_v)}{\max(1.0, S_W)}$$
      * *Note : La division normalise l'entrée pour éviter l'explosion des valeurs si $W$ augmente.*
3.  **Mise à jour Potentiel :**
    $$P_{new} \leftarrow (P_{old} \cdot 0.9) + I + \text{Noise}(\text{Seed}, t, ID)$$

#### Loi 2 : Activation (Spike) & Coût

1.  **Seuil :** Si $P_{new} > seuil\_fire$ :
      * $P_{new} \leftarrow 1.0$ (Spike).
      * $Ref \leftarrow refract\_period$.
      * $E_{cost} \leftarrow E_{cost} + cost\_spike$ (Accumulation de la dette énergétique).
      * Mise à jour Bitfield : $H \leftarrow (H \ll 1) | 1$.
2.  **Repos :** Sinon :
      * $H \leftarrow (H \ll 1) | 0$.
3.  **Ignition (Broadcast) :** Si Spike, ajouter $+0.1$ au $P$ des neurones dans rayon $r\_ignition$.

-----

### Groupe B : Boucle Physique (Temps Lent)

*S'exécute 1 fois. Utilise `Grid_Read` et écrit dans `Grid_Write`.*

#### Loi 3 : Apprentissage (Hebb Post-Hoc)

*Avant toute modification physique.*
Pour chaque Neurone :

1.  **Plasticité :** Si ($P_{last} == 1.0$) :
      * Pour chaque voisin $i$ :
          * Si ($H_{voisin}$ a un bit à 1 dans les 3 derniers ticks) : $W_i \leftarrow W_i + learn\_rate$.
          * Sinon : $W_i \leftarrow W_i - (learn\_rate \cdot 0.1)$.
2.  **Homéostasie :** $W_i \leftarrow \text{clamp}(W_i \cdot decay\_synapse, 0, 1)$.

#### Loi 4 : Métabolisme & Mort

1.  **Facturation :** $E_{new} \leftarrow E - k_{thermo} - E_{cost}$.
      * *Reset $E_{cost} \leftarrow 0$ pour le prochain tour.*
2.  **Dette/Ennui :** $D \leftarrow D + dD$, $L \leftarrow L + dL$.
3.  **Sanity Check (Mort) :**
      * Si $E_{new} \le 0$ OU $C > Sc$ OU $E_{new}$ est `NaN` : Cellule meurt (case devient vide).

#### Loi 5 : Morphogenèse (Gradient)

1.  Calcul Barycentre $\Omega$.
2.  $G \leftarrow \exp(-\lambda \cdot \text{dist}(Pos, \Omega))$.
3.  Si Souche : Spécialisation selon $G$ (Peau vs Neurone).

#### Loi 6 : Interaction Environnement (Champs & Mouvement)

1.  Calcul `FieldMap` (E, C).
2.  Calcul Score Mouvement vers case vide $v$ :
    $$Score = (k_D D) - (k_C C) + \text{BonusChamp}(v) + \text{Adhesion}(v) - \text{cout\_mvt}$$
3.  **Résolution Conflit Mouvement :**
      * Trier candidats par $D$ décroissant.
      * Vainqueur : $Pos_{new} = v$, $E \leftarrow E - \text{cout\_mvt}$.

#### Loi 7 : Osmose & Reproduction

  * **Osmose :** Échange symétrique borné par `max_flux`. Unicité par ID ($ID_A < ID_B$).
  * **Reproduction :** Si $E > seuil$, division par 2 vers case vide. Priorité au plus riche ($E$).

-----

## 6\. Algorithme Principal (Master Loop)

```cpp
void Tick_Global() {
    // 1. Métriques & Barycentre
    Stats stats = ComputeStats(Grid_Read);
    Vec2 center = stats.barycenter;

    // 2. BOUCLE NEURALE (N itérations)
    // Copie initiale P_read <- Grid_Read.P
    for(int i=0; i < N_NEURAL; i++) {
        for(Cell c : AllCells) {
            // Loi 1 & 2
            float input = ComputeNormalizedInput(c, Buffer_P_Read);
            Buffer_P_Write[c] = ComputeActivation(c, input, &c.E_cost_accum); 
            // Note: E_cost_accum est accumulé localement
        }
        Swap(Buffer_P_Read, Buffer_P_Write);
        ApplyIgnitionBroadcast(Buffer_P_Read); // Sur le buffer lu au prochain sub-tick
    }
    // Sauvegarder état neural final dans Grid_Read (pour Hebb & Visu)
    SyncNeuralState(Grid_Read, Buffer_P_Read);

    // 3. BOUCLE PHYSIQUE
    ClearFieldMaps();
    ComputeFields(Grid_Read); // Loi 6 partie 1

    // Calcul des Intentions (Lecture T uniquement)
    List<Intent> intents;
    for(Cell c : Grid_Read) {
        if(ApplyMetabolism(c) == DEAD) continue; // Loi 4
        ApplyMorpho(c, center); // Loi 5
        ApplyHebb(c);           // Loi 3
        
        intents.add( ComputeMove(c) );   // Loi 6 partie 2
        intents.add( ComputeOsmose(c) ); // Loi 7
        intents.add( ComputeSpawn(c) );  // Loi 7
    }

    // 4. RÉSOLUTION & ÉCRITURE (Write T+1)
    Grid_Write.Clear();
    ResolveOsmose(intents);      // Modifie E temp
    ResolveMoves(intents);       // Ecrit dans Grid_Write
    ResolveSpawns(intents);      // Ecrit dans Grid_Write
    ResolveContacts(Grid_Write); // Ajuste C et L finaux
    
    // 5. SANITIZATION FINAL
    for(Cell c : Grid_Write) {
        ClampAllVariables(c);
        if(isnan(c.E)) c.Kill();
    }

    Swap(Grid_Read, Grid_Write);
    GlobalTick++;
}
```

-----

## 7\. Robustesse & Clamping (Invariants)

  * **P :** $[-1.0, 1.0]$ (Toujours).
  * **W :** $[0.0, 1.0]$ (Toujours).
  * **C, R, Sc, G :** $[0.0, 1.0]$.
  * **E, D, L, M :** $[0.0, \infty[$.
  * **NaN Policy :** Si une variable devient `NaN` ou `Inf`, la cellule est considérée comme corrompue et est supprimée immédiatement pour protéger la simulation.

-----

## 8\. Persistance & Métriques

### Format Snapshot (Binaire)

Pour sauvegarder l'état complet :

1.  **Header :** `Magic("SED8")`, `Version`, `Tick`, `GlobalSeed`, `GridW`, `GridH`.
2.  **Paramètres :** Copie du JSON config.
3.  **Body :** Dump brut du vecteur `Cells` (comprimé LZ4 recommandé).
      * *Raison :* Permet de reprendre la simulation exactement là où elle s'est arrêtée.

### Métriques de Monitoring (CSV)

À logger à chaque Tick :

1.  `Total_Energy` : Somme E (Doit diminuer de `k_thermo` \* pop).
2.  `Pop_Count` : Total, et par Type (Soma/Neuro).
3.  `Mean_Stress` : Santé globale.
4.  `Spike_Rate` : Nombre total de spikes / Nombre de neurones.
5.  `Ignition_Events` : Nombre de fois où le broadcast a été déclenché.
6.  `Network_Stability` : Moyenne de $\Delta W$ (doit tendre vers 0 si stable).
