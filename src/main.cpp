#include <iostream>
#include <llvm/Support/CommandLine.h>
#include "KoonDriver.hh"

static llvm::cl::opt<std::string>
InputFilename(  llvm::cl::Positional,
                llvm::cl::desc("<input file>"),
                llvm::cl::init("-"));

int main(int argc, char *argv[]) {
    llvm::cl::ParseCommandLineOptions(argc, argv, "koon kompiler\n");

    int res = 0;
    KoonDriver driver;
    //for (int i = 1; i < argc; ++i)
    //    if (argv[i] == std::string ("-p"))
    //        driver.traceParsing();
    //    else if (argv[i] == std::string ("-s"))
    //        driver.traceScanning();
    //    else if (argv[i] == std::string ("-o")) {
    //        i += 1;
    //        driver.setOutname(argv[i]);
    if (!driver.parse(InputFilename))
        res = driver.getResult();
    else
        res = 1;
    return res;
}
