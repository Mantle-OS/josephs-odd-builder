#include <cerrno>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <job_logger.h>
#include <reader.h>
#include <schema.h>
#include <writer.h>

#include <job_emitter_msgpack.h>

namespace fs = std::filesystem;

using namespace job::serializer;

struct AppArgs
{
    std::vector<fs::path> schemaFiles;
    fs::path outDir;
    std::string lang;
    std::string exportHeader;
    std::string exportMacro;
};

bool parseArgs(int argc, char *argv[], AppArgs &args)
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--schemas") {
            if (++i >= argc) {
                std::cerr << "ERROR: --schemas requires at least one file.\n";
                return false;
            }

            while (i < argc && argv[i][0] != '-')
                args.schemaFiles.push_back(argv[i++]);

            --i;
        } else if (arg == "--out") {
            if (++i >= argc) {
                std::cerr << "ERROR: --out requires a directory.\n";
                return false;
            }

            args.outDir = argv[i];
        } else if (arg == "--lang" || arg == "-l") {
            if (++i >= argc) {
                std::cerr << "ERROR: --lang requires a type [cpp,c,java,py,etc] "
                             "(only cpp is supported at the moment).\n";
                return false;
            }

            args.lang = argv[i];
        } else if (arg == "--export-header") {
            if (++i >= argc) {
                std::cerr << "ERROR: --export-header requires a header name.\n";
                return false;
            }

            args.exportHeader = argv[i];
        } else if (arg == "--export-macro") {
            if (++i >= argc) {
                std::cerr << "ERROR: --export-macro requires a macro name.\n";
                return false;
            }

            args.exportMacro = argv[i];
        } else {
            std::cerr << "ERROR: Unknown argument: " << arg << '\n';
            return false;
        }
    }

    if (args.schemaFiles.empty()) {
        std::cerr << "ERROR: --schemas is required.\n";
        return false;
    }

    if (args.outDir.empty()) {
        std::cerr << "ERROR: --out is required.\n";
        return false;
    }

    if (args.lang.empty())
        args.lang = "cpp";

    if (args.exportHeader.empty()) {
        std::cerr << "ERROR: --export-header is required.\n";
        return false;
    }

    if (args.exportMacro.empty()) {
        std::cerr << "ERROR: --export-macro is required.\n";
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    AppArgs args;

    if (!parseArgs(argc, argv, args)) {
        std::cerr << "Usage: " << argv[0]
                  << " --schemas <file1.yml> [file2.yml...]"
                  << " --out <dir>"
                  << " --export-header <header>"
                  << " --export-macro <macro>\n";
        return 1;
    }

    job::serializer::msg_pack::JobEmitterMsgPack emitterPlugin;

    if (args.exportHeader.empty()) {
        JOB_LOG_ERROR("[job_msg_gen] Export header is empty");
        return EINVAL;
    }

    if (args.exportMacro.empty()) {
        JOB_LOG_ERROR("[job_msg_gen] Export macro is empty");
        return EINVAL;
    }

    emitterPlugin.setExportHeader(args.exportHeader);
    emitterPlugin.setExportMacro(args.exportMacro);

    int errors = 0;

    for (const auto &schemaPath : args.schemaFiles) {
        JOB_LOG_INFO("[job_msg_gen] Processing schema: {}", schemaPath.string());

        Reader reader(schemaPath);
        Schema schema;

        if (!reader.readSchema(schema)) {
            JOB_LOG_ERROR("[job_msg_gen] FAILED to parse schema: {}", schemaPath.string());
            ++errors;
            continue;
        }

        if (!schema.isValid()) {
            JOB_LOG_ERROR("[job_msg_gen] Schema is invalid");
            schema.dump();
            return EPROTO;
        }

        fs::path headerFile = args.outDir / schema.hdr_name;
        fs::path sourceFile = args.outDir / schema.src_name;

        Writer writer(headerFile);

        if (!writer.writeEmitter(emitterPlugin, schema, headerFile, sourceFile)) {
            JOB_LOG_ERROR("[job_msg_gen] FAILED to write generated code for: {}", schemaPath.string());
            ++errors;
            continue;
        }

        JOB_LOG_INFO("[job_msg_gen] Wrote: {}", headerFile.string());
        JOB_LOG_INFO("[job_msg_gen] Wrote: {}", sourceFile.string());
    }

    if (errors > 0) {
        JOB_LOG_ERROR("[job_msg_gen] Finished with {} errors.", errors);
        return 1;
    }

    JOB_LOG_INFO("[job_msg_gen] All schemas processed successfully.");

    return 0;
}