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
    return ((val / 16u) & 0b1111);
}

unsigned export_kac_1_0_c::reduce_8bit_color_value_to_5bit(const uint8_t val)
{
    return ((val / 8u) & 0b11111);
}

unsigned export_kac_1_0_c::get_texture_side_length_from_exponent(const unsigned exp)
{
    assert(((exp >= 0) && (exp <= 7)) && "The given texture side length exponent is out of bounds.");

    return std::pow(2, (exp + 1));
}

unsigned export_kac_1_0_c::get_exponent_from_texture_side_length(const unsigned len)
{
    assert(((len >= 2) && (len <= 256)) && "The given texture side length is out of bounds.");
    assert(((len & (len - 1)) == 0) && "The given texture side length is not a power of two.");

    return (unsigned(std::log2(len) - 1) & 0x7);
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

bool export_kac_1_0_c::write_ending(void) const
{
    if (this->is_valid_output_stream())
    {
        std::fputs("ENDS", this->file);
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
            const uint16_t packedColor = material.color.r |
                                         (material.color.g << 4) |
                                         (material.color.b << 8) |
                                         (material.color.a << 12);

            std::fwrite((char*)&packedColor, sizeof(packedColor), 1, this->file);

            const uint16_t packedMetadata = ((material.metadata.textureMetadataIdx  & 0x1ff) << 0)  |
                                            ((material.metadata.hasTexture          & 0x1  ) << 9)  |
                                            ((material.metadata.hasTextureFiltering & 0x1  ) << 10) |
                                            ((material.metadata.hasSmoothShading    & 0x1  ) << 11);
                                            
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

bool export_kac_1_0_c::write_texture_metadata(const std::vector<kac_1_0_texture_s> &textures) const
{
    if (this->is_valid_output_stream())
    {
        const uint32_t numTextures = textures.size();

        std::fputs("TXMD", this->file);
        std::fwrite((char*)&numTextures, sizeof(numTextures), 1, this->file);
        
        for (const auto &texture: textures)
        {
            const uint32_t packedMetadata = ((texture.metadata.sideLengthExponent & 0x7)       << 0) |
                                            ((texture.metadata.pixelDataOffset    & 0x1ffffff) << 3);
                                            
            std::fwrite((char*)&packedMetadata, sizeof(packedMetadata), 1, this->file);

            std::fwrite((char*)&texture.metadata.pixelHash, sizeof(texture.metadata.pixelHash), 1, this->file);
        }
    }

    return this->is_valid_output_stream();
}

bool export_kac_1_0_c::write_texture_pixels(const std::vector<kac_1_0_texture_s> &textures) const
{
    if (this->is_valid_output_stream())
    {
        uint32_t numPixels = 0;
        for (const auto &texture: textures)
        {
            const uint32_t textureSideLen = export_kac_1_0_c::get_texture_side_length_from_exponent(texture.metadata.sideLengthExponent);
            const uint32_t textureSize = (textureSideLen * textureSideLen);

            numPixels += textureSize;
        }

        std::fputs("TXPX", this->file);
        std::fwrite((char*)&numPixels, sizeof(numPixels), 1, this->file);

        // Write the textures' individual pixels.
        for (const auto &texture: textures)
        {
            const uint32_t textureSideLen = export_kac_1_0_c::get_texture_side_length_from_exponent(texture.metadata.sideLengthExponent);
            const uint32_t texturePixelCount = (textureSideLen * textureSideLen);

            for (unsigned i = 0; i < texturePixelCount; i++)
            {
                const uint16_t packedColor = (texture.pixels[i].r << 0)  |
                                             (texture.pixels[i].g << 5)  |
                                             (texture.pixels[i].b << 10) |
                                             (texture.pixels[i].a << 15);

                std::fwrite((char*)&packedColor, sizeof(packedColor), 1, this->file);
            }
        }
    }

    return this->is_valid_output_stream();
}
