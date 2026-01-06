#include "InputSystem.h"
#include <algorithm>

void InputSystem::RecordAction(const SimulationAction &action) {
  recorded_actions.push_back(action);
}

void InputSystem::SaveRecording(const std::string &filename) {
  nlohmann::json j_root;
  j_root["actions"] = nlohmann::json::array();

  for (const auto &action : recorded_actions) {
    j_root["actions"].push_back(action.toJson());
  }

  std::ofstream file(filename);
  if (file.is_open()) {
    file << j_root.dump(4);
    file.close();
    std::cout << "[InputSystem] Recording saved to " << filename << std::endl;
  } else {
    std::cerr << "[InputSystem] Failed to save recording to " << filename
              << std::endl;
  }
}

bool InputSystem::LoadRecording(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "[InputSystem] Failed to open " << filename << std::endl;
    return false;
  }

  try {
    nlohmann::json j_root;
    file >> j_root;

    recorded_actions.clear();
    if (j_root.contains("actions")) {
      for (const auto &j_action : j_root["actions"]) {
        recorded_actions.push_back(SimulationAction::fromJson(j_action));
      }
    }

    // Sort just in case JSON was messed up, though typically ordered
    std::sort(recorded_actions.begin(), recorded_actions.end(),
              [](const SimulationAction &a, const SimulationAction &b) {
                return a.cycle < b.cycle;
              });

    std::cout << "[InputSystem] Loaded " << recorded_actions.size()
              << " actions." << std::endl;
    playback_index = 0;
    is_replaying = true;
    return true;
  } catch (const std::exception &e) {
    std::cerr << "[InputSystem] JSON Parse Error: " << e.what() << std::endl;
    return false;
  }
}

std::vector<SimulationAction> InputSystem::PopActionsForCycle(int cycle) {
  std::vector<SimulationAction> actions;

  if (!is_replaying)
    return actions;

  while (playback_index < recorded_actions.size()) {
    const auto &act = recorded_actions[playback_index];
    if (act.cycle == cycle) {
      actions.push_back(act);
      playback_index++;
    } else if (act.cycle < cycle) {
      // Missed action? Skip or apply late?
      // Better to apply late than never, but warns.
      std::cerr << "[InputSystem] WARNING: Missed action at cycle " << act.cycle
                << " (Current: " << cycle << ")" << std::endl;
      playback_index++;
    } else {
      // act.cycle > cycle -> Future action
      break;
    }
  }
  return actions;
}
