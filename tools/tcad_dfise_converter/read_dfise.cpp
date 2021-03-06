#include "read_dfise.h"

#include <cassert>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <string>

// Include trim utility from allpix
#include "core/utils/string.h"

std::map<std::string, std::vector<Point>> read_grid(const std::string& file_name) {
    std::ifstream file(file_name);
    if(!file) {
        throw std::runtime_error("file cannot be accessed");
    }

    DFSection main_section = DFSection::HEADER;
    DFSection sub_section = DFSection::NONE;

    std::vector<Point> vertices;
    std::vector<std::pair<long unsigned int, long unsigned int>> edges;
    std::vector<std::vector<long unsigned int>> faces;
    std::vector<std::vector<long unsigned int>> elements;

    std::map<std::string, std::vector<long unsigned int>> regions_vertices;

    std::string region;
    long unsigned int data_count = 0;
    bool in_data_block = false;
    while(!file.eof()) {
        std::string line;
        std::getline(file, line);
        line = allpix::trim(line);
        if(line.empty()) {
            continue;
        }

        // Check if line with begin of section
        if(line.find('{') != std::string::npos) {
            // Search for new simple headers
            auto base_regex = std::regex("([a-zA-Z]+) \\{");
            std::smatch base_match;
            if(std::regex_match(line, base_match, base_regex) && base_match.ready()) {
                auto header_string = base_match[1].str();

                if(header_string == "Info") {
                    main_section = DFSection::INFO;
                } else if(header_string == "Data") {
                    in_data_block = true;
                } else {
                    if(main_section != DFSection::NONE) {
                        sub_section = DFSection::IGNORED;
                    } else {
                        main_section = DFSection::IGNORED;
                    }
                }
            }

            // Search for headers with data
            base_regex = std::regex("([a-zA-Z]+) \\((\\S+)\\) \\{");
            if(std::regex_match(line, base_match, base_regex) && base_match.ready()) {
                auto header_string = base_match[1].str();
                auto header_data = base_match[2].str();

                if(header_string == "Region") {
                    main_section = DFSection::REGION;
                    region = header_data.substr(1, header_data.size() - 2);
                } else if(header_string == "Vertices") {
                    main_section = DFSection::VERTICES;
                    data_count = std::stoul(header_data);
                } else if(header_string == "Edges") {
                    main_section = DFSection::EDGES;
                    data_count = std::stoul(header_data);
                } else if(header_string == "Faces") {
                    main_section = DFSection::FACES;
                    data_count = std::stoul(header_data);
                } else if(header_string == "Elements") {
                    if(main_section == DFSection::REGION) {
                        sub_section = DFSection::ELEMENTS;
                    } else {
                        main_section = DFSection::ELEMENTS;
                    }
                    data_count = std::stoul(header_data);
                } else {
                    if(main_section != DFSection::NONE) {
                        sub_section = DFSection::IGNORED;
                    } else {
                        main_section = DFSection::IGNORED;
                    }
                }
            }

            continue;
        }

        // Look for close of section
        if(line.find('}') != std::string::npos) {
            switch(main_section) {
            case DFSection::VERTICES:
                if(vertices.size() != data_count) {
                    throw std::runtime_error("incorrect number of vertices");
                }
                break;
            case DFSection::EDGES:
                if(edges.size() != data_count) {
                    throw std::runtime_error("incorrect number of vertices");
                }
                break;
            case DFSection::FACES:
                if(faces.size() != data_count) {
                    throw std::runtime_error("incorrect number of faces");
                }
                break;
            case DFSection::ELEMENTS:
                if(elements.size() != data_count) {
                    throw std::runtime_error("incorrect number of elements");
                }
                break;
            default:
                break;
            }

            // Close section
            if(sub_section != DFSection::NONE) {
                sub_section = DFSection::NONE;
            } else if(main_section != DFSection::NONE) {
                main_section = DFSection::NONE;
            } else if(in_data_block) {
                in_data_block = false;
            } else {
                throw std::runtime_error("incorrect nesting of blocks");
            }

            continue;
        }

        // Handle data
        std::stringstream sstr(line);
        switch(main_section) {
        case DFSection::HEADER:
            if(line != "DF-ISE text") {
                throw std::runtime_error("incorrect format, file does not have 'DF-ISE text' header");
            }
        case DFSection::INFO:
            break;
        case DFSection::VERTICES: {
            // Read vertex points
            Point point;
            while(sstr >> point.x >> point.y >> point.z) {
                vertices.push_back(point);
            }

        } break;
        case DFSection::EDGES: {
            // Read edges
            std::pair<long unsigned int, long unsigned int> edge;
            while(sstr >> edge.first >> edge.second) {
                if(edge.first >= vertices.size() || edge.second >= vertices.size()) {
                    throw std::runtime_error("vertex index is higher than number of vertices");
                }
                edges.push_back(edge);
            }
        } break;
        case DFSection::FACES: {
            // Get vertex indices for every face
            size_t n;
            sstr >> n;
            std::vector<long unsigned int> face;
            for(size_t i = 0; i < n; ++i) {
                long edge_idx;
                sstr >> edge_idx;

                bool swap = false;
                if(edge_idx < 0) {
                    edge_idx = -edge_idx - 1;
                    swap = true;
                }

                if(edge_idx >= static_cast<long>(edges.size())) {
                    throw std::runtime_error("edge index is higher than number of edges");
                }

                auto edge = edges[static_cast<size_t>(edge_idx)];

                if(swap) {
                    std::swap(edge.first, edge.second);
                }
                if(!face.empty() && face.back() == edge.second) {
                    std::swap(edge.first, edge.second);
                }

                face.push_back(edge.first);
                face.push_back(edge.second);
            }

            // Check first
            if(face.front() != face.back()) {
                std::swap(face.front(), face.back());
            }

            // Remove duplicates
            auto iter = std::unique(face.begin(), face.end());
            face.erase(iter, face.end());
            face.pop_back();

            faces.push_back(face);
        } break;
        case DFSection::ELEMENTS: {
            int k;
            sstr >> k;
            std::vector<long unsigned int> element;

            size_t size = 0;
            switch(k) {
            case 2: /* triangle */
                size = 3;
                break;
            case 3: /* rectangle */
                size = 4;
                break;
            case 5: /* tetrahedron */
                size = 4;
                break;
            case 6: /* pyramid */
                size = 5;
                break;
            case 7: /* prism */
                size = 5;
                break;
            case 8: /* brick */
                size = 6;
                break;
            default:
                throw std::runtime_error("element type " + std::to_string(k) + " is not supported");
            }

            for(size_t i = 0; i < size; ++i) {
                long face_idx;
                sstr >> face_idx;

                bool reverse = false;
                if(face_idx < 0) {
                    reverse = true;
                    face_idx = -face_idx - 1;
                }

                if(face_idx >= static_cast<long>(faces.size())) {
                    throw std::runtime_error("face index is higher than number of faces");
                }

                auto face = faces[static_cast<size_t>(face_idx)];

                if(reverse) {
                    std::reverse(face.begin() + 1, face.end());
                }

                element.insert(element.end(), face.begin(), face.end());
            }

            elements.push_back(element);
            break;
        }
        case DFSection::REGION: {
            if(sub_section != DFSection::ELEMENTS) {
                continue;
            }
            long unsigned int elem_idx;
            while(sstr >> elem_idx) {
                if(elem_idx >= elements.size()) {
                    throw std::runtime_error("element index is higher than number of elements");
                }

                regions_vertices[region].insert(
                    regions_vertices[region].end(), elements[elem_idx].begin(), elements[elem_idx].end());
            }

        } break;
        default:
            break;
        }
    }

    std::map<std::string, std::vector<Point>> ret_map;
    for(auto& name_region_vertices : regions_vertices) {
        auto region_vertices = name_region_vertices.second;

        std::sort(region_vertices.begin(), region_vertices.end());
        auto iter = std::unique(region_vertices.begin(), region_vertices.end());
        region_vertices.erase(iter, region_vertices.end());

        std::vector<Point> ret_vector;
        for(auto& vertex_idx : region_vertices) {
            ret_vector.push_back(vertices[vertex_idx]);
        }

        ret_map[name_region_vertices.first] = ret_vector;
    }

    return ret_map;
}

std::map<std::string, std::vector<Point>> read_electric_field(const std::string& file_name) {
    std::ifstream file(file_name);
    if(!file) {
        throw std::runtime_error("file cannot be accessed");
    }

    DFSection main_section = DFSection::HEADER;
    DFSection sub_section = DFSection::NONE;

    std::map<std::string, std::vector<Point>> region_electric_field_map;
    std::vector<double> region_electric_field_num;

    std::string region;
    long unsigned int data_count = 0;
    bool in_data_block = false;
    while(!file.eof()) {
        std::string line;
        std::getline(file, line);
        line = allpix::trim(line);
        if(line.empty()) {
            continue;
        }

        // Check if line with begin of section
        if(line.find('{') != std::string::npos) {
            // Search for new simple headers
            auto base_regex = std::regex("([a-zA-Z]+) \\{");
            std::smatch base_match;
            if(std::regex_match(line, base_match, base_regex) && base_match.ready()) {
                auto header_string = base_match[1].str();

                if(header_string == "Info") {
                    main_section = DFSection::INFO;
                } else if(header_string == "Data") {
                    in_data_block = true;
                } else {
                    if(main_section != DFSection::NONE) {
                        sub_section = DFSection::IGNORED;
                    } else {
                        main_section = DFSection::IGNORED;
                    }
                }
            }

            // Search for headers with data
            base_regex = std::regex("([a-zA-Z]+) \\((\\S+)\\) \\{");
            if(std::regex_match(line, base_match, base_regex) && base_match.ready()) {
                auto header_string = base_match[1].str();
                auto header_data = base_match[2].str();

                if(header_string == "Dataset") {
                    std::string data_type = header_data.substr(1, header_data.size() - 2);

                    if(data_type == "ElectricField") {
                        main_section = DFSection::ELECTRIC_FIELD;
                    } else {
                        main_section = DFSection::IGNORED;
                    }
                } else if(header_string == "Values") {
                    sub_section = DFSection::VALUES;
                    data_count = std::stoul(header_data);
                } else {
                    if(main_section != DFSection::NONE) {
                        sub_section = DFSection::IGNORED;
                    } else {
                        main_section = DFSection::IGNORED;
                    }
                }
            }

            continue;
        }

        // Look for close of section
        if(line.find('}') != std::string::npos) {
            if(main_section == DFSection::ELECTRIC_FIELD && sub_section == DFSection::VALUES) {
                if(data_count != region_electric_field_num.size()) {
                    throw std::runtime_error("incorrect number of electric field points");
                }

                for(size_t i = 0; i < region_electric_field_num.size(); i += 3) {
                    auto x = region_electric_field_num[i];
                    auto y = region_electric_field_num[i + 1];
                    auto z = region_electric_field_num[i + 2];
                    region_electric_field_map[region].emplace_back(x, y, z);
                }

                region_electric_field_num.clear();
            }

            // Close section
            if(sub_section != DFSection::NONE) {
                sub_section = DFSection::NONE;
            } else if(main_section != DFSection::NONE) {
                main_section = DFSection::NONE;
            } else if(in_data_block) {
                in_data_block = false;
            } else {
                throw std::runtime_error("incorrect nesting of blocks");
            }

            continue;
        }

        // Look for key data pairs
        if(line.find('=') != std::string::npos) {
            auto base_regex = std::regex("([a-zA-Z]+)\\s+=\\s+([\\S ]+)");
            std::smatch base_match;
            if(std::regex_match(line, base_match, base_regex) && base_match.ready()) {
                auto key = base_match[1].str();
                auto value = allpix::trim(base_match[2].str());

                // Filter correct electric field type
                if(main_section == DFSection::ELECTRIC_FIELD) {
                    if(key == "type" && value != "vector") {
                        main_section = DFSection::IGNORED;
                    }
                    if(key == "dimension" && std::stoul(value) != 3) {
                        main_section = DFSection::IGNORED;
                    }
                    if(key == "location" && value != "vertex") {
                        main_section = DFSection::IGNORED;
                    }
                    if(key == "validity") {
                        // Ignore any electric field valid for multiple regions
                        base_regex = std::regex("\\[\\s+\"(\\w+)\"\\s+\\]");
                        if(std::regex_match(value, base_match, base_regex) && base_match.ready()) {
                            region = base_match[1].str();
                        } else {
                            main_section = DFSection::IGNORED;
                        }
                    }
                }
            }
            continue;
        }

        // Handle data
        if(main_section == DFSection::ELECTRIC_FIELD && sub_section == DFSection::VALUES) {
            std::stringstream sstr(line);
            double num;
            while(sstr >> num) {
                region_electric_field_num.push_back(num);
            }
        }
    }

    return region_electric_field_map;
}
