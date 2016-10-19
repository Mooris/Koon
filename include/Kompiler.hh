#pragma once

#include <llvm/ADT/Triple.h>
#include <string>
#include <memory>

namespace llvm {
    class Module;
    class StringRef;
    class tool_output_file;
}

class Kompiler {
public:
    Kompiler(std::string name)
        : OutputFilename(std::move(name)) {}
    int kompile(llvm::Module &);

private:
    int compile(llvm::Module &);
    std::unique_ptr<llvm::tool_output_file> getStream(llvm::Triple::OSType);
    llvm::AnalysisID getPassID(const char   *argv0,
                            const char      *OptionName,
                            llvm::StringRef       PassName);

private:
    std::string OutputFilename;
};
