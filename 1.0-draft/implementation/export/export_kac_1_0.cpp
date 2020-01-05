/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: File writer for the KAC 1.0 data format.
 * 
 * Provides functionality to write data into a KAC 1.0 file in an organized manner.
 * 
 * NOTE: This implementation assumes little-endian byte ordering and 32-bit floats.
 * 
 */

static_assert(sizeof(float) == 4);

#include <cassert>
#include <cmath>
#include "export_kac_1_0.hpp"

export_kac_1_0_c::export_kac_1_0_c(const char *const outputFilename)
{
    this->file = std::fopen(outputFilename, "wb");

    return;
}

bool export_kac_1_0_c::is_valid_output_stream(void) const
{
    return bool(this->file &&
                !std::ferror(this->file));
}

unsigned export_kac_1_0_c::reduce_8bit_color_value_to_1bit(const uint8_t val)
{
    return (bool(val) & 0b1);
}

unsigned export_kac_1_0_c::reduce_8bit_color_value_to_4bit(const uint8_t val)
{
    // Use (255 / 15) instead of (>> 4) for potentially better dynamic range.
    return (unsigned(val / (255 / 15.0)) & 0b1111);
}

unsigned export_kac_1_0_c::reduce_8bit_color_value_to_5bit(const uint8_t val)
{
    // Use (255 / 31) instead of (>> 3) for potentially better dynamic range.
    return (unsigned(val / (255 / 31.0)) & 0b11111);
}

bool export_kac_1_0_c::write_header(void) const
{
    if (this->is_valid_output_stream())
    {
        std::fputs("KAC ", this->file);
        std::fwrite((char*)&this->formatVersion, sizeof(this->formatVersion), 1, this->file);
    }

    return this->is_valid_output_stream();
}

bool export_kac_1_0_c::write_uv_coordinates(const std::vector<kac_1_0_uv_coordinates_s> &uvCoordinates) const
{
    if (this->is_valid_output_stream())
    {
        const uint32_t numUVs = uvCoordinates.size();

        std::fputs("UV  ", this->file);
        std::fwrite((char*)&numUVs, sizeof(numUVs), 1, this->file);

        for (const auto &uv: uvCoordinates)
        {
            std::fwrite((char*)&uv.u, sizeof(uv.u), 1, this->file);
            std::fwrite((char*)&uv.v, sizeof(uv.v), 1, this->file);
        }
    }

    return this->is_valid_output_stream();
}

bool export_kac_1_0_c::write_vertex_coordinates(const std::vector<kac_1_0_vertex_coordinates_s> &vertexCoordinates) const
{
    if (this->is_valid_output_stream())
    {
        const uint32_t numVertices = vertexCoordinates.size();

        std::fputs("VERT", this->file);
        std::fwrite((char*)&numVertices, sizeof(numVertices), 1, this->file);

        for (const auto &vertex: vertexCoordinates)
        {
            std::fwrite((char*)&vertex.x, sizeof(vertex.x), 1, this->file);
            std::fwrite((char*)&vertex.y, sizeof(vertex.y), 1, this->file);
            std::fwrite((char*)&vertex.z, sizeof(vertex.z), 1, this->file);
        }
    }

    return this->is_valid_output_stream();
}

bool export_kac_1_0_c::write_materials(const std::vector<kac_1_0_material_s> &materials) const
{
    if (this->is_valid_output_stream())
    {
        const uint32_t numMaterials = materials.size();

        std::fputs("MATE", this->file);
        std::fwrite((char*)&numMaterials, sizeof(numMaterials), 1, this->file);

        for (const auto &material: materials)
        {
            const uint16_t packedColor = (material.color.r << 0) |
                                         (material.color.g << 4) |
                                         (material.color.b << 8) |
                                         (material.color.a << 12);

            std::fwrite((char*)&packedColor, sizeof(packedColor), 1, this->file);

            const uint32_t packedMetadata = ((material.metadata.textureIdx          & 0xffff) <<  0) |
                                            ((material.metadata.hasTexture          & 0x1   ) << 16) |
                                            ((material.metadata.hasTextureFiltering & 0x1   ) << 17) |
                                            ((material.metadata.hasSmoothShading    & 0x1   ) << 18);
                                            
            std::fwrite((char*)&packedMetadata, sizeof(packedMetadata), 1, this->file);
        }
    }

    return this->is_valid_output_stream();
}

bool export_kac_1_0_c::write_normals(const std::vector<kac_1_0_normal_s> &normals) const
{
    if (this->is_valid_output_stream())
    {
        const uint32_t numNormals = normals.size();

        std::fputs("NORM", this->file);
        std::fwrite((char*)&numNormals, sizeof(numNormals), 1, this->file);

        for (const auto &normal: normals)
        {
            std::fwrite((char*)&normal.x, sizeof(normal.x), 1, this->file);
            std::fwrite((char*)&normal.y, sizeof(normal.y), 1, this->file);
            std::fwrite((char*)&normal.z, sizeof(normal.z), 1, this->file);
        }
    }

    return this->is_valid_output_stream();
}

bool export_kac_1_0_c::write_triangles(const std::vector<kac_1_0_triangle_s> &triangles) const
{
    if (this->is_valid_output_stream())
    {
        const uint32_t numTriangles = triangles.size();

        std::fputs("3MSH", this->file);
        std::fwrite((char*)&numTriangles, sizeof(numTriangles), 1, this->file);

        for (const auto &triangle: triangles)
        {
            std::fwrite((char*)&triangle.materialIdx, sizeof(triangle.materialIdx), 1, this->file);

            for (const auto &vertex: triangle.vertices)
            {
                std::fwrite((char*)&vertex.vertexCoordinatesIdx, sizeof(vertex.vertexCoordinatesIdx), 1, this->file);
                std::fwrite((char*)&vertex.normalIdx, sizeof(vertex.normalIdx), 1, this->file);
                std::fwrite((char*)&vertex.uvIdx, sizeof(vertex.uvIdx), 1, this->file);
            }
        }
    }

    return this->is_valid_output_stream();
}

bool export_kac_1_0_c::write_textures(const std::vector<kac_1_0_texture_s> &textures) const
{
    if (this->is_valid_output_stream())
    {
        const uint32_t numTextures = textures.size();

        std::fputs("TXTR", this->file);
        std::fwrite((char*)&numTextures, sizeof(numTextures), 1, this->file);
        
        for (const auto &texture: textures)
        {
            assert((texture.metadata.sideLength >= KAC_1_0_MIN_TEXTURE_SIDE_LENGTH) &&
                   (texture.metadata.sideLength <= KAC_1_0_MAX_TEXTURE_SIDE_LENGTH) &&
                   ((texture.metadata.sideLength & (texture.metadata.sideLength - 1)) == 0) && // Power of two.
                   "Invalid texture dimensions.");

            // Write the texture's metadata.
            {
                const uint32_t packedParams = (texture.metadata.sideLength & 0xffff);

                std::fwrite((char*)&packedParams, sizeof(packedParams), 1, this->file);
                std::fwrite((char*)&texture.metadata.pixelHash, sizeof(texture.metadata.pixelHash), 1, this->file);
            }

            // Write the texture's pixel data in progressively shrinking levels of mipmapping
            // until we reach the smallest level.
            for (unsigned m = 0; ; m++)
            {
                const uint32_t textureSideLen = (texture.metadata.sideLength / pow(2, m));
                const uint32_t texturePixelCount = (textureSideLen * textureSideLen);

                if (textureSideLen < KAC_1_0_MIN_TEXTURE_SIDE_LENGTH)
                {
                    assert((m > 0) && "At least one level of mipmapping is required.");
                    break;
                }

                assert((m < KAC_1_0_MAX_NUM_MIP_LEVELS) &&
                       "A texture is overflowing the maximum mip level count.");

                for (unsigned p = 0; p < texturePixelCount; p++)
                {
                    const uint16_t packedColor = (texture.mipLevel[m][p].r << 0)  |
                                                 (texture.mipLevel[m][p].g << 5)  |
                                                 (texture.mipLevel[m][p].b << 10) |
                                                 (texture.mipLevel[m][p].a << 15);

                    std::fwrite((char*)&packedColor, sizeof(packedColor), 1, this->file);
                }
            }
        }
    }

    return this->is_valid_output_stream();
}
