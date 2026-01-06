#ifndef ILAW_H
#define ILAW_H

#include <string>

/**
 * @interface ILaw
 * @brief Base interface for all laws in the simulation.
 */
class ILaw {
public:
  virtual ~ILaw() = default;
  virtual std::string GetName() const = 0;
};

#endif // ILAW_H
