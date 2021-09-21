#include <modules/tnm067lab2/processors/hydrogengenerator.h>
#include <inviwo/core/datastructures/volume/volume.h>
#include <inviwo/core/util/volumeramutils.h>
#include <modules/base/algorithm/dataminmax.h>
#include <inviwo/core/util/indexmapper.h>
#include <inviwo/core/datastructures/volume/volumeram.h>
#include <modules/base/algorithm/dataminmax.h>

namespace inviwo {

const ProcessorInfo HydrogenGenerator::processorInfo_{
    "org.inviwo.HydrogenGenerator",  // Class identifier
    "Hydrogen Generator",            // Display name
    "TNM067",                        // Category
    CodeState::Stable,               // Code state
    Tags::CPU,                       // Tags
};

const ProcessorInfo HydrogenGenerator::getProcessorInfo() const { return processorInfo_; }

HydrogenGenerator::HydrogenGenerator()
    : Processor(), volume_("volume"), size_("size_", "Volume Size", 16, 4, 256) {
    addPort(volume_);
    addProperty(size_);
}

void HydrogenGenerator::process() {
    auto vol = std::make_shared<Volume>(size3_t(size_), DataFloat32::get());

    auto ram = vol->getEditableRepresentation<VolumeRAM>();
    auto data = static_cast<float*>(ram->getData());
    util::IndexMapper3D index(ram->getDimensions());

    util::forEachVoxel(*ram, [&](const size3_t& pos) {
        vec3 cartesian = idTOCartesian(pos);
        data[index(pos)] = static_cast<float>(eval(cartesian));
    });

    auto minMax = util::volumeMinMax(ram);
    vol->dataMap_.dataRange = vol->dataMap_.valueRange = dvec2(minMax.first.x, minMax.second.x);

    volume_.setData(vol);
}

vec3 HydrogenGenerator::cartesianToSpherical(vec3 cartesian) {
    vec3 sph{cartesian};

    // TASK 1: implement conversion using the equations in the lab script
    sph.x = glm::sqrt(glm::pow(cartesian.x, 2.0) + glm::pow(cartesian.y, 2.0) + glm::pow(cartesian.z, 2.0));
    
    // Avoid division with 0
    if(glm::abs(sph.x) < 0.0000001) return vec3{0.0};
    
    sph.y = glm::acos(cartesian.z / sph.x);
    
    sph.z = glm::atan(cartesian.y, cartesian.x); // std::atan only gives a result in 1st or 4th quadrant
    
    return sph;
}

double HydrogenGenerator::eval(vec3 cartesian) {
//    const double density = cartesian.x;

    vec3 sph{cartesianToSpherical(cartesian)};
    
    // Constants given in lab compendium
    const double Z{1.0};
    const double a0{1.0};
    
    /* TASK 2: Evaluate wave function */
    double eq1{1.0 / (81 * glm::sqrt(6.0 * M_PI))};
    double eq2{glm::pow(Z / a0, 3.0 / 2.0)}; // glm::pow(z / a0, 3/2), z = a0 = 1
    double eq3{glm::pow((Z * sph.x)/a0, 2.0)}; // (Z^2 * r^2) / a0^2
    double eq4{glm::exp(-Z * sph.x / (3.0 * a0))}; // exp(-Zr / 3*a0)
    double eq5{3.0 * glm::pow(glm::cos(sph.y), 2.0) - 1.0}; // 3cos^2(theta) - 1
    
    
    
    return glm::pow(eq1 * eq2 * eq3 * eq4 * eq5, 2.0);
}

vec3 HydrogenGenerator::idTOCartesian(size3_t pos) {
    vec3 p(pos);
    p /= size_ - 1;
    return p * (36.0f) - 18.0f;
}

}  // namespace inviwo
