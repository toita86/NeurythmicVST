#include "MatsuokaEngine.h"
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

static void printHeader(const std::string& mode) {
  std::cout << "\n# ==================================\n";
  std::cout << "# MODE: " << mode << "\n";
  std::cout << "# ==================================\n";
  std::cout << "# step,time_sec,node,velocity\n";
}

struct RunStats {
  std::map<unsigned, unsigned> eventCount;
  std::map<unsigned, std::vector<double>> eventTimes;

  void record(unsigned nodeID, double timeSec) {
    eventCount[nodeID]++;
    eventTimes[nodeID].push_back(timeSec);
  }

  void printSummary(const std::string& label) {
    std::cout << "\n# --- Summary: " << label << " ---\n";
    std::cout << "# node,events,avg_interval_sec,est_freq_hz\n";
    for (const auto& [nodeID, times] : eventTimes) {
      if (times.size() < 2)
        continue;
      double total = 0.0;
      for (size_t i = 1; i < times.size(); ++i)
        total += times[i] - times[i - 1];
      double avgInterval = total / (times.size() - 1);
      double estFreq = 1.0 / avgInterval;
      std::cout << "# " << nodeID << "," << times.size() << "," << std::fixed
                << std::setprecision(3) << avgInterval << "," << std::fixed
                << std::setprecision(2) << estFreq << "\n";
    }
  }
};

static RunStats runNetwork(bool quantiserOn) {
  /*
  We run at 44100Hz to have enough timing resolution to see quantiser shifts. At
  1000Hz (the library default), the quantiser's ~0.7ms shift is less than one
  step and invisible.
  */
  constexpr unsigned SAMPLE_RATE = 44100;
  constexpr double DURATION_SEC = 5.0;
  constexpr unsigned TOTAL_STEPS = SAMPLE_RATE * DURATION_SEC;

  RunStats stats;

  /*
  Internally, this:
  - Creates a CPG with 1 node (root, node ID 0, default 1Hz)
  - Creates a QuantisedEventQueue with two grids (24th and 32nd), both synced to
  node 0's tempo
  - Nodes 1-15 are pre-configured with _32nd grid and multiple=8 (default
  quantiser is ON for all children!)
          */
  MatsuokaEngine engine(SAMPLE_RATE);

  /*
  Runs the frequency compensation routine so setNodeFrequency() works
  accurately. Without this, asking for 2Hz might give you 1.7Hz or 2.4Hz — the
  oscillator parameters don't directly encode frequency.
  */
  engine.calibrate();

  engine.addChild(0, 1);     // node 1 is child of root
  engine.addChild(0, 2);     // node 2 is child of root
  engine.doQueuedActions();  // <-- THIS applies the additions
  /* 
	addChild is a QUEUED ACTION. Until doQueuedActions(), the network still has only node 0. After, it has 3 nodes. doQueuedActions() is the gate.
	*/

  engine.setNodeFrequency(0, 2.0, false);
  engine.setNodeFrequency(1, 2.7, false);
  engine.setNodeFrequency(2, 3.3, false);
  engine.setConnection(0, 1, 0.15);
  engine.setConnection(0, 2, 0.15);
  engine.doQueuedActions();
	/*
	Why 2.7Hz and 3.3Hz: These are deliberately NOT integer multiples of 2Hz. If we used 3Hz and 4Hz with strong coupling (0.5), the entrainment would force the children to lock to 2:1 and 3:2 ratios, and their events would naturally fall on grid positions — the quantiser would do nothing visible. Non-integer ratios + weak coupling = the CPG's organic rhythm is visible, and the quantiser's grid-snapping stands out.

	Why inherit=false: If true, changing the root frequency would proportionally change children. We want independent control.
	*/

  if (quantiserOn) {
    auto grid = MatsuokaEngine::gridType::_32nd;
    engine.setNodeQuantiser_Grid(1, grid);
    engine.setQuantiseAmount(1, 1.0f);
    engine.setNodeQuantiser_Grid(2, grid);
    engine.setQuantiseAmount(2, 1.0f);
    engine.doQueuedActions();
  } else {
    engine.setNodeQuantiser_Grid(1, MatsuokaEngine::gridType::unQuantised);
    engine.setNodeQuantiser_Grid(2, MatsuokaEngine::gridType::unQuantised);
    engine.doQueuedActions();
  }
	/*
	This explicit OFF is critical — the QuantisedEventQueue constructor sets _32nd for all child nodes by default. Without this, "OFF" is indistinguishable from "ON."
	Node 0 is never quantised — setNodeGrid(0, ...) is hardcoded to return early. Node 0 is the free-running tempo reference that the grids sync to.
	*/

  for (unsigned step = 0; step < TOTAL_STEPS; ++step) {
    engine.step();
    auto events = engine.getEvents();

    double timeSec = static_cast<double>(step) / SAMPLE_RATE;

    for (const auto& e : events) {
      std::cout << step << "," << std::fixed << std::setprecision(3) << timeSec
                << "," << e.nodeID << "," << e.velocity << "\n";
      stats.record(e.nodeID, timeSec);
    }
  }
	/*
	1. step() — what actually happens inside:
	step():
		_cpg.step() // advance all 3 oscillators by 1 sample
								// for each node:
								//   apply inputs from connections
								//   apply self-noise (if set)
								//   run RK4 integration (X1, X2, V1, V2)
								//   detect zero crossings → raw events

		_quantiser.tick() // advance both grids (24th + 32nd) by 1 sample
											// grid position = {gridline, phase}

		fillOutputs() // for each raw event from CPG:
									//   _quantiser.addEvent(e)
									//     if quantised: snap to nearest gridline
									//     if unquantised: use current position
									//     event goes into sorted delay queue
									//
									//   outputEvent ready = _quantiser.getNote()
									//     checks if any delayed event has "matured"
									//     if yes: pop from queue, add to _outputs
									//     if no: skip
	2. getEvents() returns _outputs — a vector of {nodeID, velocity}. Usually empty; only non-empty when a node fired at this sample.
	3. Time conversion: step 44100 = exactly 1.0 seconds.
	4. Output: CSV line per fired event. In OFF mode, events scatter across non-grid times. In ON mode, every timestamp is an integer multiple of 15.625ms (one 32nd note).
	*/

  return stats;
}

int main() {
  printHeader("QUANTISER OFF (free CPG rhythm)");
  auto statsOff = runNetwork(false);
  statsOff.printSummary("QUANTISER OFF");

  printHeader("QUANTISER ON (strict 32nd grid)");
  auto statsOn = runNetwork(true);
  statsOn.printSummary("QUANTISER ON");

  std::cout << "\n# NOTE: Nodes 1+ default to _32nd quantiser.\n";
  std::cout << "# The OFF run explicitly disables it with 'unQuantised'.\n";
  std::cout << "# Compare the summary freq columns -- quantised events\n";
  std::cout << "# should lock to integer subdivisions of the root tempo.\n";
  return 0;
}
