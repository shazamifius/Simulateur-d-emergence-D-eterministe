#include "Validator.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

std::vector<LogEntry> Validator::history;
bool Validator::log_to_file = false;
std::string Validator::log_filename = "simulation_log.txt";

// --- Invariants & Checks ---

bool Validator::CheckInvariants(const MondeSED &monde) {
  bool all_valid = true;

  // Check global bounds and energy consistency if needed
  // For now, we iterate cells and check local bounds
  // const auto &grille = monde.getGrille(); // REMOVED
  int bad_cells = 0;

  const auto &chunks = monde.getWorldMap().GetAllChunks();
  for (const auto &pair : chunks) {
    const Chunk *chk = pair.second.get();
    if (!chk)
      continue;

    for (const auto &cell : chk->cells) {
      if (cell.is_alive) {
        std::string reason;
        if (!CheckBounds(cell, reason)) {
          bad_cells++;
          all_valid = false;
          // Log first few failures detailed
          if (bad_cells <= 5) {
            LogEvent(monde, EventType::INVARIANT_FAIL,
                     "Cell Validation Failed: " + reason);
          }
        }
      }
    }
  }

  if (!all_valid) {
    std::stringstream ss;
    ss << "Invariant Failure: " << bad_cells << " cells out of bounds.";
    LogEvent(monde, EventType::INVARIANT_FAIL, ss.str());
  }

  return all_valid;
}

float Validator::CalculateTotalEnergy(const MondeSED &monde) {
  double total_E = 0.0; // Use double to avoid accumulation error
  // const auto &grille = monde.getGrille(); // REMOVED
  const auto &chunks = monde.getWorldMap().GetAllChunks();
  for (const auto &pair : chunks) {
    const Chunk *chk = pair.second.get();
    if (chk) {
      for (const auto &cell : chk->cells) {
        if (cell.is_alive) {
          total_E += cell.E;
        }
      }
    }
  }
  return static_cast<float>(total_E);
}

bool Validator::CheckBounds(const Cellule &cell, std::string &reason) {
  // E should generally be >= 0 (though debt model might allow transient issues,
  // let's say hard limit 0) P should be [-1, 1] C, D, L usually >= 0

  if (std::isnan(cell.E) || std::isinf(cell.E)) {
    reason = "E is NaN/Inf";
    return false;
  }
  if (cell.P < -1.1f || cell.P > 1.1f) {
    reason = "P out of bounds [-1.1, 1.1]: " + std::to_string(cell.P);
    return false;
  }
  if (std::isnan(cell.P)) {
    reason = "P is NaN";
    return false;
  }
  if (cell.C < -0.1f) {
    reason = "C < -0.1: " + std::to_string(cell.C);
    return false;
  }

  return true;
}

// --- Logging & Traceability ---

void Validator::LogEvent(const MondeSED &monde, EventType type,
                         const std::string &message) {
  LogEntry entry;
  entry.cycle = monde.getCycleActuel();
  entry.type = type;
  entry.message = message;
  entry.timestamp = GetCurrentTimestamp();

  history.push_back(entry);

  // Optional: Print to console for immediate feedback during dev
  if (type == EventType::ERROR || type == EventType::INVARIANT_FAIL) {
    std::cout << "[CYCLE " << entry.cycle << "] " << EventTypeToString(type)
              << ": " << message << std::endl;
  }

  if (log_to_file) {
    std::ofstream outfile;
    outfile.open(log_filename, std::ios_base::app); // Append mode
    if (outfile.is_open()) {
      outfile << "[" << entry.timestamp << "] [Cycle " << entry.cycle << "] "
              << EventTypeToString(type) << ": " << message << "\n";
      outfile.close();
    }
  }
}

void Validator::SaveLog(const std::string &filename) {
  std::ofstream outfile(filename);
  if (outfile.is_open()) {
    for (const auto &entry : history) {
      outfile << "[" << entry.timestamp << "] [Cycle " << entry.cycle << "] "
              << EventTypeToString(entry.type) << ": " << entry.message << "\n";
    }
    outfile.close();
  }
}

std::vector<LogEntry> &Validator::GetLogHistory() { return history; }

void Validator::SetLogToFile(bool enable, const std::string &filename) {
  log_to_file = enable;
  if (!filename.empty()) {
    log_filename = filename;
  }
  // Create/Clear file if enabling
  if (enable) {
    std::ofstream outfile(log_filename);
    outfile << "--- Simulation Log Started " << GetCurrentTimestamp()
            << " ---\n";
    outfile.close();
  }
}

// --- Helpers ---

std::string Validator::GetCurrentTimestamp() {
  auto now = std::time(nullptr);
  auto tm_info = std::localtime(&now);
  char buffer[20];
  std::strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", tm_info);
  return std::string(buffer);
}

std::string Validator::EventTypeToString(EventType type) {
  switch (type) {
  case EventType::INFO:
    return "INFO";
  case EventType::WARNING:
    return "WARNING";
  case EventType::ERROR:
    return "ERROR";
  case EventType::DEATH:
    return "DEATH";
  case EventType::MITOSIS:
    return "MITOSIS";
  case EventType::DIFFERENTIATION:
    return "DIFFERENTIATION";
  case EventType::INVARIANT_FAIL:
    return "INVARIANT_FAIL";
  default:
    return "UNKNOWN";
  }
}
