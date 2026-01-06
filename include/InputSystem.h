#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// Action Types
enum class ActionType {
  PARAM_CHANGE_FLOAT,
  // PARAM_CHANGE_INT, // For now parameters are mostly floats or handled as
  // such
  BRUSH_ACTION, // Paint/Delete
  SCENARIO_LOAD,
  RESET_WORLD
};

struct SimulationAction {
  int cycle; // Timestamp
  ActionType type;
  std::string target; // "K_D", "BRUSH_SOMA", etc.
  float val1;         // Value or X
  float val2;         // Y (or unused)
  float val3;         // Z (or unused)
  float val4;         // W / Extra

  // JSON Serialization
  nlohmann::json toJson() const {
    return nlohmann::json{{"cycle", cycle},   {"type", (int)type},
                          {"target", target}, {"val1", val1},
                          {"val2", val2},     {"val3", val3},
                          {"val4", val4}};
  }

  static SimulationAction fromJson(const nlohmann::json &j) {
    SimulationAction a;
    a.cycle = j.value("cycle", 0);
    a.type = (ActionType)j.value("type", 0);
    a.target = j.value("target", "");
    a.val1 = j.value("val1", 0.0f);
    a.val2 = j.value("val2", 0.0f);
    a.val3 = j.value("val3", 0.0f);
    a.val4 = j.value("val4", 0.0f);
    return a;
  }
};

class InputSystem {
public:
  InputSystem() = default;

  // Recording API
  void RecordAction(const SimulationAction &action);
  void SaveRecording(const std::string &filename);

  // Playback API
  bool LoadRecording(const std::string &filename);
  // Pop actions for a specific cycle
  std::vector<SimulationAction> PopActionsForCycle(int cycle);

  bool IsReplaying() const { return is_replaying; }
  void SetReplayMode(bool replay) { is_replaying = replay; }

private:
  std::vector<SimulationAction> recorded_actions;

  // For efficient playback:
  // We can keep index or map. Since we access sequentially, an index is fine.
  size_t playback_index = 0;
  bool is_replaying = false;
};

#endif // INPUT_SYSTEM_H
