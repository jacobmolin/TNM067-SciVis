#include <modules/tnm067lab4/jacobian.h>

namespace inviwo {

    namespace util {

        mat2 jacobian(const ImageSampler& sampler, vec2 position, vec2 offset) {
            vec2 ox(offset.x, 0);
            vec2 oy(0, offset.y);
            mat2 J;
            // TASK 1: Calculate the Jacobian J
            // ox = delta x
            // oy = delta y

            // Why are we using sampler? What is it?
            vec2 dVdx = (sampler.sample(position + ox) - sampler.sample(position - ox)) / (2.0 * offset.x);
            vec2 dVdy = (sampler.sample(position + oy) - sampler.sample(position - oy)) / (2.0 * offset.y);

            // J is a matrix

            // mat2 J{ dVdx, dVdy };
            // mat2 J{ dVdx.x, dVdx.y, dVdy.x, dVdy.y };
            // J[0][0] = dVdx.x;
            // J[0][1] = dVdx.y;
            // J[1][0] = dVdy.x;
            // J[1][1] = dVdy.y;

            J[0] = dVdx;
            J[1] = dVdy;
            return J;
        }

    }  // namespace util

}  // namespace inviwo
