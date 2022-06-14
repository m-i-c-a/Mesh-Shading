#include "Loader.hpp"

#include <fstream>
#include <assert.h>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <array>

ObjectBufferData loadObjFile(const std::string& filepath)
{
    std::ifstream file (filepath, std::ifstream::in);
    assert(file.is_open());

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;

    ObjectBufferData objectData;
    std::vector<Vertex> vertexData;
    std::vector<uint32_t> indexData;
    std::unordered_map<std::string, uint32_t> indexLookup;
    uint32_t nextIdx = 0;

    std::string line;
    while (std::getline(file, line))
    {
        std::string header;
        std::istringstream iss(line);
        iss >> header;

        if (header == "v")
        {
            glm::vec3 pos;
            iss >> pos[0] >> pos[1] >> pos[2];
            positions.push_back(pos);
        }
        else if (header == "vt")
        {
            glm::vec2 uv;
            iss >> uv[0] >> uv[1];
            texCoords.push_back(uv);
        }
        else if (header == "vn")
        {
            glm::vec3 norm;
            iss >> norm[0] >> norm[1] >> norm[2];
            normals.push_back(norm);
        }
        else if (header == "f")
        {
            std::array<std::string, 3> faces; 
            iss >> faces[0] >> faces[1] >> faces[2];

            for (uint8_t i = 0; i < faces.size(); i++)
            {
                auto iter = indexLookup.find(faces[i]);
                if (iter != indexLookup.end())
                {
                    objectData.indices.push_back(iter->second);
                }
                else
                {
                    std::istringstream issFace(faces[i]);

                    std::string strPosIdx, strUvIdx, strNormalIdx;
                    std::getline(issFace, strPosIdx, '/');
                    std::getline(issFace, strUvIdx, '/');
                    std::getline(issFace, strNormalIdx, '/');

                    objectData.vertices.emplace_back(positions[std::stoi(strPosIdx)-1], texCoords[std::stoi(strUvIdx)-1], normals[std::stoi(strNormalIdx)-1]);

                    indexLookup.emplace(faces[i], nextIdx);
                    objectData.indices.push_back(nextIdx++);
                }
            }
        }
    }

    return objectData;
}

