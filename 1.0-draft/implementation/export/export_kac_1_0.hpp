/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: File writer for the KAC 1.0 data format.
 * 
 * Provides functionality to write data into a KAC 1.0 file in an organized manner.
 * 
 */

#ifndef EXPORT_KAC_1_0_H
#define EXPORT_KAC_1_0_H

#include <string>
#include <vector>
#include "../kac_1_0_types.h"

class export_kac_1_0_c
{
    public:
        export_kac_1_0_c(const char *const outputFilename);

        // Returns true if the output file stream is currently valid (the file has
        // been opened successfully, there have not been IO errors, etc.).
        bool is_valid_output_stream(void) const;

        // Functionality to write the various KAC 1.0 data segments into the file.
        // For details, refer to the KAC 1.0 specification. The functions return
        // true if the writing succeeded; false otherwise.
        bool write_header(void) const;
        bool write_ending(void) const;
        bool write_normals(const std::vector<kac_1_0_normal_s> &normals) const;
        bool write_materials(const std::vector<kac_1_0_material_s> &materials) const;
        bool write_triangles(const std::vector<kac_1_0_triangle_s> &triangles) const;
        bool write_uv_coordinates(const std::vector<kac_1_0_uv_coordinates_s> &uvCoordinates) const;
        bool write_vertex_coordinates(const std::vector<kac_1_0_vertex_coordinates_s> &vertices) const;
        bool write_texture_pixels(const std::vector<kac_1_0_texture_s> &textures) const;
        bool write_texture_metadata(const std::vector<kac_1_0_texture_s> &textures) const;

        // Utility functions.
        static unsigned reduce_8bit_color_value_to_1bit(const uint8_t val);
        static unsigned reduce_8bit_color_value_to_4bit(const uint8_t val);
        static unsigned reduce_8bit_color_value_to_5bit(const uint8_t val);
        static unsigned get_texture_side_length_from_exponent(const unsigned exp);
        static unsigned get_exponent_from_texture_side_length(const unsigned len);

    private:
        std::FILE *file;
        const float formatVersion = 1.0;
};

#endif
