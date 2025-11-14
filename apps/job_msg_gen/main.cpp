#include <iostream>
#include <filesystem>
#include <string>
#include <vector>

#include <job_logger.h>
#include <schema.h>
#include <reader.h>
#include <writer.h>
#include <emitters/emitter.h>
#include <job_emitter_msgpack.h>

namespace fs = std::filesystem;
using namespace job::serializer;

struct AppArgs {
    std::vector<fs::path> schema_files;
    fs::path out_dir;
    std::string lang;
};

bool parse_args( int argc, char *argv[], AppArgs &args )
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--schemas") {
            if (++i >= argc) {
                std::cerr << "ERROR: --schemas requires at least one file." << std::endl;
                return false;
            }
            while (i < argc && argv[i][0] != '-')
                args.schema_files.push_back(argv[i++]);

            --i;
        } else if (arg == "--out") {
            if (++i >= argc) {
                std::cerr << "ERROR: --out requires a directory." << std::endl;
                return false;
            }
            args.out_dir = argv[i];
        } else if (arg == "--lang" || arg == "-l") {
            if (++i >= argc) {
                std::cerr << "ERROR: --lang requires a type [cpp,c,java,py, ect] (only cpp is supported at the moment)." << std::endl;
                return false;
            }
            args.lang = argv[i];
        }
        else {
            std::cerr << "ERROR: Unknown argument: " << arg << std::endl;
            return false;
        }
    }

    if (args.schema_files.empty()) {
        std::cerr << "ERROR: --schemas is required." << std::endl;
        return false;
    }

    if (args.out_dir.empty()) {
        std::cerr << "ERROR: --out is required." << std::endl;
        return false;
    }

    if (args.lang.empty()) {
        args.lang = "cpp";
    }

    return true;
}

int main( int argc, char *argv[])
{
    AppArgs args;
    if (!parse_args(argc, argv, args)) {
        std::cerr << "Usage: " << argv[0] << " --schemas <file1.yml> [file2.yml...] --out <dir>" << std::endl;
        return 1;
    }

    job::serializer::msg_pack::JobEmitterMsgPack emitter_plugin;
    int errors = 0;
    for (const auto &schema_path : args.schema_files) {
        JOB_LOG_INFO("[job_msg_gen] Processing schema: {}", schema_path.string());
        Reader reader(schema_path);
        Schema schema;
        if (!reader.readSchema(schema)) {
            JOB_LOG_ERROR("[job_msg_gen] FAILED to parse schema: {}", schema_path.string());
            errors++;
            continue;
        }
        //NEW
        if(!schema.isValid()){
            JOB_LOG_ERROR("[job_msg_gen] Schema: is infalid \n");
            schema.dump();
            return EPROTO;
        }


        fs::path header_file = args.out_dir / schema.hdr_name;
        fs::path source_file = args.out_dir / schema.src_name;
        Writer writer(header_file);

        if (!writer.writeEmitter(emitter_plugin, schema, header_file, source_file)) {
            JOB_LOG_ERROR("[job_msg_gen] FAILED to write generated code for: {}", schema_path.string());
            errors++;
            continue;
        }

        JOB_LOG_INFO("[job_msg_gen] Wrote: {}", header_file.string());
        JOB_LOG_INFO("[job_msg_gen] Wrote: {}", source_file.string());
    }

    if (errors > 0) {
        JOB_LOG_ERROR("[job_msg_gen] Finished with {} errors.", errors);
        return 1;
    }

    JOB_LOG_INFO("[job_msg_gen] All schemas processed successfully.");
    return 0;
}
// CHECKPOINT: v1
