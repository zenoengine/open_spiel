// Copyright 2019 DeepMind Technologies Ltd. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef THIRD_PARTY_OPEN_SPIEL_ALGORITHMS_EXTERNAL_SAMPLING_MCCFR_H_
#define THIRD_PARTY_OPEN_SPIEL_ALGORITHMS_EXTERNAL_SAMPLING_MCCFR_H_

#include <memory>
#include <random>
#include <vector>

#include "open_spiel/algorithms/cfr.h"
#include "open_spiel/policy.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_optional.h"

// An implementation of external sampling Monte Carlo Counterfactual Regret
// Minimization (CFR). See Lanctot 2009 [0] and Chapter 4 of Lanctot 2013 [1]
// for details.
// [0]: http://mlanctot.info/files/papers/nips09mccfr.pdf
// [1]: http://mlanctot.info/files/papers/PhD_Thesis_MarcLanctot.pdf

namespace open_spiel {
namespace algorithms {

// How to average the strategy. The 'simple' type does the averaging for
// player i + 1 mod num_players on player i's regret update pass; in two players
// this corresponds to the standard implementation (updating the average
// policy at opponent nodes). In n>2 players, this can be a problem for several
// reasons: first, it does not compute the estimate as described by the
// (unbiased) stochastically-weighted averaging in chapter 4 of Lanctot 2013
// commonly used in MCCFR because the denominator (important sampling
// correction) should include all the other sampled players as well so the
// sample reach no longer cancels with reach of the player updating their
// average policy. Second, if one player assigns zero probability to an action
// (leading to a subtree), the average policy of a different player in that
// subtree is no longer updated. Hence, the full averaging does not update the
// average policy in the regret passes but does a separate pass to update the
// average policy. Nevertheless, we set the simple type as the default because
// it is faster, seems to work better empirically, and it matches what was done
// in Pluribus (Brown and Sandholm. Superhuman AI for multiplayer poker.
// Science, 11, 2019).
enum class AverageType {
  kSimple,
  kFull,
};

class ExternalSamplingMCCFRSolver {
 public:
  static constexpr double kInitialTableValues = 0.000001;

  // Creates a solver with a specific seed and average type.
  ExternalSamplingMCCFRSolver(const Game& game, int seed = 0,
                              AverageType avg_type = AverageType::kSimple);

  // Performs one iteration of external sampling MCCFR, updating the regrets
  // and average strategy for all players. This method uses the internal random
  // number generator.
  void RunIteration();

  // Same as above, but uses the specified random number generator instead.
  void RunIteration(std::mt19937* rng);

  // Computes the average policy, containing the policy for all players.
  // The returned policy instance should only be used during the lifetime of
  // the CFRSolver object.
  std::unique_ptr<Policy> AveragePolicy() const {
    return std::unique_ptr<Policy>(
        new CFRAveragePolicy(info_states_, uniform_policy_));
  }

 private:
  double UpdateRegrets(const State& state, int player, std::mt19937* rng);
  void FullUpdateAverage(const State& state,
                         const std::vector<double>& reach_probs);

  std::unique_ptr<Game> game_;
  std::unique_ptr<std::mt19937> rng_;
  AverageType avg_type_;
  CFRInfoStateValuesTable info_states_;
  std::uniform_real_distribution<double> dist_;
  std::shared_ptr<TabularPolicy> uniform_policy_;
};

}  // namespace algorithms
}  // namespace open_spiel

#endif  // THIRD_PARTY_OPEN_SPIEL_ALGORITHMS_EXTERNAL_SAMPLING_MCCFR_H_
