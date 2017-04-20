#include <llvm/ADT/StringRef.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Pass.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include "Kompiler.hh"

using namespace llvm;

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

int Kompiler::kompile(Module &theModule) {
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllAsmParsers();

    PassRegistry *Registry = PassRegistry::getPassRegistry();
    initializeCore(*Registry);
    initializeCodeGen(*Registry);
    initializeLoopStrengthReducePass(*Registry);
    initializeLowerIntrinsicsPass(*Registry);

    for (unsigned I = TimeCompilations; I; --I)
        if (int RetVal = this->compile(theModule))
            return RetVal;
    return 0;
}

//TODO: Je je je sais pas la ...
AnalysisID Kompiler::getPassID(const char *argv0, const char *OptionName, StringRef PassName) {
    if (PassName.empty())
        return nullptr;

    const PassRegistry &PR = *PassRegistry::getPassRegistry();
    const PassInfo *PI = PR.getPassInfo(PassName);
    if (!PI) {
        errs() << argv0 << ": " << OptionName << " pass is not registered.\n";
        exit(1);
    }
    return PI->getTypeInfo();
}

int Kompiler::compile(Module &theModule) {
    SMDiagnostic    err;
    Triple          babyTriple;

    if (verifyModule(theModule, &errs())) {
        errs() << "\nModule broke... Sorry :'(";
        return 1;
    }
    if (!TargetTriple.empty()) {
        theModule.setTargetTriple(Triple::normalize(TargetTriple));
        babyTriple = Triple(theModule.getTargetTriple());
    } else {
        babyTriple = Triple(Triple::normalize(TargetTriple));
    }

    if (babyTriple.getTriple().empty())
        babyTriple.setTriple(sys::getDefaultTargetTriple());

    std::string error;
    const Target *TheTarget = TargetRegistry::lookupTarget(MArch, babyTriple, error);
    if (!TheTarget) {
        errs() << "koon" << ": " << error;
        return 1;
    }

    std::string CPUStr = getCPUStr(), FeaturesStr = getFeaturesStr();

    CodeGenOpt::Level OLvl = CodeGenOpt::Default;
    switch (OptLevel) {
    default:
        errs() << "koon" << ": invalid optimization level.\n";
        return 1;
    case ' ': break;
    case '0': OLvl = CodeGenOpt::None; break;
    case '1': OLvl = CodeGenOpt::Less; break;
    case '2': OLvl = CodeGenOpt::Default; break;
    case '3': OLvl = CodeGenOpt::Aggressive; break;
    }

    TargetOptions laOptions = InitTargetOptionsFromCodeGenFlags();
    laOptions.DisableIntegratedAS = NoIntegratedAssembler;
    laOptions.MCOptions.ShowMCEncoding = ShowMCEncoding;
    laOptions.MCOptions.MCUseDwarfDirectory = EnableDwarfDirectory;
    laOptions.MCOptions.AsmVerbose = AsmVerbose;

    std::unique_ptr<TargetMachine> leTarget(
        TheTarget->createTargetMachine( babyTriple.getTriple(),
                                        CPUStr,
                                        FeaturesStr,
                                        laOptions,
                                        llvm::Optional<llvm::Reloc::Model>(RelocModel),
                                        CMModel,
                                        OLvl));

    if (FloatABIForCalls != FloatABI::Default)
        laOptions.FloatABIType = FloatABIForCalls;

    std::unique_ptr<tool_output_file> Out =
        this->getStream(babyTriple.getOS());
    if (!Out) return 1;

    legacy::PassManager passManager;

    TargetLibraryInfoImpl TLII(Triple(theModule.getTargetTriple()));

    if (DisableSimplifyLibCalls)
        TLII.disableAllFunctions();
    passManager.add(new TargetLibraryInfoWrapperPass(TLII));
    theModule.setDataLayout(leTarget->createDataLayout());

    setFunctionAttributes(CPUStr, FeaturesStr, theModule);

    std::unique_ptr<raw_svector_ostream> BOS;
    SmallVector<char, 0> Buffer;
    {
        raw_pwrite_stream *OS = &Out->os();

        if (!Out->os().supportsSeeking() || CompileTwice) {
            BOS = make_unique<raw_svector_ostream>(Buffer);
            OS = BOS.get();
        }

        const char *argv0 = "koon";
        AnalysisID StartBeforeID = getPassID(argv0, "start-before", StartBefore);
        AnalysisID StartAfterID = getPassID(argv0, "start-after", StartAfter);
        AnalysisID StopAfterID = getPassID(argv0, "stop-after", StopAfter);
        AnalysisID StopBeforeID = getPassID(argv0, "stop-before", StopBefore);

        if (StartBeforeID && StartAfterID) {
            errs() << "koon"<< ": -start-before and -start-after specified!\n";
            return 1;
        }
        if (StopBeforeID && StopAfterID) {
            errs() << "koon" << ": -stop-before and -stop-after specified!\n";
            return 1;
        }
        if (leTarget->addPassesToEmitFile(passManager,
                                        *OS, TargetMachine::CGFT_ObjectFile,
                                        NoVerify, StartBeforeID, StartAfterID,
                                        StopAfterID)) {
            errs()  << "koon" << ": target does not support generation of this"
                    << " file type!\n";
            return 1;
        }
    }

    SmallVector<char, 0> CompileTwiceBuffer;
    if (CompileTwice) {
        std::unique_ptr<Module> doppelganger(llvm::CloneModule(&theModule));
        passManager.run(*doppelganger);
        CompileTwiceBuffer = Buffer;
        Buffer.clear();
    }

    passManager.run(theModule);
    //auto HasError = *static_cast<bool *>(getGlobalContext().getDiagnosticContext());
    //if (HasError)
        //return 1;

    if (CompileTwice) {
        if (Buffer.size() != CompileTwiceBuffer.size() ||
                (memcmp(Buffer.data(), CompileTwiceBuffer.data(), Buffer.size()) != 0)) {
            errs() <<   "Running the pass manager twice changed the output.\n"
                        "Writing the result of the second run to the specified output\n"
                        "To generate the one-run comparison binary, just run without\n"
                        "the compile-twice option\n";
            Out->os() << Buffer;
            Out->keep();
            return 1;
        }
    }
    if (BOS) {
        Out->os() << Buffer;
    }
    Out->keep();

    return 0;
}

std::unique_ptr<tool_output_file> Kompiler::getStream(Triple::OSType os) {
    std::string filename = "/tmp/" + OutputFilename;

    if (os == Triple::Win32)
        filename += ".obj";
    else
        filename += ".o";

    std::error_code EC;
    sys::fs::OpenFlags OpenFlags = sys::fs::F_None;
    auto FDOut = llvm::make_unique<tool_output_file>(filename,
                                                    EC,
                                                    OpenFlags);
    if (EC) {
        errs() << EC.message() << '\n';
        return nullptr;
    }

    return FDOut;
}
