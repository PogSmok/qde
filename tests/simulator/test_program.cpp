// Simulator::RunProgram and the growth of a quantum state it relies on.

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "qde/gate_registry.hpp"
#include "qde/program.hpp"
#include "qde/simulator/internal/quantum_state.hpp"
#include "qde/simulator/simulation_circuit.hpp"
#include "qde/simulator/simulation_state.hpp"
#include "qde/simulator/simulator.hpp"

namespace qde::simulator {

namespace {

constexpr double kTol = 1e-9;

// A program given as a function of its backend.
class ScriptedProgram final : public Program {
 public:
  using Script = std::function<ProgramResult(QuantumBackend&)>;

  explicit ScriptedProgram(Script script) : script_(std::move(script)) {}

  [[nodiscard]] ProgramResult Run(QuantumBackend& backend) const override {
    return script_(backend);
  }

 private:
  Script script_;
};

}  // namespace

class ProgramTest : public ::testing::Test {
 protected:
  void Apply(QuantumBackend& backend, const char* gate,
             const std::vector<std::size_t>& qubits) const {
    backend.ApplyGate(*registry_.Find(gate), {}, qubits);
  }

  GateRegistry registry_{GateRegistry::WithBuiltins()};
};

// ---- RunProgram -------------------------------------------------------------

TEST_F(ProgramTest, ProgramWithoutQubitsHasTheOneState) {
  const ScriptedProgram program(
      [](QuantumBackend&) { return ProgramResult{}; });
  const SimulationState state = Simulator::RunProgram(program, 0);
  EXPECT_EQ(state.qubit_count, 0U);
  EXPECT_NEAR(state.Probability(0), 1.0, kTol);
}

TEST_F(ProgramTest, GatesAndMeasurementsReachTheState) {
  const ScriptedProgram program([this](QuantumBackend& backend) {
    backend.AppendQubits(2);
    Apply(backend, "x", {1});
    ProgramResult result;
    result.classical_bits = {
        static_cast<std::uint8_t>(backend.Measure(0) ? 1 : 0),
        static_cast<std::uint8_t>(backend.Measure(1) ? 1 : 0)};
    return result;
  });
  const SimulationState state = Simulator::RunProgram(program, 0);
  EXPECT_NEAR(state.Probability(2), 1.0, kTol);
  EXPECT_EQ(state.classical_bits, (std::vector<std::uint8_t>{0, 1}));
  EXPECT_EQ(state.bit_count, 2U);
}

TEST_F(ProgramTest, ProgramReactsToItsMeasurements) {
  // Flip the qubit back whenever it is measured as 1.
  const ScriptedProgram program([this](QuantumBackend& backend) {
    backend.AppendQubits(1);
    Apply(backend, "h", {0});
    if (backend.Measure(0)) {
      Apply(backend, "x", {0});
    }
    return ProgramResult{};
  });
  for (std::uint64_t seed = 0; seed < 16; ++seed) {
    EXPECT_NEAR(Simulator::RunProgram(program, seed).Probability(0), 1.0, kTol);
  }
}

TEST_F(ProgramTest, SeedFixesTheOutcomes) {
  const ScriptedProgram program([this](QuantumBackend& backend) {
    backend.AppendQubits(3);
    ProgramResult result;
    for (std::size_t qubit = 0; qubit < 3; ++qubit) {
      Apply(backend, "h", {qubit});
      result.classical_bits.push_back(backend.Measure(qubit) ? 1 : 0);
    }
    return result;
  });
  EXPECT_EQ(Simulator::RunProgram(program, 7).classical_bits,
            Simulator::RunProgram(program, 7).classical_bits);
}

TEST_F(ProgramTest, QubitsAddedLaterStartInZero) {
  const ScriptedProgram program([this](QuantumBackend& backend) {
    backend.AppendQubits(1);
    Apply(backend, "x", {0});
    backend.AppendQubits(2);
    Apply(backend, "x", {2});
    return ProgramResult{};
  });
  const SimulationState state = Simulator::RunProgram(program, 0);
  EXPECT_EQ(state.qubit_count, 3U);
  EXPECT_NEAR(state.Probability(5), 1.0, kTol);
}

TEST_F(ProgramTest, ResetOfEntangledQubitMixesTheState) {
  const ScriptedProgram program([this](QuantumBackend& backend) {
    backend.AppendQubits(2);
    Apply(backend, "h", {0});
    Apply(backend, "cx", {0, 1});
    backend.Reset(0);
    backend.AppendQubits(1);
    return ProgramResult{};
  });
  const SimulationState state = Simulator::RunProgram(program, 0);
  EXPECT_FALSE(state.IsPure());
  EXPECT_EQ(state.qubit_count, 3U);
  EXPECT_NEAR(state.Probability(0), 0.5, kTol);
  EXPECT_NEAR(state.Probability(2), 0.5, kTol);
}

TEST_F(ProgramTest, FailedProgramThrows) {
  const ScriptedProgram program([](QuantumBackend&) {
    ProgramResult result;
    result.errors.push_back({3, 0, "division by zero"});
    return result;
  });
  try {
    static_cast<void>(Simulator::RunProgram(program, 0));
    FAIL();
  } catch (const std::runtime_error& e) {
    EXPECT_NE(std::string(e.what()).find("division by zero"),
              std::string::npos);
    EXPECT_NE(std::string(e.what()).find("line 3"), std::string::npos);
  }
}

TEST_F(ProgramTest, TooManyQubitsThrows) {
  const ScriptedProgram program([](QuantumBackend& backend) {
    backend.AppendQubits(kMaxQubits + 1);
    return ProgramResult{};
  });
  EXPECT_THROW(static_cast<void>(Simulator::RunProgram(program, 0)),
               std::runtime_error);
}

// ---- QuantumState::AppendQubits ---------------------------------------------

TEST(QuantumStateGrowth, PureStateKeepsItsAmplitudes) {
  const GateRegistry registry = GateRegistry::WithBuiltins();
  internal::QuantumState state(1);
  state.ApplyGate(registry.Find("h")->Matrix(), {0});
  state.AppendQubits(2);
  EXPECT_EQ(state.QubitCount(), 3U);
  EXPECT_TRUE(state.IsPure());
  const std::vector<double> probabilities = state.Probabilities();
  ASSERT_EQ(probabilities.size(), 8U);
  EXPECT_NEAR(probabilities[0], 0.5, kTol);
  EXPECT_NEAR(probabilities[1], 0.5, kTol);
  EXPECT_NEAR(probabilities[4], 0.0, kTol);
}

TEST(QuantumStateGrowth, MixedStateKeepsItsDensityMatrix) {
  const GateRegistry registry = GateRegistry::WithBuiltins();
  internal::QuantumState state(1);
  state.ApplyGate(registry.Find("h")->Matrix(), {0});
  state.ConvertToDensityMatrix();
  state.AppendQubits(1);
  EXPECT_FALSE(state.IsPure());
  ASSERT_EQ(state.Data().size(), 16U);
  // The coherence between |0> and |1> of the old qubit is still there.
  EXPECT_NEAR(state.Data()[1].real(), 0.5, kTol);
  const std::vector<double> probabilities = state.Probabilities();
  EXPECT_NEAR(probabilities[0], 0.5, kTol);
  EXPECT_NEAR(probabilities[1], 0.5, kTol);
  EXPECT_NEAR(probabilities[2], 0.0, kTol);
}

TEST(QuantumStateGrowth, NewQubitsCanBeUsed) {
  const GateRegistry registry = GateRegistry::WithBuiltins();
  internal::QuantumState state(0);
  state.AppendQubits(2);
  state.ApplyGate(registry.Find("x")->Matrix(), {1});
  EXPECT_NEAR(state.Probabilities()[2], 1.0, kTol);
}

TEST(QuantumStateGrowth, TooManyQubitsThrow) {
  internal::QuantumState state(1);
  EXPECT_THROW(state.AppendQubits(1000), std::length_error);
}

}  // namespace qde::simulator
