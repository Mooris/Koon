#pragma once

#include <string>

#include "Kontext.hh"
#include "Nodes.hh"
#include "parser.hpp"

# define YY_DECL \
    yy::KoonParser::symbol_type yylex(KoonDriver& driver)
YY_DECL;

namespace llvm {
    class Module;
}

class KoonDriver {
public:
    KoonDriver();
    virtual ~KoonDriver();

    inline int  getResult() { return result; }
    inline std::string *getFile() { return &this->file; }
    inline void traceParsing() { this->trace_parsing = true; }
    inline void traceScanning() { this->trace_scanning = true; }
    inline llvm::Module *module() const { return this->_k.module(); }

    void        scan_begin();
    void        scan_end();

    int         parse(const std::string& f);

    void        error(const yy::location& l, const std::string& m);
    void        error(const std::string& m);
    int         output() const;

    //TMP
    KFuncList   rootBlock;

private:
    Kontext     _k;
    int         result;
    bool        trace_scanning;
    bool        trace_parsing;
    std::string file;
};
