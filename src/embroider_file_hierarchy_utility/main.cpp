#include "embroider_file_hierarchy_utility/pch.hpp"
#include <optional>
#include <algorithm>

void log( const String& str ) {
    std::cout << str << std::endl;
}

int main( int argc, char* argv[] ) {
    if ( argc < 2 ) {
        log( "No path provided!" );
        return 1;
    }
    String root_path( argv[1] );

    if ( !std::filesystem::is_directory( root_path ) ) {
        log( "Not a directory!" );
        return 1;
    }

    namespace fs = std::filesystem;

    // Generated files
    struct FileData {
        fs::path orig_path;
        String name;
    };
    std::vector<FileData> fotos_collection;
    std::vector<FileData> layout_collection;

    // Utility functions
    auto is_image = []( const fs::path& file ) {
        return fs::is_regular_file( file ) &&
               ( file.extension() == ".jpg" || file.extension() == ".JPG" ||
                 file.extension() == ".jpeg" || file.extension() == ".JPEG" ||
                 file.extension() == ".png" || file.extension() == ".PNG" );
    };
    auto find_common_prefix = []( const std::vector<fs::path>& files ) -> String {
        if ( files.size() < 2 )
            return ""; // By our definition one file has no common prefix
        String prefix;
        for ( size_t i = 0; i < files.front().stem().string().size(); i++ ) {
            auto c = files.front().stem().string()[i];
            for ( size_t j = 1; j < files.size(); j++ ) {
                if ( files[j].stem().string().size() <= i || files[j].stem().string()[i] != c ) {
                    // Non-prefix character
                    return prefix;
                }
            }
            // All remaining files include c at i
            prefix += c;
        }
        return prefix;
    };
    auto find_common_suffix = []( const std::vector<fs::path>& files ) -> String {
        if ( files.size() < 2 )
            return ""; // By our definition one file has no common suffix
        String suffix;
        for ( size_t i = 0; i < files.front().stem().string().size(); i++ ) {
            auto c = files.front().stem().string()[files.front().stem().string().size() - i - 1];
            for ( size_t j = 1; j < files.size(); j++ ) {
                if ( files[j].stem().string().size() <= i ||
                     files[j].stem().string()[files[j].stem().string().size() - i - 1] != c ) {
                    // Non-suffix character
                    return suffix;
                }
            }
            // All remaining files include c at i
            suffix.insert( suffix.begin(), c );
        }
        return suffix;
    };
    auto trim_on_prefix_suffix = []( const String& stem, const String& prefix,
                                     const String& suffix ) -> String {
        size_t expected_length = stem.size() - prefix.size() - suffix.size();
        if ( expected_length == 0 ) {
            // Trimming too much
            if ( !prefix.empty() ) {
                return prefix;
            } else {
                return suffix;
            }
        }
        return stem.substr( prefix.size(), expected_length );
    };
    auto check_trim_is_new = []( std::vector<String>& existing_trims, const String& new_name,
                                 const String& full_stem ) {
        if ( std::find( existing_trims.begin(), existing_trims.end(), new_name ) !=
             existing_trims.end() ) {
            log( "WARN: Two folders resolve to the same trimmed name." );
            log( "Full path stem '" + full_stem + "' is trimmed to '" + new_name + "'" );
            log( "Press Enter to ignore this and continue. Terminate the program otherwise." );
            std::cin.get();
        }
        existing_trims.push_back( new_name );
    };
    auto has_suffix_or_equals = []( const String& str, const String& suffix ) {
        return str.find( suffix ) == str.size() - suffix.size();
    };

    // Traverse group dirs
    log( "Scanning directory..." );
    std::vector<String> group_trims;
    for ( auto const& group : fs::directory_iterator{ root_path } ) {
        if ( group.is_directory() ) {
            String group_name = group.path().filename().string().substr( 0, 16 );
            check_trim_is_new( group_trims, group_name, group.path().filename().string() );

            // Traverse main list of patterns
            std::vector<String> pattern_trims;
            for ( auto const& pattern_folder : fs::directory_iterator{ group.path() } ) {
                if ( pattern_folder.is_directory() ) {
                    String pattern_name = pattern_folder.path().filename().string().substr( 0, 16 );
                    check_trim_is_new( pattern_trims, pattern_name,
                                       pattern_folder.path().filename().string() );

                    // Check if "Fotos" exists
                    std::optional<fs::path> fotos_path;
                    for ( auto const& f : fs::directory_iterator{ pattern_folder.path() } ) {
                        if ( f.is_directory() &&
                             has_suffix_or_equals( f.path().stem().string(), "Fotos" ) ) {
                            fotos_path = f;
                            break;
                        }
                    }

                    if ( fotos_path ) {
                        std::vector<fs::path> img_files;
                        for ( auto const& img : fs::directory_iterator{ fotos_path.value() } ) {
                            if ( is_image( img ) ) {
                                img_files.push_back( img.path() );
                            }
                        }
                        auto prefix = find_common_prefix( img_files );
                        auto suffix = find_common_suffix( img_files );

                        // Add files to foto collection
                        for ( const auto& img : img_files ) {
                            String stem_trimmed =
                                trim_on_prefix_suffix( img.stem(), prefix, suffix );
                            fotos_collection.push_back(
                                FileData{ img, group_name + "__" + pattern_name + "__" +
                                                   stem_trimmed + img.extension().string() } );
                        }
                    }

                    // Check layout dirs
                    std::vector<fs::path> layout_dirs;
                    for ( auto const& layout_dir :
                          fs::directory_iterator{ pattern_folder.path() } ) {
                        if ( layout_dir.is_directory() &&
                             ( !fotos_path || layout_dir != fotos_path.value() ) ) {
                            // A "not fotos" directory
                            layout_dirs.push_back( layout_dir.path() );
                        }
                    }

                    auto prefix = find_common_prefix( layout_dirs );
                    auto suffix = find_common_suffix( layout_dirs );
                    for ( const auto& lay : layout_dirs ) {
                        String lay_stem_trimmed =
                            trim_on_prefix_suffix( lay.stem(), prefix, suffix );

                        // Iterate inner most image files
                        std::vector<fs::path> images;
                        for ( auto const& img_file : fs::directory_iterator{ lay } ) {
                            if ( is_image( img_file ) ) {
                                images.push_back( img_file.path() );
                            }
                        }
                        auto prefix = find_common_prefix( images );
                        auto suffix = find_common_suffix( images );

                        // Add files to layout collection
                        for ( const auto& img : images ) {
                            String img_stem_trimmed =
                                trim_on_prefix_suffix( img.stem(), prefix, suffix );
                            layout_collection.push_back( FileData{
                                img, group_name + "__" + pattern_name + "__" + lay_stem_trimmed +
                                         "__" + img_stem_trimmed + img.extension().string() } );
                        }
                    }
                }
            }
        }
    }
    log( "Found " + to_string( fotos_collection.size() + layout_collection.size() ) + " images." );

    // Check if generated file names occur multiple times (due to trimming)
    log( "Checking path compatibility..." );
    bool check_successful = true;
    auto check_collection_compatibility = [&]( const std::vector<FileData>& collection ) {
        size_t i = 0;
        for ( const auto& a : collection ) {
            size_t j = 0;
            for ( const auto& b : collection ) {
                if ( a.name == b.name && i != j ) {
                    log( "ERROR: Two files resolve to the same trimmed file. Those are:" );
                    log( "File path 1: " + a.orig_path.string() );
                    log( "File path 2: " + b.orig_path.string() );
                    log( "Resolve both to: " + a.name );
                    log( "Please rename the files or directories to fix this issue, or change the "
                         "trimming limits." );
                    check_successful = false;
                }
                j++;
            }
            i++;
        }
    };
    check_collection_compatibility( fotos_collection );
    check_collection_compatibility( layout_collection );
    if ( !check_successful ) {
        log( "Not copying files due to error." );
        return -1;
    }

    // Copy files into target directories
    log( "Copying files..." );
    size_t copy_ctr = 0;
    auto copy_file_collection = [&]( const std::vector<FileData>& collection,
                                     const String& collection_name ) {
        if ( collection.empty() )
            return;
        String out_dir = root_path + "/" + collection_name;
        if ( !fs::exists( out_dir ) )
            fs::create_directories( out_dir );
        for ( const auto& fd : collection ) {
            if ( fs::copy_file( fd.orig_path, out_dir + "/" + fd.name,
                                fs::copy_options::skip_existing ) ) {
                copy_ctr++;
            }
        }
    };
    copy_file_collection( fotos_collection, "fotos_collection" );
    copy_file_collection( layout_collection, "layout_collection" );

    log( "Done. Copied " + to_string( copy_ctr ) + " new files." );

    return 0;
}
