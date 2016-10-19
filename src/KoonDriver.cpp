#include <fstream>
#include <array>
#include <llvm/IR/Value.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/CommandLine.h>
#include "KoonDriver.hh"
#include "Kontext.hh"
#include "Process.hh"

#include "Kompiler.hh"

static llvm::cl::opt<std::string>
OutputFilename( "o",
                llvm::cl::init("k.out"),
                llvm::cl::desc("Output filename"),
                llvm::cl::value_desc("filename"));

static llvm::cl::opt<bool>
ShowAssembly("show-assembly",
            llvm::cl::desc("Show the llvm-assembly code bebore compiling"),
            llvm::cl::init(false));

KoonDriver::KoonDriver()
    : result(0), trace_scanning(false), trace_parsing(false)
{
}

KoonDriver::~KoonDriver() {
}

int KoonDriver::parse(const std::string &f)
{
    using namespace std;

    this->file = f;
    scan_begin ();
    yy::KoonParser parser(*this);
    parser.set_debug_level(trace_parsing);
    int res = parser.parse();
    scan_end();

    if (!res) {
        for (auto &func: this->rootBlock) {
            func.codeGen(this->_k);
        }
        if (ShowAssembly) {
            llvm::PassManager<llvm::Module> pm;
            pm.addPass(llvm::PrintModulePass(llvm::outs()));
            pm.run(*this->module());
        }
        return this->output();
    }
    return res;
}

int KoonDriver::output() const {
    Kompiler kompiler(OutputFilename);
    int res = kompiler.kompile(*this->_k.module());

    if (res) return res;
    std::string object = "/tmp/" + OutputFilename + ".o";

    Process("/usr/bin/ld",
            "--build-id",
            "--eh-frame-hdr",
            "--hash-style=gnu",
            "-m",
            "elf_x86_64",
            "-dynamic-linker",
            "/lib64/ld-linux-x86-64.so.2",
            "/usr/lib/gcc/x86_64-pc-linux-gnu/6.2.1/../../../../lib/crt1.o",
            "/usr/lib/gcc/x86_64-pc-linux-gnu/6.2.1/../../../../lib/crti.o",
            "/usr/lib/gcc/x86_64-pc-linux-gnu/6.2.1/crtbegin.o",
            "-L/usr/lib/gcc/x86_64-pc-linux-gnu/6.2.1",
            "-L/usr/lib/gcc/x86_64-pc-linux-gnu/6.2.1/../../../../lib",
            "-L/lib/../lib",
            "-L/usr/lib",
            object.c_str(),
            "-lgcc",
            "--as-needed",
            "-lgcc_s",
            "--no-as-needed",
            "-lc",
            "-lgcc",
            "--as-needed",
            "-lgcc_s",
            "--no-as-needed",
            "/usr/lib/gcc/x86_64-pc-linux-gnu/6.2.1/crtend.o",
            "/usr/lib/gcc/x86_64-pc-linux-gnu/6.2.1/../../../../lib/crtn.o",
            "-o",
            OutputFilename.c_str());

    Process("/usr/bin/rm", "-f", object.c_str());
    return res;
}

void KoonDriver::error(const yy::location& l, const std::string& m)
{
    std::cerr << this->file << ":" << l << ": " << m << std::endl;
}

void KoonDriver::error(const std::string& m)
{
    std::cerr << m << std::endl;
}
