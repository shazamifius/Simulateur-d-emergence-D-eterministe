#pragma once

#include "MondeSED.h"
#include <ctime>
#include <fstream>
#include <string>
#include <vector>

enum class EventType {
  INFO,
  WARNING,
  ERROR,
  DEATH,
  MITOSIS,
  DIFFERENTIATION,
  INVARIANT_FAIL
};

struct LogEntry {
  int cycle;
  EventType type;
  std::string message;
  std::string timestamp;
};

class Validator {
public:
  // --- Invariants & Checks ---
  static bool CheckInvariants(const MondeSED &monde);
  static float CalculateTotalEnergy(const MondeSED &monde);
  static bool CheckBounds(
      const Cellule &cell,
      std::string &reason); // Checks if P, C, L, etc. are within valid ranges

  // --- Logging & Traceability ---
  static void LogEvent(const MondeSED &monde, EventType type,
                       const std::string &message);
  static void SaveLog(const std::string &filename);
  static std::vector<LogEntry> &GetLogHistory();

  // --- Configuration ---
  static void SetLogToFile(bool enable,
                           const std::string &filename = "simulation_log.txt");

private:
  static std::vector<LogEntry> history;
  static bool log_to_file;
  static std::string log_filename;

  static std::string GetCurrentTimestamp();
  static std::string EventTypeToString(EventType type);
};
