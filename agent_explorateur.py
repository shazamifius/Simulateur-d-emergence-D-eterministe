#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Agent Explorateur IA Autonome — SED
====================================
Explore automatiquement l'espace des paramètres du simulateur SED
pour trouver des comportements émergents intéressants.

Usage:
    python agent_explorateur.py                          # Mode interactif
    python agent_explorateur.py "un rampeur rapide"      # Mode CLI
"""

import urllib.request
import json
import time
import random
import sys
import os
import math

# ===========================================================================
# Fix Windows console encoding for Unicode (emojis)
# ===========================================================================
if sys.platform == "win32":
    os.system("")  # Enable ANSI/VT100 escape sequences
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")

# ===========================================================================
# Configuration
# ===========================================================================

SIMULATOR_URL = "http://127.0.0.1:8080/"
OLLAMA_URL = "http://127.0.0.1:11434/api/generate"

# Nombre de tentatives en cas d'erreur réseau (WinError 10053 etc.)
MAX_RETRIES = 3
RETRY_DELAY = 0.5  # secondes entre chaque tentative

# ===========================================================================
# Utilitaires API avec Retry automatique
# ===========================================================================

def send_command(cmd, payload=None, timeout=None):
    """Envoie une commande JSON-RPC HTTP au simulateur SED Rust.
    
    Inclut un mécanisme de retry automatique pour gérer les erreurs
    réseau transitoires (WinError 10053 : connexion interrompue par l'OS).
    """
    data = {"cmd": cmd}
    if payload:
        data.update(payload)
    
    # Timeout adaptatif : les commandes de simulation longues ont besoin de plus
    if timeout is None:
        timeout = 180 if cmd == "step" else 10
    
    last_error = None
    for attempt in range(1, MAX_RETRIES + 1):
        req = urllib.request.Request(
            SIMULATOR_URL,
            data=json.dumps(data).encode("utf-8"),
            headers={"Content-Type": "application/json"}
        )
        try:
            with urllib.request.urlopen(req, timeout=timeout) as response:
                return json.loads(response.read().decode("utf-8"))
        except Exception as e:
            last_error = e
            if attempt < MAX_RETRIES:
                time.sleep(RETRY_DELAY * attempt)  # Backoff progressif
            # Ne pas afficher de message pour les retries intermédiaires
    
    print(f"[Erreur API] Échec après {MAX_RETRIES} tentatives : {last_error}")
    return None

def query_ollama(prompt, model="gemma2:2b"):
    """Interroge Ollama en local s'il est actif sur le port 11434."""
    data = {
        "model": model,
        "prompt": prompt,
        "stream": False,
        "format": "json"
    }
    req = urllib.request.Request(
        OLLAMA_URL,
        data=json.dumps(data).encode("utf-8"),
        headers={"Content-Type": "application/json"}
    )
    try:
        with urllib.request.urlopen(req, timeout=15) as response:
            res = json.loads(response.read().decode("utf-8"))
            return json.loads(res.get("response", "{}"))
    except Exception:
        return None

# ===========================================================================
# Tracking de Mouvement (Déplacement du Centre de Gravité)
# ===========================================================================

def measure_movement(entities_before, entities_after):
    """Mesure le déplacement moyen et max des entités entre deux snapshots.
    
    Compare les centres de gravité des entités entre deux instants.
    Utilise un appariement par proximité (nearest-neighbor) pour 
    associer les entités entre les deux snapshots.
    
    Returns:
        dict avec avg_displacement, max_displacement, moving_entities_count
    """
    if not entities_before or not entities_after:
        return {"avg_displacement": 0.0, "max_displacement": 0.0, "moving_count": 0, "details": []}
    
    # Construire les positions des entités "avant"
    positions_before = []
    for e in entities_before:
        cg = e.get("center_of_gravity", [0, 0, 0])
        positions_before.append((e.get("id", -1), e.get("size", 0), cg))
    
    # Construire les positions des entités "après"
    positions_after = []
    for e in entities_after:
        cg = e.get("center_of_gravity", [0, 0, 0])
        positions_after.append((e.get("id", -1), e.get("size", 0), cg))
    
    # Appariement par proximité + taille similaire (nearest neighbor)
    used = set()
    movements = []
    
    for id_a, size_a, cg_a in positions_after:
        best_dist = float("inf")
        best_idx = -1
        
        for idx, (id_b, size_b, cg_b) in enumerate(positions_before):
            if idx in used:
                continue
            # Distance euclidienne 3D
            dist = math.sqrt(sum((a - b) ** 2 for a, b in zip(cg_a, cg_b)))
            # Pénaliser les entités de taille très différente
            size_ratio = min(size_a, size_b) / max(size_a, size_b, 1)
            adjusted_dist = dist / max(size_ratio, 0.1)
            
            if adjusted_dist < best_dist:
                best_dist = adjusted_dist
                best_idx = idx
        
        if best_idx >= 0:
            used.add(best_idx)
            _, _, cg_b = positions_before[best_idx]
            real_dist = math.sqrt(sum((a - b) ** 2 for a, b in zip(cg_a, cg_b)))
            movements.append({
                "entity_id": id_a,
                "size": size_a,
                "displacement": round(real_dist, 3),
                "from": [round(x, 1) for x in cg_b],
                "to": [round(x, 1) for x in cg_a]
            })
    
    if not movements:
        return {"avg_displacement": 0.0, "max_displacement": 0.0, "moving_count": 0, "details": []}
    
    displacements = [m["displacement"] for m in movements]
    # Seuil minimum pour considérer un déplacement réel (pas juste du bruit)
    MOVEMENT_THRESHOLD = 0.5
    moving = [d for d in displacements if d > MOVEMENT_THRESHOLD]
    
    return {
        "avg_displacement": round(sum(displacements) / len(displacements), 3),
        "max_displacement": round(max(displacements), 3),
        "moving_count": len(moving),
        "details": sorted(movements, key=lambda m: m["displacement"], reverse=True)[:5]
    }

# ===========================================================================
# Scoring Avancé (Multi-critères)
# ===========================================================================

def compute_score(user_query, cells_alive, entities, movement_data):
    """Calcule un score multi-critères sur 100.
    
    Critères :
    - Survie (0-15) : les cellules sont vivantes
    - Structure (0-25) : nombre et taille des entités multicellulaires  
    - Activité neurale (0-25) : taux de firing des neurones
    - Mouvement (0-25) : déplacement des centres de gravité
    - Bonus contextuel (0-10) : adapté au prompt utilisateur
    """
    score = 0
    details = []
    
    # 1. Survie (0-15)
    if cells_alive > 0:
        survie = min(15, int(cells_alive / 100))
        score += survie
        details.append(f"Survie={survie}/15")
    else:
        details.append("Survie=0/15 (EXTINCTION)")
        return 0, " | ".join(details)
    
    # 2. Structure (0-25) : diversité et taille
    if entities:
        num_entities = len(entities)
        max_size = max(e.get("size", 0) for e in entities)
        # Récompenser la diversité (beaucoup d'entités distinctes)
        diversity_score = min(10, num_entities)
        # Récompenser les grosses structures
        size_score = min(15, int(max_size / 30))
        structure = diversity_score + size_score
        score += structure
        details.append(f"Structure={structure}/25 ({num_entities} entités, max={max_size})")
    
    # 3. Activité neurale (0-25)
    if entities:
        firing_rates = [e.get("neural_firing_rate", 0) for e in entities]
        avg_firing = sum(firing_rates) / len(firing_rates) if firing_rates else 0
        max_firing = max(firing_rates) if firing_rates else 0
        neural = min(25, int(avg_firing * 100) + int(max_firing * 50))
        score += neural
        details.append(f"Neural={neural}/25 (avg={round(avg_firing*100,1)}%, max={round(max_firing*100,1)}%)")
    
    # 4. Mouvement (0-25)
    avg_disp = movement_data.get("avg_displacement", 0)
    max_disp = movement_data.get("max_displacement", 0)
    moving_count = movement_data.get("moving_count", 0)
    mouvement = min(25, int(avg_disp * 5) + int(max_disp * 3) + moving_count * 2)
    score += mouvement
    details.append(f"Mouvement={mouvement}/25 (avg={avg_disp}, max={max_disp}, {moving_count} mobiles)")
    
    # 5. Bonus contextuel (0-10)
    prompt_lower = user_query.lower()
    bonus = 0
    if "deplac" in prompt_lower or "mouv" in prompt_lower or "rampeur" in prompt_lower or "fourmi" in prompt_lower or "nage" in prompt_lower:
        # Bonus mouvement
        bonus = min(10, int(max_disp * 5))
    elif "stabl" in prompt_lower or "fixe" in prompt_lower or "coloni" in prompt_lower:
        # Bonus stabilité = beaucoup de structures
        bonus = min(10, len(entities) if entities else 0)
    elif "intellig" in prompt_lower or "neuron" in prompt_lower or "cerveau" in prompt_lower:
        # Bonus activité neurale
        if entities:
            max_fr = max(e.get("neural_firing_rate", 0) for e in entities)
            bonus = min(10, int(max_fr * 100))
    else:
        # Bonus générique : récompenser la complexité globale
        bonus = min(10, score // 10)
    
    score += bonus
    details.append(f"Bonus={bonus}/10")
    
    return min(100, score), " | ".join(details)

# ===========================================================================
# Boucle d'optimisation heuristique (Fallback sans LLM)
# ===========================================================================

def heuristic_optimization(prompt, history, current_params):
    """Heuristique sémantique si aucun LLM local n'est détecté."""
    prompt_lower = prompt.lower()
    is_movement = "deplac" in prompt_lower or "mouv" in prompt_lower or "nage" in prompt_lower or "fourmi" in prompt_lower or "rampeur" in prompt_lower
    is_neural = "intellig" in prompt_lower or "neuron" in prompt_lower or "cerveau" in prompt_lower
    is_stable = "stabl" in prompt_lower or "fixe" in prompt_lower or "oscill" in prompt_lower or "coloni" in prompt_lower
    
    if not history:
        return current_params
    
    last_run = history[-1]
    entities = last_run.get("entities", [])
    cells_alive = last_run.get("cells_alive", 0)
    movement = last_run.get("movement", {})
    
    new_params = current_params.copy()
    
    if cells_alive == 0:
        # Extinction : augmenter les ressources d'énergie, baisser les coûts
        new_params["sensibilite_soleil"] = min(0.1, new_params.get("sensibilite_soleil", 0.05) + 0.015)
        new_params["k_thermo"] = max(0.005, new_params.get("k_thermo", 0.02) - 0.005)
        new_params["cout_mouvement"] = max(0.002, new_params.get("cout_mouvement", 0.01) - 0.002)
        return new_params
    
    # Toujours essayer d'activer les neurones (le problème principal)
    avg_firing = 0
    if entities:
        avg_firing = sum(e.get("neural_firing_rate", 0) for e in entities) / max(len(entities), 1)
    
    if avg_firing < 0.01:
        # Neurones quasi-inactifs → baisser le seuil de fire, augmenter learn_rate
        new_params["seuil_fire"] = max(0.1, new_params.get("seuil_fire", 0.5) - 0.05)
        new_params["learn_rate"] = min(0.3, new_params.get("learn_rate", 0.05) + 0.02)
        new_params["cout_spike"] = max(0.001, new_params.get("cout_spike", 0.01) - 0.002)
        new_params["ticks_neuraux_par_physique"] = min(10, new_params.get("ticks_neuraux_par_physique", 3) + 1)
    
    if is_movement:
        # Optimiser pour la vitesse
        new_params["cout_mouvement"] = max(0.001, new_params.get("cout_mouvement", 0.01) - 0.002)
        new_params["learn_rate"] = min(0.2, new_params.get("learn_rate", 0.05) + 0.01)
        new_params["sensibilite_soleil"] = min(0.1, new_params.get("sensibilite_soleil", 0.05) + 0.005)
        # Si pas de mouvement détecté, changer plus agressivement
        if movement.get("max_displacement", 0) < 1.0:
            new_params["seuil_fire"] = max(0.05, new_params.get("seuil_fire", 0.5) * 0.7)
    elif is_neural:
        # Optimiser pour l'activité neurale
        new_params["seuil_fire"] = max(0.05, new_params.get("seuil_fire", 0.5) - 0.08)
        new_params["learn_rate"] = min(0.3, new_params.get("learn_rate", 0.05) + 0.03)
        new_params["ticks_neuraux_par_physique"] = min(15, new_params.get("ticks_neuraux_par_physique", 3) + 2)
        new_params["cout_spike"] = max(0.0005, new_params.get("cout_spike", 0.01) * 0.8)
    elif is_stable:
        # Optimiser pour la stabilité
        new_params["seuil_energie_division"] = min(4.0, new_params.get("seuil_energie_division", 1.5) + 0.2)
        new_params["facteur_echange_energie"] = min(0.5, new_params.get("facteur_echange_energie", 0.1) + 0.05)
        
    # Mutation aléatoire légère
    for k in new_params:
        if isinstance(new_params[k], float) and k != "cycle":
            new_params[k] = round(new_params[k] * random.uniform(0.92, 1.08), 4)
            
    return new_params

# ===========================================================================
# Exécution Principale
# ===========================================================================

def main():
    print("=" * 70)
    print("🤖  AGENT EXPLORATEUR IA AUTONOME - SED v2")
    print("    Avec tracking de mouvement + scoring avancé")
    print("=" * 70)
    
    # 1. Vérifier la connexion au simulateur
    print("[Connexion] Tentative de connexion au simulateur SED sur 127.0.0.1:8080...")
    status = send_command("get_params")
    if not status:
        print("[Erreur] Impossible de se connecter. Lancez d'abord le simulateur avec `cargo run`.")
        sys.exit(1)
    print("[Connexion] ✅ Simulateur détecté avec succès !")
    
    # 2. Vérifier Ollama
    print("[Vérification IA] Recherche d'Ollama local sur le port 11434...")
    ollama_active = False
    test_llm = None
    if False:
        print("[Vérification IA] ✅ Ollama local est actif !")
        ollama_active = True
    else:
        print("[Vérification IA] ⚠️ Ollama non détecté. Mode heuristique activé.")
        
    # 3. Demander la recherche utilisateur
    print("-" * 70)
    if len(sys.argv) > 1:
        user_query = " ".join(sys.argv[1:])
        print(f"[Mode CLI] Recherche pour : '{user_query}'")
    else:
        user_query = input("🔍 Que souhaitez-vous chercher ? : ")
    if not user_query.strip():
        user_query = "un organisme multicellulaire stable"
    print(f"[Cible] Lancement de la recherche pour : '{user_query}'")
    print("-" * 70)
    
    history = []
    current_params = send_command("get_params")
    if not current_params:
        print("[Erreur] Impossible de récupérer les paramètres.")
        sys.exit(1)
    
    # --- Override des paramètres neuraux pour activer les neurones ---
    # EXPLICATION : Le seuil_fire par défaut (0.85) est mathématiquement 
    # inatteignable car :
    #   - Bruit déterministe : range [-0.05, +0.049]  
    #   - Poids synaptiques initiaux : 0.01 (très faibles)
    #   - Décay : p * 0.9 chaque tick
    # → Le potentiel max converge vers ~0.2, jamais vers 0.85
    # L'agent abaisse donc le seuil pour permettre l'activité neuronale.
    # L'utilisateur peut toujours changer ces valeurs via l'interface GUI.
    print("[Agent] 🧠 Ajustement des paramètres neuraux pour activation...")
    current_params["seuil_fire"] = 0.15       # Seuil atteignable par le bruit
    current_params["learn_rate"] = 0.15        # Apprentissage plus rapide
    current_params["ticks_neuraux_par_physique"] = 8  # Plus de ticks neuraux
    current_params["cout_spike"] = 0.002       # Coût réduit pour ne pas tuer les neurones
    current_params["decay_synapse"] = 0.995    # Décay lent pour garder les connexions
    print(f"   seuil_fire: 0.85 → 0.15 | learn_rate: → 0.15 | ticks_neuraux: → 8")
    
    max_runs = 15
    best_score_global = 0
    
    for run in range(1, max_runs + 1):
        print(f"\n{'='*50}")
        print(f"🚀 [Run #{run}/{max_runs}]")
        print(f"{'='*50}")
        
        # Choisir une seed aléatoire
        seed = random.randint(1, 9999)
        res = send_command("reset", {"seed": seed, "size": 24, "density": 0.15})
        if not res:
            print("   ⚠️ Reset échoué, tentative suivante...")
            continue
        
        # Appliquer les paramètres
        send_command("set_params", {"params": current_params})
        
        # --- Phase 1 : Simulation initiale (5000 cycles, par blocs de 100 pour garder le GUI fluide) ---
        print(f"   ⌛ Phase 1 : 5000 premiers cycles...")
        sim_res = None
        for _ in range(50):
            sim_res = send_command("step", {"cycles": 100})
            if not sim_res:
                break
        if not sim_res:
            print("   ⚠️ Simulation échouée, passage au run suivant...")
            continue
        
        # Snapshot des entités au cycle 5000
        ent_res_t1 = send_command("get_entities")
        entities_t1 = ent_res_t1.get("entities", []) if ent_res_t1 else []
        
        # --- Phase 2 : Simulation suite (100 cycles de plus) ---
        print(f"   ⌛ Phase 2 : 100 cycles supplémentaires (tracking mouvement)...")
        sim_res = send_command("step", {"cycles": 100})
        if not sim_res:
            continue
            
        cells_alive = sim_res.get("cells_alive", 0)
        current_cycle = sim_res.get("current_cycle", 0)
        
        # Snapshot des entités au cycle 5100
        ent_res_t2 = send_command("get_entities")
        entities_t2 = ent_res_t2.get("entities", []) if ent_res_t2 else []
        
        # --- Mesure du mouvement ---
        movement = measure_movement(entities_t1, entities_t2)
        
        # --- Scoring avancé ---
        score, score_details = compute_score(user_query, cells_alive, entities_t2, movement)
        
        # Affichage
        print(f"\n   📊 [Bilan Run #{run}]")
        print(f"      Cycle = {current_cycle} | Cellules = {cells_alive} | Entités = {len(entities_t2)}")
        print(f"      Mouvement : avg={movement['avg_displacement']} | max={movement['max_displacement']} | {movement['moving_count']} entités mobiles")
        
        # Top 3 entités
        entities_t2.sort(key=lambda e: e.get("size", 0), reverse=True)
        for i, ent in enumerate(entities_t2[:3]):
            cg = [round(x, 1) for x in ent.get("center_of_gravity", [0, 0, 0])]
            fr = round(ent.get('neural_firing_rate', 0) * 100, 1)
            en = round(ent.get('avg_energy', 0), 2)
            print(f"      - Entité #{ent['id']} : {ent['size']} cells | C.G.={cg} | Neural={fr}% | E={en}")
        
        # Top mouvements
        if movement["details"]:
            print(f"      🏃 Mouvements détectés :")
            for m in movement["details"][:3]:
                if m["displacement"] > 0.5:
                    print(f"         Entité #{m['entity_id']} ({m['size']} cells) : {m['from']} → {m['to']} (Δ={m['displacement']})")
        
        # Score
        is_best = score > best_score_global
        if is_best:
            best_score_global = score
        marker = " ⭐ NOUVEAU RECORD !" if is_best else ""
        print(f"\n   🏆 Score : {score}/100{marker}")
        print(f"      {score_details}")
        
        # Enregistrer dans l'historique
        history.append({
            "run": run,
            "seed": seed,
            "params": current_params.copy(),
            "cells_alive": cells_alive,
            "entities": entities_t2,
            "movement": movement,
            "score": score
        })
        
        # Optimisation des paramètres pour le prochain run
        if ollama_active:
            history_summary = []
            for h in history[-3:]:
                history_summary.append({
                    "run": h["run"], "score": h["score"],
                    "cells_alive": h["cells_alive"],
                    "num_entities": len(h["entities"]),
                    "max_displacement": h["movement"].get("max_displacement", 0),
                    "params": {k: v for k, v in h["params"].items() if k != "cycle"}
                })
            prompt = f"""
            Tu es le cerveau d'un Agent Explorateur IA pour un simulateur d'émergence biologique déterministe 3D.
            L'utilisateur recherche : "{user_query}".
            
            Résultats des derniers essais : {json.dumps(history_summary, indent=2)}
            Paramètres actuels : {json.dumps({k: v for k, v in current_params.items() if k != "cycle"}, indent=2)}
            
            IMPORTANT: Les neurones ne fire jamais si seuil_fire est trop haut. Commence bas (~0.2).
            Réponds EXCLUSIVEMENT en JSON: {{"params": {{...les 12 paramètres...}}}}
            """
            ia_params = query_ollama(prompt)
            if ia_params and "params" in ia_params:
                print("   🧠 [IA] Paramètres modifiés par le LLM.")
                current_params.update(ia_params["params"])
            else:
                current_params = heuristic_optimization(user_query, history, current_params)
        else:
            current_params = heuristic_optimization(user_query, history, current_params)
            
        time.sleep(0.5)

    # === Bilan final ===
    print("\n" + "=" * 70)
    print("🎉 EXPLORATION TERMINÉE !")
    print("=" * 70)
    
    if not history:
        print("Aucun résultat enregistré. Vérifiez la connexion au simulateur.")
        return
    
    best_run = max(history, key=lambda h: h["score"])
    print(f"\n🥇 Meilleur résultat : Run #{best_run['run']} (Score: {best_run['score']}/100)")
    print(f"   Seed : {best_run['seed']}")
    print(f"   Cellules vivantes : {best_run['cells_alive']}")
    print(f"   Entités autonomes : {len(best_run['entities'])}")
    print(f"   Mouvement max : {best_run['movement'].get('max_displacement', 0)}")
    print(f"   Entités mobiles : {best_run['movement'].get('moving_count', 0)}")
    print(f"\n   Paramètres optimaux :")
    for k, v in sorted(best_run["params"].items()):
        if k != "cycle":
            print(f"   * {k}: {v}")

    # Sauvegarde du meilleur specimen dans snapshots/
    print(f"\n💾 [Sauvegarde] Recréation et sauvegarde du meilleur spécimen...")
    send_command("reset", {"seed": best_run["seed"], "size": 24, "density": 0.12})
    send_command("set_params", {"params": best_run["params"]})
    for _ in range(51):
        send_command("step", {"cycles": 100})

    filename = f"meilleur_specimen_run_{best_run['run']}_score_{best_run['score']}.sed"
    res_save = send_command("save_snapshot", {"path": filename})
    if res_save and res_save.get("status") == "success":
        print(f"✅ Spécimen sauvegardé sous : snapshots/{filename}")
    else:
        print(f"❌ Échec de la sauvegarde du spécimen.")
    
    # Top 3 runs
    top3 = sorted(history, key=lambda h: h["score"], reverse=True)[:3]
    print(f"\n📊 Classement des 3 meilleurs runs :")
    for i, r in enumerate(top3):
        print(f"   {i+1}. Run #{r['run']} — Score={r['score']}/100 | Seed={r['seed']} | Cells={r['cells_alive']} | Entités={len(r['entities'])} | MaxMouv={r['movement'].get('max_displacement', 0)}")

if __name__ == "__main__":
    main()
