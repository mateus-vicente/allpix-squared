/**
 * @file
 * @brief Parameters of a hybrid pixel detector model
 *
 * @copyright MIT License
 */

#ifndef ALLPIX_HYBRID_PIXEL_DETECTOR_H
#define ALLPIX_HYBRID_PIXEL_DETECTOR_H

#include <string>
#include <utility>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "DetectorModel.hpp"

namespace allpix {

    /**
     * @ingroup DetectorModels
     * @brief Model of a hybrid pixel detector. This a model where the sensor is bump-bonded to the chip
     */
    class HybridPixelDetectorModel : public DetectorModel {
    public:
        /**
         * @brief Constructs the hybrid pixel detector model
         * @param type Name of the model type
         * @param reader Configuration reader with description of the model
         */
        explicit HybridPixelDetectorModel(std::string type, const ConfigReader& reader)
            : DetectorModel(std::move(type), reader) {
            auto config = reader.getHeaderConfiguration();

            // Set bump parameters
            setBumpCylinderRadius(config.get<double>("bump_cylinder_radius"));
            setBumpHeight(config.get<double>("bump_height"));
            setBumpSphereRadius(config.get<double>("bump_sphere_radius", 0));
            setBumpOffset(config.get<ROOT::Math::XYVector>("bump_offset", {0, 0}));
        }

        /**
         * @brief Get center of the chip in local coordinates
         * @return Center of the chip
         *
         * The center of the chip as given by \ref DetectorModel::getChipCenter() with extra offset for bump bonds.
         */
        ROOT::Math::XYZPoint getChipCenter() const override {
            ROOT::Math::XYZVector offset(0, 0, getBumpHeight());
            return DetectorModel::getChipCenter() + offset;
        }

        /**
         * @brief Return all layers of support
         * @return List of all the support layers
         *
         * The center of the support is adjusted to take the bump bonds into account
         */
        std::vector<SupportLayer> getSupportLayers() const override {
            auto ret_layers = DetectorModel::getSupportLayers();

            for(auto& layer : ret_layers) {
                if(layer.location_ == "chip") {
                    layer.center_.SetZ(layer.center_.z() + getBumpHeight());
                }
            }

            return ret_layers;
        }

        /**
         * @brief Get the center of the bump bonds in local coordinates
         * @return Center of the bump bonds
         *
         * The bump bonds are aligned with the grid with an optional XY-offset. The z-offset is calculated with the sensor
         * and chip offsets taken into account.
         */
        virtual ROOT::Math::XYZPoint getBumpsCenter() const {
            ROOT::Math::XYZVector offset(
                bump_offset_.x(), bump_offset_.y(), getSensorSize().z() / 2.0 + getBumpHeight() / 2.0);
            return getCenter() + offset;
        }
        /**
         * @brief Get the radius of the sphere of every individual bump bond (union solid with cylinder)
         * @return Radius of bump bond sphere
         */
        double getBumpSphereRadius() const { return bump_sphere_radius_; }
        /**
         * @brief Set the radius of the sphere of every individual bump bond  (union solid with cylinder)
         * @param val Radius of bump bond sphere
         */
        void setBumpSphereRadius(double val) { bump_sphere_radius_ = val; }
        /**
         * @brief Get the radius of the cylinder of every individual bump bond  (union solid with sphere)
         * @return Radius of bump bond cylinder
         */
        double getBumpCylinderRadius() const { return bump_cylinder_radius_; }
        /**
         * @brief Set the radius of the cylinder of every individual bump bond  (union solid with sphere)
         * @param val Radius of bump bond cylinder
         */
        void setBumpCylinderRadius(double val) { bump_cylinder_radius_ = val; }
        /**
         * @brief Get the height of the bump bond cylinder, determining the offset between sensor and chip
         * @return Height of the bump bonds
         */
        double getBumpHeight() const { return bump_height_; }
        /**
         * @brief Set the height of the bump bond cylinder, determining the offset between sensor and chip
         * @param val Height of the bump bonds
         */
        void setBumpHeight(double val) { bump_height_ = val; }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setBumpOffset(ROOT::Math::XYVector val) { bump_offset_ = std::move(val); }

    private:
        double bump_sphere_radius_{};
        double bump_height_{};
        ROOT::Math::XYVector bump_offset_;
        double bump_cylinder_radius_{};
    };
} // namespace allpix

#endif /* ALLPIX_HYBRID_PIXEL_DETECTOR_H */
