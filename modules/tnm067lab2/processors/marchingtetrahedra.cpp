#include <modules/tnm067lab2/processors/marchingtetrahedra.h>
#include <inviwo/core/datastructures/geometry/basicmesh.h>
#include <inviwo/core/datastructures/volume/volumeram.h>
#include <inviwo/core/util/indexmapper.h>
#include <inviwo/core/util/assertion.h>
#include <inviwo/core/network/networklock.h>
#include <modules/tnm067lab1/utils/interpolationmethods.h>
#include <inviwo/core/util/consolelogger.h>

#include <iostream>

namespace inviwo {

    // ------------------ OUR OWN FUNCTIONS AND STRUCTS --------------------

    void MarchingTetrahedra::createTriangle(const TriEdge& edge0, const TriEdge& edge1, const TriEdge& edge2, float iso, const MarchingTetrahedra::Tetrahedra& tetra, MarchingTetrahedra::MeshHelper& mesh) {

        // Interpolation lambda function
        auto interPos = [iso, &tetra] (TriEdge s) -> vec3 {
            return tetra.dataPoints[s.origin].pos + (tetra.dataPoints[s.dest].pos - tetra.dataPoints[s.origin].pos) * ((iso - tetra.dataPoints[s.origin].value) / (tetra.dataPoints[s.dest].value - tetra.dataPoints[s.origin].value));
        };
        // origin.pos + (dest.pos - origin.pos) * (iso - origin.val) / (dest.val - origin.val)

    // Get interpolated vertices
        auto interVert0 = interPos(edge0); // 0->1
        auto interVert1 = interPos(edge1); // 0->2
        auto interVert2 = interPos(edge2); // 0->3

        // Add interpolated vertices and get their indices
        size_t i0{ mesh.addVertex(interVert0, tetra.dataPoints[edge0.origin].index, tetra.dataPoints[edge0.dest].index) };
        size_t i1{ mesh.addVertex(interVert1, tetra.dataPoints[edge1.origin].index, tetra.dataPoints[edge1.dest].index) };
        size_t i2{ mesh.addVertex(interVert2, tetra.dataPoints[edge2.origin].index, tetra.dataPoints[edge2.dest].index) };

        // Add triangle to mesh
        mesh.addTriangle(i0, i1, i2);
    };

    // ----------------------------------------------------------------------

    size_t MarchingTetrahedra::HashFunc::max = 1;

    const ProcessorInfo MarchingTetrahedra::processorInfo_{
        "org.inviwo.MarchingTetrahedra",  // Class identifier
        "Marching Tetrahedra",            // Display name
        "TNM067",                         // Category
        CodeState::Stable,                // Code state
        Tags::None,                       // Tags
    };
    const ProcessorInfo MarchingTetrahedra::getProcessorInfo() const { return processorInfo_; }

    MarchingTetrahedra::MarchingTetrahedra()
        : Processor()
        , volume_("volume")
        , mesh_("mesh")
        , isoValue_("isoValue", "ISO value", 0.5f, 0.0f, 1.0f) {

        addPort(volume_);
        addPort(mesh_);

        addProperty(isoValue_);

        isoValue_.setSerializationMode(PropertySerializationMode::All);

        volume_.onChange([&] () {
            if (!volume_.hasData()) {
                return;
            }
            NetworkLock lock(getNetwork());
            float iso = (isoValue_.get() - isoValue_.getMinValue()) /
                (isoValue_.getMaxValue() - isoValue_.getMinValue());
            const auto vr = volume_.getData()->dataMap_.valueRange;
            isoValue_.setMinValue(static_cast<float>(vr.x));
            isoValue_.setMaxValue(static_cast<float>(vr.y));
            isoValue_.setIncrement(static_cast<float>(glm::abs(vr.y - vr.x) / 50.0));
            isoValue_.set(static_cast<float>(iso * (vr.y - vr.x) + vr.x));
            isoValue_.setCurrentStateAsDefault();
                         });
    }

    void MarchingTetrahedra::process() {
        auto volume = volume_.getData()->getRepresentation<VolumeRAM>();
        MeshHelper mesh(volume_.getData());

        const auto& dims = volume->getDimensions();
        MarchingTetrahedra::HashFunc::max = dims.x * dims.y * dims.z;

        const float iso = isoValue_.get();

        util::IndexMapper3D indexInVolume(dims);

        const static size_t tetrahedraIds[6][4] = { { 0, 1, 2, 5 }, { 1, 3, 2, 5 }, { 3, 2, 5, 7 },
                                                   { 0, 2, 4, 5 }, { 6, 4, 2, 5 }, { 6, 7, 5, 2 } };

        size3_t pos{};
        for (pos.z = 0; pos.z < dims.z - 1; ++pos.z) {
            for (pos.y = 0; pos.y < dims.y - 1; ++pos.y) {
                for (pos.x = 0; pos.x < dims.x - 1; ++pos.x) {
                    // Step 1: create current cell

                    // The DataPoint index should be the 1D-index for the DataPoint in the cell
                    // Use volume->getAsDouble to query values from the volume
                    // Spatial position should be between 0 and 1

                    Cell c;

                    //                    int volumeIndex{0};
                    for (size_t z{ 0 }; z < 2; ++z) {
                        for (size_t y{ 0 }; y < 2; ++y) {
                            for (size_t x{ 0 }; x < 2; ++x) {

                                // Get index for data point
                                int index{ calculateDataPointIndexInCell(ivec3{ x, y, z }) };

                                vec3 posGlobal{ pos.x + x, pos.y + y, pos.z + z };

                                // Get scaled position
                                vec3 scaledPos{ calculateDataPointPos(pos, ivec3{ x, y, z }, dims) };

                                size_t idxInVol{ indexInVolume(posGlobal) };

                                // Add data point to cell
                                c.dataPoints[index] = MarchingTetrahedra::DataPoint{ scaledPos, (float)volume->getAsDouble(posGlobal), idxInVol };

                            }
                        }
                    }

                    // Step 2: Subdivide cell into tetrahedra (hint: use tetrahedraIds)
                    std::vector<Tetrahedra> tetrahedras;
                    for (size_t t{ 0 }; t < 6; ++t) {
                        Tetrahedra tetra{};
                        for (size_t p{ 0 }; p < 4; ++p) {
                            tetra.dataPoints[p] = c.dataPoints[tetrahedraIds[t][p]];
                        }
                        tetrahedras.push_back(tetra);
                    }


                    // Step three: Calculate for tetra case index
                    for (const Tetrahedra& tetrahedra : tetrahedras) {
                        int caseId = 0;

                        for (size_t point{ 0 }; point < 4; ++point) {
                            if (tetrahedra.dataPoints[point].value < iso) {
                                caseId += (int)glm::pow(2.0, (double)point);
                            }
                        }


                        // step four: Extract triangles
                        switch (caseId) {
                        case 0: case 15:
                        break;
                        case 1: case 14: { // Point normals in different directions
                            if (caseId == 1) { // 0->1, 0->2, 0->3
                                createTriangle(TriEdge{ 0, 1 }, TriEdge{ 0, 2 }, TriEdge{ 0, 3 }, iso, tetrahedra, mesh);
                            }
                            else { // 0->1, 0->3, 0->2
                                createTriangle(TriEdge{ 0, 1 }, TriEdge{ 0, 3 }, TriEdge{ 0, 2 }, iso, tetrahedra, mesh);
                            }
                            break;
                        }
                        case 2: case 13: { // Point normals in different directions
                            if (caseId == 2) { // 1->0, 1->3, 1->2
                                createTriangle(TriEdge{ 1, 0 }, TriEdge{ 1, 3 }, TriEdge{ 1, 2 }, iso, tetrahedra, mesh);
                            }
                            else { // 1->0, 1->2, 1->3
                                createTriangle(TriEdge{ 1, 0 }, TriEdge{ 1, 2 }, TriEdge{ 1, 3 }, iso, tetrahedra, mesh);
                            }
                            break;
                        }
                        case 3: case 12: { // Point normals in different directions
                            if (caseId == 3) {
                                // left: 0->3, 1->3, 1->2
                                createTriangle(TriEdge{ 0, 3 }, TriEdge{ 1, 3 }, TriEdge{ 1, 2 }, iso, tetrahedra, mesh);
                                // right: 0->3, 1->2, 0->2
                                createTriangle(TriEdge{ 0, 3 }, TriEdge{ 1, 2 }, TriEdge{ 0, 2 }, iso, tetrahedra, mesh);
                            }
                            else {
                                // left: 0->3, 1->2, 1->3
                                createTriangle(TriEdge{ 0, 3 }, TriEdge{ 1, 2 }, TriEdge{ 1, 3 }, iso, tetrahedra, mesh);
                                // right: 0->3, 0->2, 1->2
                                createTriangle(TriEdge{ 0, 3 }, TriEdge{ 0, 2 }, TriEdge{ 1, 2 }, iso, tetrahedra, mesh);
                            }
                            break;
                        }
                        case 4: case 11: { // Point normals in different directions
                            if (caseId == 4) { // 2->0, 2->1, 2->3
                                createTriangle(TriEdge{ 2, 0 }, TriEdge{ 2, 1 }, TriEdge{ 2, 3 }, iso, tetrahedra, mesh);
                            }
                            else { // 2->0, 2->3, 2->1
                                createTriangle(TriEdge{ 2, 0 }, TriEdge{ 2, 3 }, TriEdge{ 2, 1 }, iso, tetrahedra, mesh);
                            }
                            break;
                        }
                        case 5: case 10: { // Point normals in different directions
                            if (caseId == 5) {
                                // left: 0->3, 0->1, 1->2
                                createTriangle(TriEdge{ 0, 3 }, TriEdge{ 0, 1 }, TriEdge{ 1, 2 }, iso, tetrahedra, mesh);
                                // right: 0->3, 1->2, 2->3
                                createTriangle(TriEdge{ 0, 3 }, TriEdge{ 1, 2 }, TriEdge{ 2, 3 }, iso, tetrahedra, mesh);
                            }
                            else {
                                // left: 0->3, 1->2, 0->1
                                createTriangle(TriEdge{ 0, 3 }, TriEdge{ 1, 2 }, TriEdge{ 0, 1 }, iso, tetrahedra, mesh);
                                // right: 0->3, 2->3, 1->2
                                createTriangle(TriEdge{ 0, 3 }, TriEdge{ 2, 3 }, TriEdge{ 1, 2 }, iso, tetrahedra, mesh);
                            }
                            break;
                        }
                        case 6: case 9: { // Point normals in different directions
                            if (caseId == 6) {
                                // left: 0->2, 0->1, 1->3
                                createTriangle(TriEdge{ 0, 2 }, TriEdge{ 0, 1 }, TriEdge{ 1, 3 }, iso, tetrahedra, mesh);
                                // right: 0->2, 1->3, 3->2
                                createTriangle(TriEdge{ 0, 2 }, TriEdge{ 1, 3 }, TriEdge{ 3, 2 }, iso, tetrahedra, mesh);
                            }
                            else {
                                // left: 0->2, 1->3, 0->1
                                createTriangle(TriEdge{ 0, 2 }, TriEdge{ 1, 3 }, TriEdge{ 0, 1 }, iso, tetrahedra, mesh);
                                // right: 0->2, 3->2, 1->3
                                createTriangle(TriEdge{ 0, 2 }, TriEdge{ 3, 2 }, TriEdge{ 1, 3 }, iso, tetrahedra, mesh);
                            }
                            break;
                        }
                        case 7: case 8: { // Point normals in different directions
                            if (caseId == 7) { // 1->3, 2->3, 0->3
                                createTriangle(TriEdge{ 1, 3 }, TriEdge{ 2, 3 }, TriEdge{ 0, 3 }, iso, tetrahedra, mesh);
                            }
                            else { // 1->3, 0->3, 2->3
                                createTriangle(TriEdge{ 1, 3 }, TriEdge{ 0, 3 }, TriEdge{ 2, 3 }, iso, tetrahedra, mesh);
                            }
                            break;
                        }
                        }
                    }
                }
            }
        }

        mesh_.setData(mesh.toBasicMesh());
    }

    int MarchingTetrahedra::calculateDataPointIndexInCell(ivec3 index3D) {
        // TODO: TASK 5: map 3D index to 1D index
        return 1 * index3D.x + 2 * index3D.y + 4 * index3D.z;
    }

    vec3 MarchingTetrahedra::calculateDataPointPos(size3_t posVolume, ivec3 posCell, ivec3 dims) {
        // TODO: TASK 5: scale DataPoint position with dimensions to be between 0 and 1
        vec3 posVolVec{ posVolume };
        vec3 posCellVec{ posCell };
        vec3 dimsVec{ dims - 1 };   // dims = # of vertices in each dimension
        return vec3{ (posVolVec + posCellVec) / dimsVec };
    }

    MarchingTetrahedra::MeshHelper::MeshHelper(std::shared_ptr<const Volume> vol)
        : edgeToVertex_()
        , vertices_()
        , mesh_(std::make_shared<BasicMesh>())
        , indexBuffer_(mesh_->addIndexBuffer(DrawType::Triangles, ConnectivityType::None)) {
        mesh_->setModelMatrix(vol->getModelMatrix());
        mesh_->setWorldMatrix(vol->getWorldMatrix());
    }

    void MarchingTetrahedra::MeshHelper::addTriangle(size_t i0, size_t i1, size_t i2) {
        IVW_ASSERT(i0 != i1, "i0 and i1 should not be the same value");
        IVW_ASSERT(i0 != i2, "i0 and i2 should not be the same value");
        IVW_ASSERT(i1 != i2, "i1 and i2 should not be the same value");

        indexBuffer_->add(static_cast<glm::uint32_t>(i0));
        indexBuffer_->add(static_cast<glm::uint32_t>(i1));
        indexBuffer_->add(static_cast<glm::uint32_t>(i2));

        const auto a = std::get<0>(vertices_[i0]);
        const auto b = std::get<0>(vertices_[i1]);
        const auto c = std::get<0>(vertices_[i2]);

        const vec3 n = glm::normalize(glm::cross(b - a, c - a));
        std::get<1>(vertices_[i0]) += n;
        std::get<1>(vertices_[i1]) += n;
        std::get<1>(vertices_[i2]) += n;
    }

    std::shared_ptr<BasicMesh> MarchingTetrahedra::MeshHelper::toBasicMesh() {
        for (auto& vertex : vertices_) {
            // Normalize the normal of the vertex
            std::get<1>(vertex) = glm::normalize(std::get<1>(vertex));
        }
        mesh_->addVertices(vertices_);
        return mesh_;
    }

    std::uint32_t MarchingTetrahedra::MeshHelper::addVertex(vec3 pos, size_t i, size_t j) {
        IVW_ASSERT(i != j, "i and j should not be the same value");
        if (j < i) std::swap(i, j);

        auto [edgeIt, inserted] = edgeToVertex_.try_emplace(std::make_pair(i, j), vertices_.size());
        if (inserted) {
            vertices_.push_back({ pos, vec3(0, 0, 0), pos, vec4(0.7f, 0.7f, 0.7f, 1.0f) });
        }
        return static_cast<std::uint32_t>(edgeIt->second);
    }

}  // namespace inviwo
