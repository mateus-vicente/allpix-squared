/**
 * @file
 * @brief Definition of Geant4 deposition module
 * @copyright MIT License
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_H

#include <memory>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "SensitiveDetectorActionG4.hpp"

class G4UserLimits;
class G4RunManager;

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to simulate the particle beam and generating the charge deposits in the sensor
     *
     * A beam is defined at a certain position that propagates a particular particle in a certain direction. When the beam
     * hits the sensor the energy loss is converted to charge deposits using the electron-hole creation energy. The energy
     * deposits are specific for a detector. The module also returns the information of the real particle passage (the
     * MCParticle).
     */
    class DepositionGeant4Module : public Module {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        DepositionGeant4Module(Configuration, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief Initializes the physics list of processes and constructs the particle source
         */
        void init() override;

        /**
         * @brief Deposit charges for a single event
         */
        void run(unsigned int) override;

        /**
         * @brief Display statistical summary
         */
        void finalize() override;

    private:
        Configuration config_;
        Messenger* messenger_;
        GeometryManager* geo_manager_;

        // Handling of the charge deposition in all the sensitive devices
        std::vector<SensitiveDetectorActionG4*> sensors_;

        // Number of the last event
        unsigned int last_event_num_;

        // Class holding the limits for the step size
        std::unique_ptr<G4UserLimits> user_limits_;

        // Pointer to the Geant4 manager (owned by GeometryBuilderGeant4)
        G4RunManager* run_manager_g4_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_H */
