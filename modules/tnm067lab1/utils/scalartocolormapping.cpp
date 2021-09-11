#include <modules/tnm067lab1/utils/scalartocolormapping.h>
#include <inviwo/core/util/consolelogger.h>

namespace inviwo {

    void ScalarToColorMapping::clearColors() { baseColors_.clear(); }
    void ScalarToColorMapping::addBaseColors(vec4 color) { baseColors_.push_back(color); }

    vec4 ScalarToColorMapping::sample(float t) const {
        if (baseColors_.size() == 0) return vec4(t);
        if (baseColors_.size() == 1) return vec4(baseColors_[0]);
        if (t <= 0) return vec4(baseColors_.front());
        if (t >= 1) return vec4(baseColors_.back());

        // TODO: use t to select which two base colors to interpolate in-between

        // Get t according to size of baseColors
        float t_new = t * (baseColors_.size() - 1);

        int colorIndex = static_cast<int>(t_new);   // Get index by casting to int
        vec4 firstColor = baseColors_[colorIndex];
        vec4 secondColor = baseColors_[colorIndex + 1];

        // TODO: Interpolate colors in baseColors_ and set dummy color to result
        // vec4 m = (secondColor - firstColor);
        // vec4 f0 = firstColor;
        // float x = static_cast<float>(colorIndex);

        // Adjust scale factor t_new to be in between the points colorIndex and colorIndex + 1
        t_new -= colorIndex;

        // vec4 finalColor(t, t, t, 1);  // dummy color
        // vec4 finalColor = f0 + m * x;

        // Interpolate
        return vec4((1 - t_new) * firstColor + t_new * secondColor);
    }

}  // namespace inviwo
