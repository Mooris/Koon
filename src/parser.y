%skeleton "lalr1.cc"
%require "3.0.4"
%defines
%define parser_class_name {KoonParser}

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires
{
    # include <string>
    # include "Nodes.hh"
    class KoonDriver;
}

%param { KoonDriver& driver }

%locations
%initial-action
{
    // Initialize the initial location.
    @$.begin.filename = @$.end.filename = driver.getFile();
};

// /!\ Maybe incorrect

%define parse.trace
%define parse.error verbose

%code
{
    # include "KoonDriver.hh"
}

%define api.token.prefix {TOK_}
%token
    END  0  "end of file"
    ASSIGN  "="
    MINUS   "-"
    PLUS    "+"
    STAR    "*"
    SLASH   "/"
    LPAREN  "("
    RPAREN  ")"
    LBRACE  "{"
    RBRACE  "}"
    DECL    "decl"
    RET     "return"
    CLASS   "class"
    VAR     "var"
    RARROW  "->"
    DOT     "."
    PIPE    "|"
;

%token  <std::string>   IDENTIFIER  "identifier"
%token  <std::string>   COLID       "colid"
%token  <int>           NUMBER      "number"
%type   <KBlock>        stmts block
%type   <std::shared_ptr<KStatement>>    stmt func_or_obj
%type   <std::shared_ptr<KObjField>>    obj_component
%type   <KObjectDecl>   obj_decl
%type   <KFuncDecl>     func_decl
%type   <KTopLevel>     toplevel
%type   <KObjFieldList> obj_block
%type   <KIdentifier>   return_type
%type   <KArg>          arg
%type   <KCallArg>      carg
%type   <KArgList>      arg_list inner_arg_list
%type   <KCallArgList>  call_args inner_call_args
%type   <std::shared_ptr<KExpression>>   expr
%type   <KReturnStatement>  return_statement
%type   <KVarDecl>      var_decl
%type   <std::shared_ptr<KOperator>>     operator

/*
%printer {} <KBlock>;
%printer {} <KStatement>;
%printer { yyoutput << $$;  } <*>;
*/

%left "+" "-";
%left "*" "/";
%%

%start program;

program:
    toplevel    { driver.rootBlock = std::move($1); }
;

toplevel:
    func_or_obj         { $$ = KTopLevel(); $$.emplace_back(std::move($1)); }
|   toplevel func_or_obj { $1.emplace_back(std::move($2)); $$ = std::move($1); }
;

func_or_obj:
    func_decl   { $$ = std::make_shared<KFuncDecl>(std::move($1)); }
|   obj_decl    { $$ = std::make_shared<KObjectDecl>(std::move($1)); }
;

obj_decl:
    "class" "identifier" "{" obj_block "}" { $$ = KObjectDecl(std::move($2), std::move($4), driver.kontext()); }
;

obj_block:
    obj_component           { $$ = KObjFieldList(); $$.emplace_back(std::move($1)); }
|   obj_block obj_component { $$ = $1; $$.emplace_back(std::move($2)); }
;

obj_component:
    /* blank */ { $$ = nullptr; }
|   func_decl   { $$ = std::make_shared<KFuncDecl>(std::move($1)); }
|   obj_decl    { $$ = std::make_shared<KObjectDecl>(std::move($1)); }
|   var_decl    { $$ = std::make_shared<KVarDecl>(std::move($1)); $$->setInObj(); }
;

stmts:
    stmt        {
        $$ = KBlock();
        $$.emplaceStatement(std::move($1));
    }
|   stmts stmt  { $1.emplaceStatement(std::move($2)); $$ = std::move($1); }
;

stmt:
    var_decl { $$ = std::make_shared<KVarDecl>(std::move($1)); }
|   func_decl { $$ = std::make_shared<KFuncDecl>(std::move($1)); }
|   expr { $$ = std::make_shared<KExpressionStatement>(std::move($1)); }
|   return_statement { $$ = std::make_shared<KReturnStatement>(std::move($1)); }
;

var_decl:
    "var" "identifier" "=" expr { $$ = KVarDecl(std::move($2), std::move($4)); }
|   "var" "colid" "identifier" { $2.pop_back();$$ = KVarDecl(std::move($2), std::move($3)); }
|   "var" "colid" "identifier" "=" expr { $2.pop_back();$$ = KVarDecl(std::move($2), std::move($3), std::move($5)); }
;

return_statement:
    "return"        { $$ = KReturnStatement(); }
|   "return" expr   { $$ = KReturnStatement(std::move($2)); }
;

expr:
    "|" expr call_args "|" { $$ = std::make_shared<KFuncCall>(std::move($2), std::move($3)); }
|   operator        { $$ = std::move($1); }
|   "identifier"    { $$ = std::make_shared<KIdentifier>(std::move($1)); }
|   "number"        { $$ = std::make_shared<KInt>($1); }
;

operator:
    expr "*" expr   { $$ = std::make_shared<KBinaryOperator>(std::move($1), std::move($3), KOperator::Type::Mul); }
|   expr "/" expr   { $$ = std::make_shared<KBinaryOperator>(std::move($1), std::move($3), KOperator::Type::Div); }
|   expr "+" expr   { $$ = std::make_shared<KBinaryOperator>(std::move($1), std::move($3), KOperator::Type::Add); }
|   expr "-" expr   { $$ = std::make_shared<KBinaryOperator>(std::move($1), std::move($3), KOperator::Type::Sub); }
|   expr "=" expr   { $$ = std::make_shared<KBinaryOperator>(std::move($1), std::move($3), KOperator::Type::Equal); }
;

call_args:
    "identifier"    { $$ = KCallArgList(); $$.emplace_back(std::move($1), nullptr); }
|   inner_call_args { $$ = std::move($1); }
;

inner_call_args:
    carg            { $$ = KCallArgList(); $$.push_back(std::move($1)); }
|   inner_call_args carg  { $1.push_back(std::move($2)); }
;

carg:
    "colid" expr { $1.pop_back(); $$ = KCallArg(std::move($1), std::move($2)); }
;

block:
    "{" stmts "}" { $$ = std::move($2); }
|   "{" "}" { $$ = KBlock(); }

func_decl:
    "decl" arg_list return_type block
        { $$ = KFuncDecl(std::move($3), std::move($2), std::move($4), driver.kontext()); }
;

arg_list:
    "identifier"        { $$ = KArgList(); $$.emplace_back(std::move($1), KIdentifier("none")); }
|   inner_arg_list      { $$ = std::move($1); }
;

inner_arg_list:
    arg {
        $$ = KArgList();
        $$.emplace_back(std::move($1));
    }
|   inner_arg_list arg        {$1.emplace_back(std::move($2)); $$ = std::move($1);}
;

arg:
    "colid" "identifier"    { $1.pop_back();$$ = KArg(std::move($1), std::move($2)); }
|   "identifier" "colid" "identifier" { $2.pop_back();$$ = KArg(std::move($2), std::move($3), std::move($1)); }
;

return_type:
    /* blank */         { $$ = KIdentifier("void"); }
|   "->" "identifier"   { $$ = KIdentifier(std::move($2)); }
;





%%

void yy::KoonParser::error  (const location_type& l,
                            const std::string& m) {
    driver.error(l, m);
}
