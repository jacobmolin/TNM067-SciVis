#include <inviwo/core/util/logcentral.h>
#include <modules/opengl/texture/textureutils.h>
#include <modules/tnm067lab1/processors/imageupsampler.h>
#include <modules/tnm067lab1/utils/interpolationmethods.h>
#include <inviwo/core/datastructures/image/layerram.h>
#include <inviwo/core/datastructures/image/layerramprecision.h>
#include <inviwo/core/util/imageramutils.h>

namespace inviwo {

    namespace detail {

        template <typename T>
        void upsample(ImageUpsampler::IntepolationMethod method, const LayerRAMPrecision<T>& inputImage,
                      LayerRAMPrecision<T>& outputImage) {
            using F = typename float_type<T>::type;

            const size2_t inputSize = inputImage.getDimensions();
            const size2_t outputSize = outputImage.getDimensions();

            const T* inPixels = inputImage.getDataTyped();
            T* outPixels = outputImage.getDataTyped();

            auto inIndex = [&inputSize] (auto pos) -> size_t {
                pos = glm::clamp(pos, decltype(pos)(0), decltype(pos)(inputSize - size2_t(1)));
                return pos.x + pos.y * inputSize.x;
            };
            auto outIndex = [&outputSize] (auto pos) -> size_t {
                pos = glm::clamp(pos, decltype(pos)(0), decltype(pos)(outputSize - size2_t(1)));
                return pos.x + pos.y * outputSize.x;
            };

            util::forEachPixel(outputImage, [&] (ivec2 outImageCoords) {
                // outImageCoords: Exact pixel coordinates in the output image currently writing to
                // inImageCoords: Relative coordinates of outImageCoords in the input image, might be
                // between pixels
                dvec2 inImageCoords =
                    ImageUpsampler::convertCoordinate(outImageCoords, inputSize, outputSize);

                T finalColor(0);

                // DUMMY COLOR, remove or overwrite this bellow
                finalColor = inPixels[inIndex(
                    glm::clamp(size2_t(outImageCoords), size2_t(0), size2_t(inputSize - size2_t(1))))];

                switch (method) {
                case ImageUpsampler::IntepolationMethod::PiecewiseConstant: {
                    // Task 6
                    // Update finalColor
                    // TODO: round(), ceil() or floor()???
                    // inPixels contains the color of each pixel in inImage
                    finalColor = inPixels[inIndex(round(inImageCoords))];
                    break;
                }
                case ImageUpsampler::IntepolationMethod::Bilinear: {
                    // Update finalColor

                    // Get pixel

                    // Get neighbouring pixels
                    ivec2 pos0{ floor(inImageCoords) };
                    ivec2 pos1{ ceil(inImageCoords.x), floor(inImageCoords.y) };
                    ivec2 pos2{ floor(inImageCoords.x), ceil(inImageCoords.y) };
                    ivec2 pos3{ ceil(inImageCoords) };

                    // Get Get neighbouring pixels' values and store in v
                    std::array<T, 4> v{
                        inPixels[inIndex(pos0)],
                        inPixels[inIndex(pos1)],
                        inPixels[inIndex(pos2)],
                        inPixels[inIndex(pos3)],
                    };

                    // Get parameterization in x- and y-direction
                    double x_t = inImageCoords.x - pos0.x;
                    double y_t = inImageCoords.y - pos0.y;

                    finalColor = inviwo::TNM067::Interpolation::bilinear(v, x_t, y_t);
                    break;
                }
                case ImageUpsampler::IntepolationMethod::Biquadratic: {
                    // Update finalColor

                    // Get neighbouring pixels
                    ivec2 pos0{ floor(inImageCoords) };
                    ivec2 pos1{ ceil(inImageCoords.x), floor(inImageCoords.y) };
                    ivec2 pos2{ ceil(inImageCoords.x) + 1, floor(inImageCoords.y) };
                    ivec2 pos3{ floor(inImageCoords.x), ceil(inImageCoords.y) };
                    ivec2 pos4{ ceil(inImageCoords) };
                    ivec2 pos5{ ceil(inImageCoords.x) + 1, ceil(inImageCoords.y) };
                    ivec2 pos6{ floor(inImageCoords.x), ceil(inImageCoords.y) + 1 };
                    ivec2 pos7{ ceil(inImageCoords.x), ceil(inImageCoords.y) + 1 };
                    ivec2 pos8{ ceil(inImageCoords.x) + 1, ceil(inImageCoords.y) + 1 };

                    // Get Get neighbouring pixels' values and store in v
                    std::array<T, 9> v{
                        inPixels[inIndex(pos0)],
                        inPixels[inIndex(pos1)],
                        inPixels[inIndex(pos2)],
                        inPixels[inIndex(pos3)],
                        inPixels[inIndex(pos4)],
                        inPixels[inIndex(pos5)],
                        inPixels[inIndex(pos6)],
                        inPixels[inIndex(pos7)],
                        inPixels[inIndex(pos8)],
                    };

                    // Get parameterization in x- and y-direction
                    double x_t = inImageCoords.x - pos0.x;
                    double y_t = inImageCoords.y - pos0.y;

                    finalColor = inviwo::TNM067::Interpolation::biQuadratic(v, x_t / 2.0, y_t / 2.0);
                    break;
                }
                case ImageUpsampler::IntepolationMethod::Barycentric: {
                    // Update finalColor

                    // Get neighbouring pixels
                    ivec2 pos0{ floor(inImageCoords) };
                    ivec2 pos1{ ceil(inImageCoords.x), floor(inImageCoords.y) };
                    ivec2 pos2{ floor(inImageCoords.x), ceil(inImageCoords.y) };
                    ivec2 pos3{ ceil(inImageCoords) };

                    // Get Get neighbouring pixels' values and store in v
                    std::array<T, 4> v{
                        inPixels[inIndex(pos0)],
                        inPixels[inIndex(pos1)],
                        inPixels[inIndex(pos2)],
                        inPixels[inIndex(pos3)],
                    };

                    // Get parameterization in x- and y-direction
                    double x_t = inImageCoords.x - pos0.x;
                    double y_t = inImageCoords.y - pos0.y;

                    finalColor = inviwo::TNM067::Interpolation::barycentric(v, x_t, y_t);


                    break;
                }
                default:
                break;
                }

                outPixels[outIndex(outImageCoords)] = finalColor;
                               });
        }

    }  // namespace detail

    const ProcessorInfo ImageUpsampler::processorInfo_{
        "org.inviwo.imageupsampler",  // Class identifier
        "Image Upsampler",            // Display name
        "TNM067",                     // Category
        CodeState::Experimental,      // Code state
        Tags::None,                   // Tags
    };
    const ProcessorInfo ImageUpsampler::getProcessorInfo() const { return processorInfo_; }

    ImageUpsampler::ImageUpsampler()
        : Processor()
        , inport_("inport", true)
        , outport_("outport", true)
        , interpolationMethod_("interpolationMethod", "Interpolation Method",
                               {
                                   { "piecewiseconstant", "Piecewise Constant (Nearest Neighbor)",
                                   IntepolationMethod::PiecewiseConstant },
                               { "bilinear", "Bilinear", IntepolationMethod::Bilinear },
                               { "biquadratic", "Biquadratic", IntepolationMethod::Biquadratic },
                               { "barycentric", "Barycentric", IntepolationMethod::Barycentric },
                               }) {
        addPort(inport_);
        addPort(outport_);
        addProperty(interpolationMethod_);
    }

    void ImageUpsampler::process() {
        auto inputImage = inport_.getData();
        if (inputImage->getDataFormat()->getComponents() != 1) {
            LogError("The ImageUpsampler processor does only support single channel images");
        }

        auto inSize = inport_.getData()->getDimensions();
        auto outDim = outport_.getDimensions();

        auto outputImage = std::make_shared<Image>(outDim, inputImage->getDataFormat());
        outputImage->getColorLayer()->setSwizzleMask(inputImage->getColorLayer()->getSwizzleMask());
        outputImage->getColorLayer()
            ->getEditableRepresentation<LayerRAM>()
            ->dispatch<void, dispatching::filter::Scalars>([&] (auto outRep) {
            auto inRep = inputImage->getColorLayer()->getRepresentation<LayerRAM>();
            detail::upsample(interpolationMethod_.get(), *(const decltype(outRep))(inRep), *outRep);
                                                           });

        outport_.setData(outputImage);
    }

    dvec2 ImageUpsampler::convertCoordinate(ivec2 outImageCoords, size2_t inputSize, size2_t outputSize) {
        // TODO implement
        // copy of outPutImageCoords
        dvec2 c(outImageCoords);

        // TASK 5: Convert the outImageCoords to its coordinates in the input image
        // Get the scaling factor between inputSize and outputSize
        dvec2 factor = dvec2(inputSize) / dvec2(outputSize);
        c = c * factor;

        return c;
    }

}  // namespace inviwo
