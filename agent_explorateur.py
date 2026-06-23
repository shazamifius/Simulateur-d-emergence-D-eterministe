#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import urllib.request
import json
import time
import random
import sys

# ===========================================================================
# Configuration et Utilitaires API
# ===========================================================================

SIMULATOR_URL = "http://127.0.0.1:8080/"
OLLAMA_URL = "http://127.0.0.1:11434/api/generate"

def send_command(cmd, payload=None):
    """Envoie une commande JSON-RPC HTTP au simulateur SED Rust."""
    data = {"cmd": cmd}
    if payload:
        data.update(payload)
    
    req = urllib.request.Request(
        SIMULATOR_URL,
        data=json.dumps(data).encode("utf-8"),
        headers={"Content-Type": "application/json"}
    )
    try:
        with urllib.request.urlopen(req, timeout=5) as response:
            return json.loads(response.read().decode("utf-8"))
    except Exception as e:
        print(f"[Erreur API Simulateur] Impossible de se connecter au simulateur ({e}). Assurez-vous que le simulateur tourne sur 127.0.0.1:8080.")
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
        # Silencieusement renvoyer None si Ollama n'est pas installé ou actif
        return None

# ===========================================================================
# Boucle d'optimisation heuristique (Fallback)
# ===========================================================================

def heuristic_optimization(prompt, history, current_params):
    """Heuristique sémantique si aucun LLM local n'est détecté."""
    # Détecter l'intention du prompt utilisateur
    prompt_lower = prompt.lower()
    is_movement = "deplac" in prompt_lower or "mouv" in prompt_lower or "nage" in prompt_lower or "fourmi" in prompt_lower
    is_stable = "stabl" in prompt_lower or "fixe" in prompt_lower or "oscill" in prompt_lower
    
    # Prendre le dernier run pour évaluer
    if not history:
        return current_params
    
    last_run = history[-1]
    entities = last_run.get("entities", [])
    cells_alive = last_run.get("cells_alive", 0)
    
    new_params = current_params.copy()
    
    if cells_alive == 0:
        # Extinction : augmenter les ressources d'énergie, baisser les coûts
        new_params["sensibilite_soleil"] = min(0.1, new_params.get("sensibilite_soleil", 0.05) + 0.015)
        new_params["k_thermo"] = max(0.005, new_params.get("k_thermo", 0.02) - 0.005)
        new_params["cout_mouvement"] = max(0.002, new_params.get("cout_mouvement", 0.01) - 0.002)
        return new_params
        
    if is_movement:
        # Optimiser pour la vitesse : baisser le coût du mouvement, augmenter learn_rate neurones
        new_params["cout_mouvement"] = max(0.001, new_params.get("cout_mouvement", 0.01) - 0.002)
        new_params["learn_rate"] = min(0.2, new_params.get("learn_rate", 0.05) + 0.01)
        new_params["sensibilite_soleil"] = min(0.1, new_params.get("sensibilite_soleil", 0.05) + 0.005)
    elif is_stable:
        # Optimiser pour la stabilité : augmenter l'osmose, modérer les divisions
        new_params["seuil_energie_division"] = min(4.0, new_params.get("seuil_energie_division", 1.5) + 0.2)
        new_params["facteur_echange_energie"] = min(0.5, new_params.get("facteur_echange_energie", 0.1) + 0.05)
        
    # Ajouter une légère variation aléatoire (mutation génétique)
    for k in new_params:
        if isinstance(new_params[k], float) and k != "cycle":
            new_params[k] = round(new_params[k] * random.uniform(0.9, 1.1), 4)
            
    return new_params

# ===========================================================================
# Exécution Principale (CLI)
# ===========================================================================

def main():
    print("=" * 70)
    print("🤖  AGENT EXPLORATEUR IA AUTONOME - SED")
    print("=" * 70)
    
    # 1. Vérifier la connexion au simulateur
    print("[Connexion] Tentative de connexion au simulateur SED sur 127.0.0.1:8080...")
    status = send_command("get_params")
    if not status:
        sys.exit(1)
    print("[Connexion] ✅ Simulateur détecté avec succès !")
    
    # 2. Vérifier Ollama
    print("[Vérification IA] Recherche d'Ollama local sur le port 11434...")
    ollama_active = False
    test_llm = query_ollama("Respond with json: {'ok': true}")
    if test_llm and test_llm.get("ok"):
        print("[Vérification IA] ✅ Ollama local est actif ! Utilisation du LLM pour l'analyse sémantique.")
        ollama_active = True
    else:
        print("[Vérification IA] ⚠️ Ollama non détecté ou configuré. Utilisation du moteur d'auto-tuning heuristique.")
        
    # 3. Demander la recherche utilisateur
    print("-" * 70)
    user_query = input("Que souhaitez-vous chercher ? (ex: 'un rampeur rapide', 'une colonie stable') : ")
    if not user_query.strip():
        user_query = "un organisme multicellulaire stable"
    print(f"[Cible] Lancement de la recherche sémantique pour : '{user_query}'")
    print("-" * 70)
    
    history = []
    current_params = send_command("get_params")
    
    max_steps = 15
    for run in range(1, max_steps + 1):
        print(f"\n🚀 [Run #{run}/{max_steps}] Initialisation avec de nouveaux paramètres...")
        
        # Choisir une seed aléatoire
        seed = random.randint(1, 9999)
        send_command("reset", {"seed": seed, "size": 24, "density": 0.15})
        
        # Appliquer les paramètres
        send_command("set_params", {"params": current_params})
        
        # Simuler 200 cycles accélérés
        print(f"   ⌛ Simulation de 200 cycles en cours...")
        sim_res = send_command("step", {"cycles": 200})
        if not sim_res:
            break
            
        cells_alive = sim_res.get("cells_alive", 0)
        current_cycle = sim_res.get("current_cycle", 0)
        
        # Récupérer les entités segmentées par BFS 3D
        ent_res = send_command("get_entities")
        entities = ent_res.get("entities", []) if ent_res else []
        
        print(f"   📊 [Bilan Run #{run}] : Cycle actuel = {current_cycle} | Cellules vivantes = {cells_alive} | Entités = {len(entities)}")
        
        # Trier et afficher les 3 plus grandes entités
        entities.sort(key=lambda e: e.get("size", 0), reverse=True)
        for i, ent in enumerate(entities[:3]):
            cg = [round(x, 1) for x in ent.get("center_of_gravity", [0, 0, 0])]
            print(f"      - Entité #{ent['id']} : Taille={ent['size']} cells | C.G.={cg} | Firing Rate={round(ent['neural_firing_rate']*100, 1)}% | Énergie Moy={round(ent['avg_energy'], 2)}")
            
        # Enregistrer dans l'historique
        run_record = {
            "run": run,
            "seed": seed,
            "params": current_params.copy(),
            "cells_alive": cells_alive,
            "entities": entities
        }
        history.append(run_record)
        
        # Évaluer la réussite du run
        success_score = 0
        if cells_alive > 0:
            # Score de base si survie
            success_score += 10
            # Si le prompt demande du mouvement, récompenser la vitesse / taille
            if "deplac" in user_query.lower() or "mouv" in user_query.lower() or "fourmi" in user_query.lower() or "rampeur" in user_query.lower():
                max_entity_size = max([e["size"] for e in entities]) if entities else 0
                success_score += min(50, max_entity_size)
            else:
                # Sinon, récompenser la stabilité / nombre de structures autonomes
                success_score += min(50, len(entities) * 5)
        
        print(f"   🏆 Score d'adaptation : {success_score}/100")
        
        # Demander le prochain choix de paramètres à l'IA ou à la boucle d'auto-tuning
        if ollama_active:
            # Construire un prompt décrivant la situation
            history_summary = []
            for h in history[-3:]: # Garder les 3 derniers essais
                history_summary.append({
                    "run": h["run"],
                    "params": {k: v for k, v in h["params"].items() if k != "cycle"},
                    "cells_alive": h["cells_alive"],
                    "num_entities": len(h["entities"])
                })
                
            prompt = f"""
            Tu es le cerveau d'un Agent Explorateur IA pour un simulateur d'émergence biologique déterministe en 3D.
            L'utilisateur recherche : "{user_query}".
            
            Voici les résultats des derniers essais :
            {json.dumps(history_summary, indent=2)}
            
            Les paramètres actuels sont :
            {json.dumps({k: v for k, v in current_params.items() if k != "cycle"}, indent=2)}
            
            Analyse les échecs ou réussites, puis décide des nouveaux paramètres physiques à appliquer pour maximiser l'adaptation de l'organisme cible.
            Réponds EXCLUSIVEMENT sous forme d'un objet JSON contenant la clé "params" avec les 12 clés de paramètres physiques associées à leurs nouvelles valeurs numériques (k_thermo, sensibilite_soleil, hauteur_soleil, seuil_energie_division, cout_mouvement, facteur_echange_energie, seuil_similarite_r, ticks_neuraux_par_physique, seuil_fire, cout_spike, learn_rate, decay_synapse).
            """
            
            ia_params = query_ollama(prompt)
            if ia_params and "params" in ia_params:
                print("   🧠 [Décision IA] Paramètres modifiés par le LLM local.")
                current_params.update(ia_params["params"])
            else:
                print("   ⚠️ [IA Fallback] Erreur de parsing du JSON du LLM. Ajustement par heuristique.")
                current_params = heuristic_optimization(user_query, history, current_params)
        else:
            current_params = heuristic_optimization(user_query, history, current_params)
            
        time.sleep(1.0) # Laisser le temps à l'utilisateur de regarder l'affichage Rust

    # Bilan final
    print("\n" + "=" * 70)
    print("🎉 EXPLORATION TERMINÉE !")
    print("=" * 70)
    best_run = max(history, key=lambda h: h["cells_alive"] * (len(h["entities"]) + 1))
    print(f"Meilleur résultat trouvé au Run #{best_run['run']} :")
    print(f" - Seed : {best_run['seed']}")
    print(f" - Cellules vivantes : {best_run['cells_alive']}")
    print(f" - Entités autonomes : {len(best_run['entities'])}")
    print(f" - Paramètres optimaux :")
    for k, v in best_run["params"].items():
        if k != "cycle":
            print(f"   * {k}: {v}")

if __name__ == "__main__":
    main()
