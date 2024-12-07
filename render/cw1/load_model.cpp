#include "load_model.hpp"

#include <cassert>
#include <cstring>
#include <iostream>

#include <rapidobj/rapidobj.hpp>
#include <stb_image.h>

#include "../labutils/error.hpp"
#include "simple_model.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkobject.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/ui.hpp"

namespace lut = labutils;


SimpleModel load_simple_wavefront_obj( char const*  aPath )
{
    assert( aPath );

    // Ask rapidobj to load the requested file
    auto result = rapidobj::ParseFile( aPath );
    if( result.error )
        throw lut::Error( "Unable to load OBJ file '%s': %s", aPath, result.error.code.message().c_str() );

    // OBJ files can define faces that are not triangles. However, Vulkan will
    // only render triangles (or lines and points), so we must triangulate any
    // faces that are not already triangles. Fortunately, rapidobj can do this
    // for us.
    rapidobj::Triangulate( result );

    // Find the path to the OBJ file
    char const* pathBeg = aPath;
    char const* pathEnd = std::strrchr( pathBeg, '/' );

    std::string const prefix = pathEnd
                               ? std::string( pathBeg, pathEnd+1 )
                               : ""
    ;

    // Convert the OBJ data into a SimpleModel structure.
    // First, extract material data.
    SimpleModel ret;

    ret.modelSourcePath = aPath;

    for( auto const& mat : result.materials )
    {
        SimpleMaterialInfo mi;

        mi.materialName  = mat.name;
        mi.diffuseColor  = glm::vec3( mat.diffuse[0], mat.diffuse[1], mat.diffuse[2] );

        if( !mat.diffuse_texname.empty() )
            mi.diffuseTexturePath  = prefix + mat.diffuse_texname;

        ret.materials.emplace_back( std::move(mi) );
    }

    // Next, extract the actual mesh data. There are some complications:
    // - OBJ use separate indices to positions, normals and texture coords. To
    //   deal with this, the mesh is turned into an unindexed triangle soup.
    // - OBJ uses three methods of grouping faces:
    //   - 'o' = object
    //   - 'g' = group
    //   - 'usemtl' = switch materials
    //  The first two create logical objects/groups. The latter switches
    //  materials. We want to primarily group faces by material (and possibly
    //  secondarily by other logical groupings).
    //
    // Unfortunately, RapidOBJ exposes a per-face material index.

    std::unordered_set<std::size_t> activeMaterials;
    for( auto const& shape : result.shapes )
    {
        auto const& shapeName = shape.name;

        // Scan shape for materials
        activeMaterials.clear();

        for( std::size_t i = 0; i < shape.mesh.indices.size(); ++i )
        {
            auto const faceId = i/3; // Always triangles; see Triangulate() above

            assert( faceId < shape.mesh.material_ids.size() );
            auto const matId = shape.mesh.material_ids[faceId];

            assert( matId < int(ret.materials.size()) );
            activeMaterials.emplace( matId );
        }


        // Process vertices for active material
        // This does multiple passes over the vertex data, which is less than
        // optimal...
        //
        // Note: we still keep different "shapes" separate. For static meshes,
        // one could merge all vertices with the same material for a bit more
        // efficient rendering.
        for( auto const matId : activeMaterials )
        {
            auto* opos = &ret.dataTextured.positions;
            auto* otex = &ret.dataTextured.texcoords;

            bool const textured = !ret.materials[matId].diffuseTexturePath.empty();
            if( !textured )
            {
                opos = &ret.dataUntextured.positions;
                otex = nullptr;
            }

            // Keep track of mesh names; this can be useful for debugging.
            std::string meshName;
            if( 1 == activeMaterials.size() )
                meshName = shapeName;
            else
                meshName = shapeName + "::" + ret.materials[matId].materialName;

            // Extract this material's vertices.
            auto const firstVertex = opos->size();
            assert( !textured || firstVertex == otex->size() );

            for( std::size_t i = 0; i < shape.mesh.indices.size(); ++i )
            {
                auto const faceId = i/3; // Always triangles; see Triangulate() above
                auto const faceMat = std::size_t(shape.mesh.material_ids[faceId]);

                if( faceMat != matId )
                    continue;

                auto const& idx = shape.mesh.indices[i];

                opos->emplace_back( glm::vec3{
                        result.attributes.positions[idx.position_index*3+0],
                        result.attributes.positions[idx.position_index*3+1],
                        result.attributes.positions[idx.position_index*3+2]
                } );

                if( textured )
                {
                    otex->emplace_back( glm::vec2{
                            result.attributes.texcoords[idx.texcoord_index*2+0],
                            result.attributes.texcoords[idx.texcoord_index*2+1]
                    } );
                }
            }

            auto const vertexCount = opos->size() - firstVertex;
            assert( !textured || vertexCount == otex->size() - firstVertex );

            ret.meshes.emplace_back( SimpleMeshInfo{
                    std::move(meshName),
                    matId,
                    textured,
                    firstVertex,
                    vertexCount
            } );
        }
    }

    return ret;
}

std::vector<glm::vec3> load_triangle_soup(std::string aPath) {
//    assert( aPath );

    std::vector<glm::vec3> positions;
    int j = 0; // counter for every 3 coordinates

    std::ifstream fin(aPath);
    if (!fin.is_open()) {
        std::cerr << "Could not open file" << std::endl;
        exit(1);
    }
    std::string fileContent; //placeholder string for content of file

    getline(fin, fileContent); //first line is n of faces

    double coordinate;
    double coordinateArr[3];
    while (fin >> coordinate)
    {
        coordinateArr[j] = coordinate;
        j++;

        if(j == 3){
            //new vertex
            positions.emplace_back(
                    coordinateArr[0],
                    coordinateArr[1],
                    coordinateArr[2]
            );
            //Next vertex
            j = 0;
        }
    }
    fin.close();

    return positions;
}

/* Loads .xyz file with x y z positions on one line */
std::vector<glm::vec3> load_xyz(std::string aPath) {
//    assert(aPath);
    std::vector<glm::vec3> positions;

    std::ifstream fin(aPath);

    if (!fin.is_open()) {
        std::cerr << "Could not open file" << std::endl;
        exit(1);
    }

    std::string line;
    while (std::getline(fin, line)) {
        std::istringstream iss(line);
        double x, y, z;

        if (!(iss >> x >> y >> z)) {
            std::cerr << "Error parsing line: " << line << std::endl;
            continue;
        }

        positions.emplace_back(x,y,z);
    }
    fin.close();
    return positions;
}

void read_config(const std::string& filename, UiConfiguration& config) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open .config file" << std::endl;
        exit(1);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string key;
        if (iss >> key) {
            if (key == "grid_resolution_min") iss >> const_cast<float&>(config.grid_resolution_min);
            else if (key == "grid_resolution_max") iss >> const_cast<float&>(config.grid_resolution_max);
            else if (key == "grid_resolution") iss >> config.grid_resolution;
            else if (key == "padding") iss >> config.padding;
            else if (key == "point_cloud_size") iss >> config.point_cloud_size;
            else if (key == "isovalue") iss >> config.isovalue;
            else if (key == "target_edge_length") iss >> config.target_edge_length;
            else if (key == "remeshing_iterations") iss >> config.remeshing_iterations;
        }
    }

    file.close();
}

// Function to read ONLY vertex data from an OBJ file. It will ingore everything else
std::vector<glm::vec3> load_obj_vertices(std::string aPath) {
        std::vector<glm::vec3> vertices;
        std::ifstream file(aPath);

        if (!file.is_open()) {
            std::cerr << "Could not open file" << std::endl;
            exit(1);
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v") {
                float x, y, z;
                if (iss >> x >> y >> z) {
                    vertices.emplace_back(x, y, z);
                } else {
                    std::cerr << "Warning: Invalid vertex line format: " << line << std::endl;
                }
            }
        }

        file.close();
        return vertices;
    }



std::vector<glm::vec3> load_file(std::string aPath, char const* aConfigPath, UiConfiguration& ui_config) {
    std::filesystem::path p(aPath);
    std::cout << "Selected file: " << p.filename();
    std::vector<glm::vec3> positions;

    if(p.extension() == ".obj") {
        positions = load_obj_vertices(aPath);
    }

    if(p.extension() == ".tri") {
        positions = load_triangle_soup(aPath);
    }

    if(p.extension() == ".xyz") {
        positions = load_xyz(aPath);
    }

    std::cout << " contains " << positions.size() << " points" << std::endl;

    read_config(aConfigPath, ui_config);

    return(positions);
}



