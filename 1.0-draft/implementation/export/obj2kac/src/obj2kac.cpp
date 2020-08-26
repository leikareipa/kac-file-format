/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: OBJ2KAC
 * 
 * Converts Wavefront OBJ files into the KAC 1.0 mesh format.
 * 
 */

#include <QtCore/QCryptographicHash>
#include <QtGui/QImage>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>
#include "../../export_kac_1_0.hpp"
#include "string_utils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.hpp"

// A container for the data that makes up a KAC 1.0 file.
struct kac_1_0_data_s
{
    std::vector<kac_1_0_vertex_coordinates_s> vertexCoords;
    std::vector<kac_1_0_uv_coordinates_s> uvCoords;
    std::vector<kac_1_0_material_s> materials;
    std::vector<kac_1_0_triangle_s> triangles;
    std::vector<kac_1_0_texture_s> textures;
    std::vector<kac_1_0_normal_s> normals;
};

static unsigned nearest_power_of_two(const unsigned value)
{
    return std::pow(2, std::floor(std::log2(value)));
}

// Parses the given OBJ file and fills the given KAC data structure with the
// contents of the OBJ file converted into the KAC 1.0 format. Optionally, a
// base path for the MTL file can be specified; otherwise, the absolute path
// of the OBJ file will be used. Returns true on successful completion; false
// otherwise.
static bool make_kac_data_from_obj(kac_1_0_data_s &kacData,
                                   const std::filesystem::path &objFileName,
                                   std::filesystem::path mtlFilePath = "",
                                   const unsigned maxTextureSideLen = KAC_1_0_MAX_TEXTURE_SIDE_LENGTH)
{
    std::ifstream objFile(objFileName);

    if (!objFile.is_open())
    {
        std::cerr << "Error: Unable to open \"" << objFileName.string() << "\"\n";
        return false;
    }

    if (mtlFilePath.empty())
    {
        mtlFilePath = std::filesystem::absolute(objFileName).parent_path();
    }

    // Load in the OBJ file's data.
    tinyobj::attrib_t tinyAttributes;
    std::vector<tinyobj::shape_t> tinyShapes;
    std::vector<tinyobj::material_t> tinyMaterials;
    {
        // The OBJ importer will fill these in to report any errors/warnings.
        std::string errorMessages;
        std::string warningMessages;

        if (!tinyobj::LoadObj(&tinyAttributes, &tinyShapes, &tinyMaterials,
                              &warningMessages, &errorMessages,
                              objFileName.c_str(), mtlFilePath.c_str()) ||
            !warningMessages.empty() ||
            !errorMessages.empty())
        {
            std::cerr << "ERROR: The OBJ importer reported the following errors/warnings:\n";
            
            if (!errorMessages.empty())
            {
                std::cerr << "Errors:\n";

                const auto errors = obj2kac::string_utils::string_split(errorMessages, '\n');
                for (const auto &message: errors)
                {
                    std::cerr << " - " << message << std::endl;
                }
            }

            if (!warningMessages.empty())
            {
                std::cerr << "Warnings:\n";

                const auto warnings = obj2kac::string_utils::string_split(warningMessages, '\n');
                for (const auto &message: warnings)
                {
                    std::cerr << " - " << message << std::endl;
                }
            }

            return false;
        }
    }

    // Convert the OBJ's vertex data into the KAC format.
    {
        // Attributes returned by the OBJ loader are stored in a flat array; so
        // when an attribute consists of e.g. three components (XYZ or the like),
        // the array needs to be recomposed into the three-component format. This
        // helper function fills the given non-flat container with data from the
        // given flat attribute array; 'element' defines the type of data structure
        // to be operated on (represents one element of the container's type) and
        // 'componentPtrs' gives a list of the individual components to organize
        // the flat array into.
        const auto vertex_attribute_getter = [](const auto &attribute, auto &container,
                                                auto &element, const auto &componentPtrs)
        {
            assert((attribute.size() &&
                   (attribute.size() % componentPtrs.size() == 0))
                   && "Unexpected number of components in attribute."
                   && "Each polygon must have three vertices, a normal, and UV coordinates.\n");

            for (unsigned i = 0; i < attribute.size();)
            {
                for (auto *const component: componentPtrs)
                {
                    *component = attribute[i++];
                }
                
                container.push_back(element);
            }

            return;
        };

        kac_1_0_vertex_coordinates_s vc;
        vertex_attribute_getter(tinyAttributes.vertices, kacData.vertexCoords, vc,
                                std::list<float*>{&vc.x, &vc.y, &vc.z});

        kac_1_0_normal_s normal;
        vertex_attribute_getter(tinyAttributes.normals, kacData.normals, normal,
                                std::list<float*>{&normal.x, &normal.y, &normal.z});

        kac_1_0_uv_coordinates_s uv;
        vertex_attribute_getter(tinyAttributes.texcoords, kacData.uvCoords, uv,
                                std::list<float*>{&uv.u, &uv.v});
    }

    // Convert the OBJ's material data into the KAC format. The material data can
    // optionally include one or more texture maps, which we'll convert also.
    {
        if (tinyMaterials.empty())
        {
            std::cerr << "ERROR: The OBJ/MTL file is required to define at least one material.\n";
            return false;
        }

        for (const auto &tinyMaterial: tinyMaterials)
        {
            kac_1_0_material_s kacMaterial;

            kacMaterial.color.r = export_kac_1_0_c::reduce_8bit_color_value_to_4bit(unsigned(tinyMaterial.diffuse[0] * 255));
            kacMaterial.color.g = export_kac_1_0_c::reduce_8bit_color_value_to_4bit(unsigned(tinyMaterial.diffuse[1] * 255));
            kacMaterial.color.b = export_kac_1_0_c::reduce_8bit_color_value_to_4bit(unsigned(tinyMaterial.diffuse[2] * 255));
            kacMaterial.color.a = export_kac_1_0_c::reduce_8bit_color_value_to_4bit(255); // OBJ doesn't support alpha.

            // Note: We only recognize OBJ's diffuse textures.
            kacMaterial.metadata.hasTexture = !tinyMaterial.diffuse_texname.empty();

            // We'll smooth-shade all faces by default; but if any face in the mesh
            // that's using this material asks for flat shading, this parameter will
            // be set to false for the entire material.
            kacMaterial.metadata.hasSmoothShading = true;

            // If the material has a texture, convert its pixel data into KAC's
            // 16-bit 5551 format.
            if (!tinyMaterial.diffuse_texname.empty())
            {
                QImage texture(tinyMaterial.diffuse_texname.c_str());

                if (texture.isNull())
                {
                    std::cerr << "ERROR: Failed to load texture \"" << tinyMaterial.diffuse_texname << "\"\n";
                    return false;
                }

                unsigned textureWidth = texture.width();
                unsigned textureHeight = texture.height();

                // Clamp the texture's resolution to the range supported.
                textureWidth = std::max(KAC_1_0_MIN_TEXTURE_SIDE_LENGTH, std::min(maxTextureSideLen, textureWidth));
                textureHeight = std::max(KAC_1_0_MIN_TEXTURE_SIDE_LENGTH, std::min(maxTextureSideLen, textureHeight));

                // Resize non-square and non-power-of-two textures into a square whose side length
                // is the nearest power of two of the longer of the texture's original sides.
                if ((textureWidth != textureHeight) ||
                    ((textureWidth & (textureWidth - 1)) != 0)) // <- Test for non-power-of-two.
                {
                    const unsigned largestSideLen = std::max(textureWidth, textureHeight);
                    const unsigned newSideLen = nearest_power_of_two(largestSideLen);

                    textureWidth = textureHeight = newSideLen;
                }

                // Downscale or upscale the texture image as needed to fit the required resolution.
                if ((unsigned(texture.width()) != textureWidth) ||
                    (unsigned(texture.height()) != textureHeight))
                {
                    std::cout << "Resizing texture \"" << tinyMaterial.diffuse_texname
                              << "\" from " << texture.width() << "x" << texture.height()
                              << " to " << textureWidth << "x" << textureHeight << "\n";

                    texture = texture.scaled(textureWidth, textureHeight,
                                             Qt::IgnoreAspectRatio,
                                             Qt::SmoothTransformation);
                }

                texture = texture.mirrored(false, true);

                // This assumes that textures will be exported into the KAC file in the same
                // order as they are added into the kacData.textures container; and that
                // duplicate textures are not merged.
                kacMaterial.metadata.textureIdx = kacData.textures.size();

                kac_1_0_texture_s kacTexture;

                // The base texture side length at mip level 0.
                kacTexture.metadata.sideLength = texture.width();

                // These parameters can't be specified via an OBJ file; so let's just invent
                // reasonable defaults.
                kacTexture.metadata.sampleLinearly = 1;
                kacTexture.metadata.clampUV = 0;

                // Save the texture's pixels. We'll generate and save successively smaller
                // levels of mipmapping, from the texture's base size down to 1 x 1.
                for (unsigned m = 0; ; m++)
                {
                    const bool textureHasAlpha = texture.hasAlphaChannel();
                    const uint32_t mipLevelSideLength = (kacTexture.metadata.sideLength / pow(2, m));

                    if (mipLevelSideLength > unsigned(texture.width()))
                    {
                        std::cerr << "ERROR: Incorrect texture mip level dimensions.\n";
                        return false;
                    }

                    if (mipLevelSideLength < KAC_1_0_MIN_TEXTURE_SIDE_LENGTH)
                    {
                        if (!m)
                        {
                            std::cerr << "ERROR: Could not generate mip levels for texture \""
                                        << tinyMaterial.diffuse_texname << "\".\n";
                            return false;
                        }

                        break;
                    }

                    if (m >= KAC_1_0_MAX_NUM_MIP_LEVELS)
                    {
                        std::cerr << "ERROR: Too many mip levels for texture \""
                                    << tinyMaterial.diffuse_texname << "\".\n";
                        return false;
                    }

                    // Downscale the texture image to the next mip level.
                    if (mipLevelSideLength != unsigned(texture.width()))
                    {
                        texture = texture.scaledToWidth(mipLevelSideLength, Qt::SmoothTransformation);
                    }

                    if ((unsigned(texture.width()) != mipLevelSideLength) ||
                        (unsigned(texture.height()) != mipLevelSideLength))
                    {
                        std::cerr << "ERROR: Invalid mip level dimensions for texture \""
                                    << tinyMaterial.diffuse_texname << "\".\n";
                        return false;
                    }

                    kacTexture.mipLevel[m] = new kac_1_0_texture_s::kac_1_0_texture_pixel_s[texture.width() * texture.height()];

                    for (int y = 0; y < texture.height(); y++)
                    {
                        for (int x = 0; x < texture.width(); x++)
                        {
                            const QColor pixel(texture.pixelColor(x, y));

                            const unsigned texIdx = (x + y * texture.width());
                            kacTexture.mipLevel[m][texIdx].r = export_kac_1_0_c::reduce_8bit_color_value_to_5bit(pixel.red());
                            kacTexture.mipLevel[m][texIdx].g = export_kac_1_0_c::reduce_8bit_color_value_to_5bit(pixel.green());
                            kacTexture.mipLevel[m][texIdx].b = export_kac_1_0_c::reduce_8bit_color_value_to_5bit(pixel.blue());
                            kacTexture.mipLevel[m][texIdx].a = export_kac_1_0_c::reduce_8bit_color_value_to_1bit(textureHasAlpha? pixel.alpha() : 255);
                        }
                    }
                }

                // Create a hash of the texture's pixel data at mip level 0.
                {
                    const unsigned pixelDataByteSize = (texture.width() * texture.height() * 2);

                    const QByteArray pixelData((const char*)kacTexture.mipLevel[0], pixelDataByteSize);
                    QByteArray hash = QCryptographicHash::hash(pixelData, QCryptographicHash::Sha256);
                    hash.resize(16);

                    memcpy((char*)&kacTexture.metadata.pixelHash, hash.constData(), hash.length());
                }

                kacData.textures.push_back(kacTexture);
            }

            kacData.materials.push_back(kacMaterial);
        }
    }

    // Convert the OBJ's polygon meshes into the KAC format.
    {
        for (const auto &shape: tinyShapes)
        {
            unsigned idx = 0; // Running count of which index we're at in the master list of vertex data.
            const unsigned numFaces = shape.mesh.num_face_vertices.size();

            for (unsigned f = 0; f < numFaces; f++)
            {
                if (shape.mesh.num_face_vertices[f] != 3)
                {
                    std::cerr << "ERROR: Encountered a polygon with fewer or more than three vertices\n";
                    return false;
                }

                kac_1_0_triangle_s kacTriangle;

                // Assign the material index.
                {
                    if (shape.mesh.material_ids.empty())
                    {
                        std::cerr << "ERROR: No OBJ material IDs found\n";
                        return false;
                    }

                    if (shape.mesh.material_ids[f] >= (int)kacData.materials.size())
                    {
                        std::cerr << "ERROR: Encountered an out-of-bounds OBJ Material ID\n";
                        return false;
                    }

                    kacTriangle.materialIdx = shape.mesh.material_ids[f];
                }

                // If any face using a particular material requests smooth shading to
                // be off, smooth shading will be disabled for the entire material.
                if (shape.mesh.smoothing_group_ids[f] == 0)
                {
                    kacData.materials[shape.mesh.material_ids[f]].metadata.hasSmoothShading = false;
                }

                // Assign the vertex, normal, and UV indices.
                for (unsigned i = 0; i < shape.mesh.num_face_vertices[f]; (i++, idx++))
                {
                    if ((shape.mesh.indices[idx].normal_index < 0) ||
                        (shape.mesh.indices[idx].texcoord_index < 0) ||
                        (shape.mesh.indices[idx].vertex_index < 0))
                    {
                        std::cerr << "ERROR: Encountered a vertex that has no normal, UV coordinates, or/nor world coordinates.\n";
                        return false;
                    }

                    if ((shape.mesh.indices[idx].normal_index > std::numeric_limits<uint16_t>::max()) ||
                        (shape.mesh.indices[idx].texcoord_index > std::numeric_limits<uint16_t>::max()) ||
                        (shape.mesh.indices[idx].vertex_index > std::numeric_limits<uint16_t>::max()))
                    {
                        std::cerr << "ERROR: Encountered an out-of-bounds vertex index.\n";
                        return false;
                    }

                    kacTriangle.vertices[i].vertexCoordinatesIdx = shape.mesh.indices[idx].vertex_index;
                    kacTriangle.vertices[i].normalIdx = shape.mesh.indices[idx].normal_index;
                    kacTriangle.vertices[i].uvIdx = shape.mesh.indices[idx].texcoord_index;
                }

                kacData.triangles.push_back(kacTriangle);
            }
        }
    }

    return true;
}

int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 4)
    {
        std::cout << "Usage: obj2kac <input filename> <output filename> [<max texture side length>]\n";
        return 1;
    }

    unsigned maxTextureSideLength = KAC_1_0_MAX_TEXTURE_SIDE_LENGTH;

    if (argc == 4)
    {
        const unsigned cliMaxTextureSize = strtoul(argv[3], 0, 10);
        
        maxTextureSideLength = std::max(KAC_1_0_MIN_TEXTURE_SIDE_LENGTH,
                                        std::min(KAC_1_0_MAX_TEXTURE_SIDE_LENGTH, cliMaxTextureSize));
    }

    if (!std::filesystem::exists(argv[1]))
    {
        std::cerr << "ERROR: The input file does not appear to exist\n";
        return 1;
    }

    kac_1_0_data_s kacData;
    if (!make_kac_data_from_obj(kacData, std::filesystem::absolute(argv[1]), "", maxTextureSideLength))
    {
        std::cerr << "Failed to convert the input file\n";
        return 1;
    }

    export_kac_1_0_c kacFile(argv[2]);
    if (!kacFile.write_header() ||
        !kacFile.write_normals(kacData.normals) ||
        !kacFile.write_uv_coordinates(kacData.uvCoords) ||
        !kacFile.write_vertex_coordinates(kacData.vertexCoords) ||
        !kacFile.write_triangles(kacData.triangles) ||
        !kacFile.write_materials(kacData.materials) ||
        !kacFile.write_textures(kacData.textures))
    {
        std::cerr << "Failed to write the output file\n";
        return 1;
    }

    return 0;
}
