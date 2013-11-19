/*
  Copyright (c) 2013 Uni Research AS

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPM_SIMULATOR_OUTPUT_HPP
#define OPM_SIMULATOR_OUTPUT_HPP

#include <memory>  // unique_ptr, shared_ptr
#include <vector>

struct UnstructuredGrid;

namespace Opm {

// forward definitions
class BlackoilState;
class EclipseGridParser;
class OutputWriter;
namespace parameter { class ParameterGroup; }
class SimulatorTimer;
class WellState;

/**
 * Encapsulate output writing from simulators. This is essentially
 * a function object holding curried arguments to the writing backend
 * which is used when invoked through the event handler (which passes
 * on no arguments on it own).
 */
class SimulatorOutputBase {
protected:
    /**
     * Curry arguments for the output writer. These arguments are passed
     * to the simulator, but is not passed on to the event handler so it
     * need to pick them up from the object members.
     */
    SimulatorOutputBase (const parameter::ParameterGroup& p,
                         std::shared_ptr <EclipseGridParser> parser,
                         std::shared_ptr <UnstructuredGrid> grid,
                         std::shared_ptr <SimulatorTimer> timer,
                         std::shared_ptr <BlackoilState> state,
                         std::shared_ptr <WellState> wellState);

    /**
     * Conversion operator which allows the object to be directly passed
     * into an Event and used as a handler.
     *
     * @see Opm::SimulatorIncompTwophase::timestep_completed
     */
    operator std::function <void ()> ();

    /// Just hold a reference to these objects that are owned elsewhere.
    std::shared_ptr <SimulatorTimer> timer_;
    std::shared_ptr <BlackoilState> reservoirState_;
    std::shared_ptr <WellState> wellState_;

    /// Created locally and destructed together with us
    std::unique_ptr <OutputWriter> writer_;

    /// Call the writers that were created based on the parameters
    virtual void writeOutput ();

    /// Make sure that the simulator state is up to date before writing
    virtual void sync ();

private:
    /// Index of the upcoming reporting time
    std::vector <double>::size_type next_;

    /// Array of times when to write report
    std::vector <double> times_;
};

/**
 * Create an output writer that is coupled to a simulator capable
 * of reading Eclipse deck files. Output will be written only when
 * specified in the deck file.
 *
 * @note
 * This class is a template since there is no fixed interface for
 * simulators, only an implied type class of common method signatures.
 *
 * @example
 * @code{.cpp}
 *  // configuration
 *  ParameterGroup params (argc, argv, false);
 *
 *  // input file
 *  auto deck = make_shared <EclipseGridParser> ( ... );
 *  const GridManager manager (*parser);
 *  auto grid = share_obj (*manager.c_grid ());
 *
 *  // timestep ends up here
 *  auto timer = make_shared <SimulatorTimer> ();
 *
 *  // state ends up here
 *  auto state = make_shared <TwophaseState> ();
 *  auto wellState = make_shared <WellState> ();
 *
 *  // set up simulation
 *  auto sim = make_shared <SimulatorIncompTwophase> (params, *grid, ... );
 *
 *  // use this to dump state to disk
 *  auto output = make_shared <SimulatorOutput> (
 *          params, deck, grid, timer, state, wellState, sim);
 *
 *  // start simulation
 *  sim.run (timer, state, ... )
 * @endcode
 *
 * @todo
 * This functionality could be incorporated directly into a simulator
 * object.
 */
template <typename Simulator>
struct SimulatorOutput : public SimulatorOutputBase {
	SimulatorOutput (const parameter::ParameterGroup& params,
                     std::shared_ptr <EclipseGridParser> parser,
                     std::shared_ptr <UnstructuredGrid> grid,
                     std::shared_ptr <SimulatorTimer> timer,
                     std::shared_ptr <BlackoilState> state,
                     std::shared_ptr <WellState> wellState,
                     std::shared_ptr <Simulator> sim)
        // send all other parameters to base class
        : SimulatorOutputBase (params, parser, grid, timer, state, wellState)

        // store reference to simulator in derived class
        , sim_ (sim) {

        // connect simulation with output writer
        sim.timestep_completed ().add (*this);
    }

protected:
    // forward this request to the simulator
    virtual void sync () { sim_->sync (); }

private:
    /// Reference to the simulator class; needed to ask it to synchronize
    std::shared_ptr <Simulator> sim_;
};

}

#endif /* OPM_SIMULATOR_OUTPUT_HPP */
