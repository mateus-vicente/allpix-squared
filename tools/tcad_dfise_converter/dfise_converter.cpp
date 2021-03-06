#include "dfise_converter.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <climits>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include <Eigen/Eigen>

#include "../../src/core/utils/log.h"

#include "Octree.hpp"

#include "read_dfise.h"

std::pair<Point, bool> barycentric_interpolation(Point query_point,
                                                 std::vector<Point> tetra_vertices,
                                                 std::vector<Point> tetra_vertices_field,
                                                 double tetra_volume) {

    // Algorithm variables
    bool volume_signal = true;
    bool sub_1_signal, sub_2_signal, sub_3_signal, sub_4_signal;
    Eigen::Matrix4d matrix_sub1, matrix_sub2, matrix_sub3, matrix_sub4;
    double tetra_subvol_1, tetra_subvol_2, tetra_subvol_3, tetra_subvol_4;
    // Return variable. Point(interpolated electric field x, y, z)
    Point efield_int;
    bool flag = true;
    std::pair<Point, bool> efield_valid;

    // Function must have tetra_vertices.size() = 4
    if(tetra_vertices.size() != 4) {
        throw std::invalid_argument("Baricentric interpolation without only 4 vertices!");
    }
    if(tetra_volume > 0) {
        volume_signal = true;
    }
    if(tetra_volume < 0) {
        volume_signal = false;
    }

    // Jacobi Matrix for volume calculation for each tetrahedron with a vertex replaced by the query point
    matrix_sub1 << 1, 1, 1, 1, query_point.x, tetra_vertices[1].x, tetra_vertices[2].x, tetra_vertices[3].x, query_point.y,
        tetra_vertices[1].y, tetra_vertices[2].y, tetra_vertices[3].y, query_point.z, tetra_vertices[1].z,
        tetra_vertices[2].z, tetra_vertices[3].z;
    tetra_subvol_1 = (matrix_sub1.determinant()) / 6;
    if(tetra_subvol_1 > 0) {
        sub_1_signal = true;
    }
    if(tetra_subvol_1 < 0) {
        sub_1_signal = false;
    }
    if(tetra_subvol_1 == 0) {
        sub_1_signal = volume_signal;
    }

    matrix_sub2 << 1, 1, 1, 1, tetra_vertices[0].x, query_point.x, tetra_vertices[2].x, tetra_vertices[3].x,
        tetra_vertices[0].y, query_point.y, tetra_vertices[2].y, tetra_vertices[3].y, tetra_vertices[0].z, query_point.z,
        tetra_vertices[2].z, tetra_vertices[3].z;
    tetra_subvol_2 = (matrix_sub2.determinant()) / 6;
    if(tetra_subvol_2 > 0) {
        sub_2_signal = true;
    }
    if(tetra_subvol_2 < 0) {
        sub_2_signal = false;
    }
    if(tetra_subvol_2 == 0) {
        sub_2_signal = volume_signal;
    }

    matrix_sub3 << 1, 1, 1, 1, tetra_vertices[0].x, tetra_vertices[1].x, query_point.x, tetra_vertices[3].x,
        tetra_vertices[0].y, tetra_vertices[1].y, query_point.y, tetra_vertices[3].y, tetra_vertices[0].z,
        tetra_vertices[1].z, query_point.z, tetra_vertices[3].z;
    tetra_subvol_3 = (matrix_sub3.determinant()) / 6;
    if(tetra_subvol_3 > 0) {
        sub_3_signal = true;
    }
    if(tetra_subvol_3 < 0) {
        sub_3_signal = false;
    }
    if(tetra_subvol_3 == 0) {
        sub_3_signal = volume_signal;
    }

    matrix_sub4 << 1, 1, 1, 1, tetra_vertices[0].x, tetra_vertices[1].x, tetra_vertices[2].x, query_point.x,
        tetra_vertices[0].y, tetra_vertices[1].y, tetra_vertices[2].y, query_point.y, tetra_vertices[0].z,
        tetra_vertices[1].z, tetra_vertices[2].z, query_point.z;
    tetra_subvol_4 = (matrix_sub4.determinant()) / 6;
    if(tetra_subvol_4 > 0) {
        sub_4_signal = true;
    }
    if(tetra_subvol_4 < 0) {
        sub_4_signal = false;
    }
    if(tetra_subvol_4 == 0) {
        sub_4_signal = volume_signal;
    }

    // Electric field interpolation
    efield_int.x = (tetra_subvol_1 * tetra_vertices_field[0].x + tetra_subvol_2 * tetra_vertices_field[1].x +
                    tetra_subvol_3 * tetra_vertices_field[2].x + tetra_subvol_4 * tetra_vertices_field[3].x) /
                   tetra_volume;
    efield_int.y = (tetra_subvol_1 * tetra_vertices_field[0].y + tetra_subvol_2 * tetra_vertices_field[1].y +
                    tetra_subvol_3 * tetra_vertices_field[2].y + tetra_subvol_4 * tetra_vertices_field[3].y) /
                   tetra_volume;
    efield_int.z = (tetra_subvol_1 * tetra_vertices_field[0].z + tetra_subvol_2 * tetra_vertices_field[1].z +
                    tetra_subvol_3 * tetra_vertices_field[2].z + tetra_subvol_4 * tetra_vertices_field[3].z) /
                   tetra_volume;

    // Check if query point is outside tetrahedron
    if(sub_1_signal != volume_signal || sub_2_signal != volume_signal || sub_3_signal != volume_signal ||
       sub_4_signal != volume_signal) {
        flag = false;
        LOG(DEBUG) << "Warning: Point outside tetrahedron";
        efield_valid = std::make_pair(efield_int, flag);
        return efield_valid;
    }

    for(size_t i = 0; i < tetra_vertices.size(); i++) {
        auto distance = unibn::L2Distance<Point>::compute(tetra_vertices[i], query_point);
        LOG(DEBUG) << "Tetrahedron vertex (" << tetra_vertices[i].x << ", " << tetra_vertices[i].y << ", "
                   << tetra_vertices[i].z << ") - "
                   << " Distance: " << distance << " - Electric field: (" << tetra_vertices_field[i].x << ", "
                   << tetra_vertices_field[i].y << ", " << tetra_vertices_field[i].z << ").";
    }
    LOG(DEBUG) << "Tetra full volume: " << tetra_volume << std::endl
               << "Tetra sub volume 1: " << tetra_subvol_1 << std::endl
               << "Tetra sub volume 2: " << tetra_subvol_2 << std::endl
               << "Tetra sub volume 3: " << tetra_subvol_3 << std::endl
               << "Tetra sub volume 4: " << tetra_subvol_4 << std::endl
               << "Volume difference: "
               << tetra_volume - (tetra_subvol_1 + tetra_subvol_2 + tetra_subvol_3 + tetra_subvol_4);
    LOG(DEBUG) << "Interpolated electric field: (" << efield_int.x << "," << efield_int.y << "," << efield_int.z << ")";

    efield_valid = std::make_pair(efield_int, flag);
    return efield_valid;
}

void interrupt_handler(int) {
    LOG(STATUS) << "Interrupted! Aborting conversion...";
    allpix::Log::finish();
    std::exit(0);
}

int main(int argc, char** argv) {
    // If no arguments are provided, print the help:
    bool print_help = false;
    int return_code = 0;
    if(argc == 1) {
        print_help = true;
        return_code = 1;
    }

    // Add stream and set default logging level
    allpix::Log::addStream(std::cout);
    allpix::Log::setReportingLevel(allpix::LogLevel::INFO);

    // Install abort handler (CTRL+\) and interrupt handler (CTRL+C)
    std::signal(SIGQUIT, interrupt_handler);
    std::signal(SIGINT, interrupt_handler);

    std::string file_prefix;
    std::string init_file_prefix;
    std::string log_file_name;
    std::string region = "bulk";                          // Sensor bulk region name on DF-ISE file
    float volume_cut = std::numeric_limits<float>::min(); // Enclosing tetrahedron should have volume != 0
    size_t index_cut = 10000000;                          // Permutation index initial cut
    bool index_cut_flag = false;
    float initial_radius = 1; // Neighbour vertex search radius
    float radius_step = 0.5;  // Search radius increment
    float max_radius = 10;    // Maximum search radiuss
    int xdiv = 100;           // New mesh X pitch
    int ydiv = 100;           // New mesh Y pitch
    int zdiv = 100;           // New mesh Z pitch

    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-h") == 0) {
            print_help = true;
        } else if(strcmp(argv[i], "-v") == 0 && (i + 1 < argc)) {
            try {
                allpix::LogLevel log_level = allpix::Log::getLevelFromString(std::string(argv[++i]));
                allpix::Log::setReportingLevel(log_level);
            } catch(std::invalid_argument& e) {
                LOG(ERROR) << "Invalid verbosity level \"" << std::string(argv[i]) << "\", ignoring overwrite";
                return_code = 1;
            }
        } else if(strcmp(argv[i], "-f") == 0 && (i + 1 < argc)) {
            file_prefix = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-o") == 0 && (i + 1 < argc)) {
            init_file_prefix = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-R") == 0 && (i + 1 < argc)) {
            region = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-r") == 0 && (i + 1 < argc)) {
            initial_radius = static_cast<float>(strtod(argv[++i], nullptr));
        } else if(strcmp(argv[i], "-s") == 0 && (i + 1 < argc)) {
            radius_step = static_cast<float>(strtod(argv[++i], nullptr));
        } else if(strcmp(argv[i], "-m") == 0 && (i + 1 < argc)) {
            max_radius = static_cast<float>(strtod(argv[++i], nullptr));
        } else if(strcmp(argv[i], "-i") == 0 && (i + 1 < argc)) {
            index_cut = static_cast<size_t>(strtod(argv[++i], nullptr));
            index_cut_flag = true;
        } else if(strcmp(argv[i], "-c") == 0 && (i + 1 < argc)) {
            volume_cut = static_cast<float>(strtod(argv[++i], nullptr));
        } else if(strcmp(argv[i], "-x") == 0 && (i + 1 < argc)) {
            xdiv = static_cast<int>(strtod(argv[++i], nullptr));
        } else if(strcmp(argv[i], "-y") == 0 && (i + 1 < argc)) {
            ydiv = static_cast<int>(strtod(argv[++i], nullptr));
        } else if(strcmp(argv[i], "-z") == 0 && (i + 1 < argc)) {
            zdiv = static_cast<int>(strtod(argv[++i], nullptr));
        } else if(strcmp(argv[i], "-l") == 0 && (i + 1 < argc)) {
            log_file_name = std::string(argv[++i]);
        } else {
            LOG(ERROR) << "Unrecognized command line argument or missing value \"" << argv[i] << "\"";
            print_help = true;
            return_code = 1;
        }
    }

    if(file_prefix.empty()) {
        print_help = true;
        return_code = 1;
    }

    if(init_file_prefix.empty()) {
        init_file_prefix = file_prefix;
        auto sep_idx = init_file_prefix.find_last_of('/');
        if(sep_idx != std::string::npos) {
            init_file_prefix = init_file_prefix.substr(sep_idx + 1);
        }
    }

    // Print help if requested or no arguments given
    if(print_help) {
        std::cerr << "Usage: ./tcad_dfise_reader -f <file_name> [<options>]" << std::endl;
        std::cout << "\t -f <file_prefix>       common prefix of DF-ISE grid (.grd) and data (.dat) files" << std::endl;
        std::cout << "\t -o <init_file_prefix>  output file prefix without .init (defaults to file name of <file_prefix>)"
                  << std::endl;
        std::cout << "\t -R <region>            region name to be meshed (defaults to 'bulk')" << std::endl;
        std::cout << "\t -r <radius>            initial node neighbors search radius in um (defaults to 1 um)" << std::endl;
        std::cout << "\t -s <radius_step>       radius step if no neighbor is found (defaults to 0.5 um)" << std::endl;
        std::cout << "\t -m <max_radius>        maximum search radius (default is 10 um)" << std::endl;
        std::cout << "\t -i <index_cut>         index cut during permutation on vertex neighbours (disabled by default)"
                  << std::endl;
        std::cout << "\t -c <volume_cut>        minimum volume for tetrahedron for non-coplanar vertices (defaults to "
                     "minimum float value)"
                  << std::endl;
        std::cout << "\t -x <mesh x_pitch>      new regular mesh X pitch (defaults to 100)" << std::endl;
        std::cout << "\t -y <mesh_y_pitch>      new regular mesh Y pitch (defaults to 100)" << std::endl;
        std::cout << "\t -z <mesh_z_pitch>      new regular mesh Z pitch (defaults to 100)" << std::endl;
        std::cout << "\t -l <file>              file to log to besides standard output (disabled by default)" << std::endl;
        std::cout << "\t -v <level>             verbosity level (default reporiting level is INFO)" << std::endl;

        allpix::Log::finish();
        return return_code;
    }

    // NOTE: this stream should be available for the duration of the logging
    std::ofstream log_file;
    if(!log_file_name.empty()) {
        log_file.open(log_file_name, std::ios_base::out | std::ios_base::trunc);
        if(!log_file.good()) {
            LOG(FATAL) << "Cannot write to provided log file! Check if permissions are sufficient.";
            allpix::Log::finish();
            return 1;
        }
        allpix::Log::addStream(log_file);
    }

    auto start = std::chrono::system_clock::now();

    LOG(STATUS) << "Reading mesh grid from grid file";
    std::string grid_file = file_prefix + ".grd";

    std::vector<Point> points;
    try {
        auto region_grid = read_grid(grid_file);
        points = region_grid[region];
    } catch(std::runtime_error& e) {
        LOG(FATAL) << "Failed to parse grid file " << grid_file;
        LOG(FATAL) << " " << e.what();
        allpix::Log::finish();
        return 1;
    }

    LOG(STATUS) << "Reading electric field from data file";
    std::string data_file = file_prefix + ".dat";
    std::vector<Point> field;
    try {
        auto region_fields = read_electric_field(data_file);
        field = region_fields[region];
    } catch(std::runtime_error& e) {
        LOG(FATAL) << "Failed to parse data file " << data_file;
        LOG(FATAL) << " " << e.what();
        allpix::Log::finish();
        return 1;
    }

    if(points.size() != field.size()) {
        LOG(FATAL) << "Field and grid file do not match";
        allpix::Log::finish();
        return 1;
    }

    /* ALERT fix coordinates */
    for(unsigned int i = 0; i < points.size(); ++i) {
        std::swap(points[i].y, points[i].z);
        std::swap(field[i].y, field[i].z);
    }

    // Find minimum and maximum from mesh coordinates
    double minx = DBL_MAX, miny = DBL_MAX, minz = DBL_MAX;
    double maxx = DBL_MIN, maxy = DBL_MIN, maxz = DBL_MIN;
    for(auto& point : points) {
        minx = std::min(minx, point.x);
        miny = std::min(miny, point.y);
        minz = std::min(minz, point.z);

        maxx = std::max(maxx, point.x);
        maxy = std::max(maxy, point.y);
        maxz = std::max(maxz, point.z);
    }

    /*
     * ALERT invert the z-axis to match the ap2 system
     * WARNING this will remove the right-handedness of the coordinate system!
     */
    for(size_t i = 0; i < points.size(); ++i) {
        points[i].z = maxz - (points[i].z - minz);
        field[i].z = -field[i].z;
    }

    auto end = std::chrono::system_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    LOG(INFO) << "Reading the files took " << elapsed_seconds << " seconds.";

    LOG(STATUS) << "Starting regular grid interpolation";
    // Initializing the Octree with points from mesh cloud.
    unibn::Octree<Point> octree;
    octree.initialize(points);

    // Creating a new mesh points cloud with a regular pitch
    std::vector<Point> e_field_new_mesh;
    double xstep = (maxx - minx) / static_cast<double>(xdiv);
    double ystep = (maxy - miny) / static_cast<double>(ydiv);
    double zstep = (maxz - minz) / static_cast<double>(zdiv);

    double x = minx + xstep / 2.0;
    for(int i = 0; i < xdiv; ++i) {
        double y = miny + ystep / 2.0;
        for(int j = 0; j < ydiv; ++j) {
            double z = minz + zstep / 2.0;
            for(int k = 0; k < zdiv; ++k) {
                Point q(x, y, z); // New mesh vertex
                Point e(x, y, z); // Corresponding, to be interpolated, electric field
                bool flag = false;
                std::pair<Point, bool> return_interpolation;

                LOG_PROGRESS(INFO, "POINT") << "Interpolating point X=" << i + 1 << " Y=" << j + 1 << " Z=" << k + 1 << " ("
                                            << q.x << "," << q.y << "," << q.z << ")";

                size_t prev_neighbours = 0;
                float radius = initial_radius;
                size_t index_cut_up;
                while(radius < max_radius) {
                    LOG(TRACE) << "Search radius: " << radius;

                    // Calling octree neighbours search and sorting the results list with the closest neighbours first
                    std::vector<unsigned int> results;
                    octree.radiusNeighbors<unibn::L2Distance<Point>>(q, radius, results);
                    std::sort(results.begin(), results.end(), [&](unsigned int a, unsigned int b) {
                        return unibn::L2Distance<Point>::compute(points[a], q) <
                               unibn::L2Distance<Point>::compute(points[b], q);
                    });

                    LOG(TRACE) << "Number of vertices found: " << results.size();

                    if(results.empty()) {
                        LOG(WARNING) << "At vertex (" << x << ", " << y << ", " << z << ")" << std::endl
                                     << "Radius too Small. No neighbours found for radius " << radius << std::endl
                                     << "Increasing the readius (setting a higher initial radius may help)";
                        radius = radius + radius_step;
                        continue;
                    }

                    if(results.size() < 4) {
                        LOG(WARNING) << "At vertex (" << x << ", " << y << ", " << z << ")" << std::endl
                                     << "Incomplete mesh element found for radius " << radius << std::endl
                                     << "Increasing the readius (setting a higher initial radius may help)";
                        radius = radius + radius_step;
                        continue;
                    }

                    // If after a radius step no new neighbours are found, go to the next radius step
                    if(results.size() > prev_neighbours) {
                        prev_neighbours = results.size();
                    } else {
                        LOG(WARNING) << "At vertex (" << x << ", " << y << ", " << z << ")" << std::endl
                                     << "No new neighbour after radius step. Going to next step.";
                        radius = radius + radius_step;
                        continue;
                    }

                    // Finding tetrahedrons
                    double volume;
                    Eigen::Matrix4d matrix;
                    size_t num_nodes_element = 4;
                    std::vector<Point> tetra_vertices;
                    std::vector<Point> tetra_vertices_field;

                    std::vector<int> bitmask(num_nodes_element, 1);
                    bitmask.resize(results.size(), 0);
                    std::vector<size_t> index;

                    if(!index_cut_flag) {
                        index_cut = results.size();
                    }
                    index_cut_up = index_cut;
                    while(index_cut_up <= results.size()) {
                        do {
                            index.clear();
                            tetra_vertices.clear();
                            tetra_vertices_field.clear();
                            // print integers and permute bitmask
                            for(size_t idk = 0; idk < results.size(); ++idk) {
                                if(bitmask[idk] != 0) {
                                    index.push_back(idk);
                                    tetra_vertices.push_back(points[results[idk]]);
                                    tetra_vertices_field.push_back(field[results[idk]]);
                                }
                                if(index.size() == 4) {
                                    break;
                                }
                            }

                            if(index[0] > index_cut_up || index[1] > index_cut_up || index[2] > index_cut_up ||
                               index[3] > index_cut_up) {
                                continue;
                            }

                            LOG(TRACE) << "Parsing neighbors [index]: " << index[0] << ", " << index[1] << ", " << index[2]
                                       << ", " << index[3];

                            matrix << 1, 1, 1, 1, points[results[index[0]]].x, points[results[index[1]]].x,
                                points[results[index[2]]].x, points[results[index[3]]].x, points[results[index[0]]].y,
                                points[results[index[1]]].y, points[results[index[2]]].y, points[results[index[3]]].y,
                                points[results[index[0]]].z, points[results[index[1]]].z, points[results[index[2]]].z,
                                points[results[index[3]]].z;
                            volume = (matrix.determinant()) / 6;

                            if(std::abs(volume) <= volume_cut) {
                                LOG(DEBUG) << "Coplanar vertices. Going to the next vertex combination.";
                                continue;
                            }
                            try {
                                return_interpolation =
                                    barycentric_interpolation(q, tetra_vertices, tetra_vertices_field, volume);
                                e = return_interpolation.first;
                                flag = return_interpolation.second;
                            } catch(std::invalid_argument& exception) {
                                LOG(DEBUG) << "Failed to interpolate point: " << exception.what();
                                continue;
                            }
                            if(flag == false) {
                                continue;
                            }
                            break;
                        } while(std::prev_permutation(bitmask.begin(), bitmask.end()));

                        if(tetra_vertices.size() == 4 && flag == true) {
                            break;
                        }

                        LOG(DEBUG) << "All combinations tried up to index " << index_cut_up
                                   << " done. Increasing the index cut.";
                        index_cut_up = index_cut_up + index_cut;
                    }

                    if(tetra_vertices.size() == 4 && flag == true) {
                        break;
                    }

                    LOG(DEBUG) << "All combinations tried. Increasing the radius.";
                    radius = radius + radius_step;
                }

                if(flag == false) {
                    LOG(FATAL) << "Couldn't interpolate new mesh point, probably the grid is too irregular";
                    return 1;
                }

                e_field_new_mesh.push_back(e);
                z += zstep;
            }
            y += ystep;
        }
        x += xstep;
    }

    end = std::chrono::system_clock::now();
    elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    LOG(INFO) << "New mesh created in " << elapsed_seconds << " seconds.";

    LOG(STATUS) << "Writing INIT file";

    std::ofstream init_file;
    std::stringstream init_file_name;
    init_file_name << init_file_prefix << ".init";
    init_file.open(init_file_name.str());

    // Write INIT file h"eader
    init_file << "tcad_octree_writer" << std::endl;                                    // NAME
    init_file << "##SEED## ##EVENTS##" << std::endl;                                   // UNUSED
    init_file << "##TURN## ##TILT## 1.0" << std::endl;                                 // UNUSED
    init_file << "0.0 0.0 0.0" << std::endl;                                           // MAGNETIC FIELD (UNUSED)
    init_file << (maxz - minz) << " " << (maxx - minx) << " " << (maxy - miny) << " "; // PIXEL DIMENSIONS
    init_file << "0.0 0.0 0.0 0.0 ";                                                   // UNUSED
    init_file << xdiv << " " << ydiv << " " << zdiv << " ";                            // GRID SIZE
    init_file << "0.0" << std::endl;                                                   // UNUSED

    // Write INIT file data
    for(int i = 0; i < xdiv; ++i) {
        for(int j = 0; j < ydiv; ++j) {
            for(int k = 0; k < zdiv; ++k) {
                auto& point = e_field_new_mesh[static_cast<unsigned int>(i * ydiv * zdiv + j * zdiv + k)];
                init_file << i + 1 << " " << j + 1 << " " << k + 1 << " " << point.x << " " << point.y << " " << point.z
                          << std::endl;
            }
        }
    }
    init_file.close();

    end = std::chrono::system_clock::now();
    elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    LOG(STATUS) << "Conversion completed in " << elapsed_seconds << " seconds.";

    allpix::Log::finish();
    return 1;
}
