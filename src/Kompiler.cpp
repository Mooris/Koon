#include <llvm/Pass.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/FileSystem.h>
/*
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Transforms/Utils/Cloning.h>
*/
#include "Kompiler.hh"

using namespace llvm;

/*
static cl::opt<unsigned>
TimeCompilations("time-compilations",
                cl::Hidden,
                cl::init(1u),
                cl::value_desc("N"),
                cl::desc("Repeat compilation N times for timing"));

static cl::opt<bool>
NoIntegratedAssembler("no-integrated-as", cl::Hidden,
                    cl::desc("Disable integrated assembler"));

static cl::opt<bool>
ShowMCEncoding( "show-mc-encoding",
                cl::Hidden,
                cl::desc("Show encoding in .s output"));

static cl::opt<bool>
EnableDwarfDirectory("enable-dwarf-directory",
                    cl::Hidden,
                    cl::desc("Use .file directives with an explicit directory."));

static cl::opt<bool>
NoVerify("disable-verify", cl::Hidden,
        cl::desc("Do not verify input module"));

static cl::opt<bool>
AsmVerbose( "asm-verbose",
            cl::desc("Add comments to directives."),
            cl::init(true));

static cl::opt<std::string>
TargetTriple("mtriple",
            cl::desc("Override target triple for module"));

static cl::opt<char>
OptLevel("O",
        cl::desc("Optimization level. [-O0, -O1, -O2, or -O3] "
                "(default = '-O2')"),
        cl::Prefix,
        cl::ZeroOrMore,
        cl::init(' '));

static cl::opt<bool>
DisableSimplifyLibCalls("disable-simplify-libcalls",
                        cl::desc("Disable simplify-libcalls"));

static cl::opt<bool>
CompileTwice("compile-twice", cl::Hidden,
            cl::desc("Run everything twice, re-using the same pass "
                     "manager and verify the result is the same."),
            cl::init(false));

static cl::opt<std::string>
StopBefore( "stop-before",
            cl::desc("Stop compilation before a specific pass"),
            cl::value_desc("pass-name"), cl::init(""));

static cl::opt<std::string>
StartBefore("start-before",
            cl::desc("Resume compilation before a specific pass"),
            cl::value_desc("pass-name"), cl::init(""));
*/

int Kompiler::kompile(Module &theModule) {
    auto itsaTriple = sys::getDefaultTargetTriple();
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    std::string err;
    auto leTarget = TargetRegistry::lookupTarget(itsaTriple, err);
    if (!leTarget) {
        errs() << err;
        return 1;
    }

    auto CPU = "generic";
    auto features = "";

    TargetOptions opt;
    auto relocModel = Optional<Reloc::Model>(Reloc::Model());
    auto leTargetMachine = leTarget->createTargetMachine(itsaTriple, CPU, features, opt, relocModel.getValue());

    theModule.setDataLayout(leTargetMachine->createDataLayout());
    theModule.setTargetTriple(itsaTriple);

    auto output = "/tmp/" + OutputFilename + ".o";
    std::error_code EC;
    raw_fd_ostream dest(output, EC, sys::fs::F_None);

    if (EC) {
        errs() << "Could not open file: " << EC.message();
        return 1;
    }

    legacy::PassManager pass;
    auto FileType = TargetMachine::CGFT_ObjectFile;

    if (leTargetMachine->addPassesToEmitFile(pass, dest, FileType)) {
        errs() << "TargetMachine can't emit a file of this type";
        return 1;
    }

    pass.run(theModule);
    dest.flush();
    return 0;
}

//TODO: Je je je sais pas la ...
AnalysisID Kompiler::getPassID(const char *argv0, const char *OptionName, StringRef PassName) {
}

int Kompiler::compile(Module &theModule) {
}

std::unique_ptr<tool_output_file> Kompiler::getStream(Triple::OSType os) {
}
