/**
 * Special implementation file which add the necessary functions for the dynamic loading to perform correctly
 *
 * Needs the following defines
 * - ALLPIX_MODULE_NAME: name of the module
 * - ALLPIX_MODULE_HEADER: name of the header defining the module
 * - ALLPIX_MODULE_UNIQUE: true if the module is unique, false otherwise
 *
 * Defines the following functions
 * - generator(): should return an instantiation of a module
 * - is_unique(): should return if module is unique or not
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_NAME
#error "This header should only be automatically included during the build"
#endif

#include <memory>
#include <utility>

#include "core/config/Configuration.hpp"
#include "core/geometry/Detector.hpp"
#include "core/utils/log.h"

#include ALLPIX_MODULE_HEADER

namespace allpix {
    class Messenger;
    class GeometryManager;

    extern "C" {
    bool allpix_module_is_unique();

#if ALLPIX_MODULE_UNIQUE
    // Function that dynamically instantiates an unique module
    Module* allpix_module_generator(Configuration config, Messenger* messenger, GeometryManager* geometry);
    Module* allpix_module_generator(Configuration config, Messenger* messenger, GeometryManager* geometry) {
        auto module = new ALLPIX_MODULE_NAME(std::move(config), messenger, geometry); // NOLINT
        return static_cast<Module*>(module);
    }

    // Function that return the type of this module
    // FIXME: possibly we want a more generic approach later
    bool allpix_module_is_unique() { return true; }
#else
    // Function that dynamically instantiates a detector module
    Module* allpix_module_generator(Configuration config, Messenger* messenger, std::shared_ptr<Detector> detector);
    Module* allpix_module_generator(Configuration config, Messenger* messenger, std::shared_ptr<Detector> detector) {
        auto module = new ALLPIX_MODULE_NAME(std::move(config), messenger, std::move(detector)); // NOLINT
        return static_cast<Module*>(module);
    }

    // Function that return the type of this module
    // FIXME: possibly we want a more generic approach later
    bool allpix_module_is_unique() { return false; }
#endif
    }
}