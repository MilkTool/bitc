%{
  /*
   * Copyright (C) 2008, The EROS Group, LLC.
   * All rights reserved.
   *
   * Redistribution and use in source and binary forms, with or
   * without modification, are permitted provided that the following
   * conditions are met:
   *
   *   - Redistributions of source code must contain the above 
   *     copyright notice, this list of conditions, and the following
   *     disclaimer. 
   *
   *   - Redistributions in binary form must reproduce the above
   *     copyright notice, this list of conditions, and the following
   *     disclaimer in the documentation and/or other materials 
   *     provided with the distribution.
   *
   *   - Neither the names of the copyright holders nor the names of any
   *     of any contributors may be used to endorse or promote products
   *     derived from this software without specific prior written
   *     permission. 
   *
   * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   */

#include <sys/fcntl.h>
#include <sys/stat.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <dirent.h>
#include <iostream>

#include <string>

#include "Version.hxx"
#include "AST.hxx"
#include "ParseType.hxx"
#include "UocInfo.hxx"
#include "Options.hxx"
  
using namespace sherpa;
using namespace std;  

#define YYSTYPE ParseType
#define YYLEX_PARAM (SexprLexer *)lexer
#undef yyerror
#define yyerror(lexer, s) lexer->ReportParseError(s)

#include "SexprLexer.hxx"

#define SHOWPARSE(s) \
  do { \
    if (Options::showParse) \
      lexer->errStream << (s) << std::endl;		\
  } while(false);
#define SHOWPARSE1(s,x) \
  do { \
    if (Options::showParse) \
      lexer->errStream << (s) << " " << (x) << std::endl;	\
  } while(false);


int num_errors = 0;  /* hold the number of syntax errors encountered. */

inline int bitclex(YYSTYPE *lvalp, SexprLexer *lexer)
{
  return lexer->lex(lvalp);
}

// If the passed exprSeq has a documentation string at the front,
// remove it. Note that if the exprSeq has length 1 then the string is
// the expression value and not a documentation comment.
GCPtr<AST> stripDocString(GCPtr<AST> exprSeq)
{
  if (exprSeq->children->size() > 1 &&
      exprSeq->child(0)->astType == at_stringLiteral)
    exprSeq->disown(0);

  return exprSeq;
}

%}

%pure-parser
%parse-param {SexprLexer *lexer}

%token <tok> tk_Reserved	/* reserved words */

/* Categorical terminals: */
%token <tok> tk_Ident
%token <tok> tk_TypeVar
%token <tok> tk_Int
%token <tok> tk_Float
%token <tok> tk_Char
%token <tok> tk_String


/* Primary types and associated hand-recognized literals: */
%token <tok> '(' ')' ','	/* unit */
%token <tok> '[' ']'	/* unit */
%token <tok> tk_BOOL
%token <tok> tk_TRUE   /* #t */
%token <tok> tk_FALSE  /* #f */
%token <tok> tk_CHAR
%token <tok> tk_STRING
%token <tok> tk_FLOAT
%token <tok> tk_FROM
%token <tok> tk_DOUBLE
%token <tok> tk_DUP
%token <tok> tk_QUAD
%token <tok> tk_INT8
%token <tok> tk_INT16
%token <tok> tk_INT32
%token <tok> tk_INT64
%token <tok> tk_UINT8
%token <tok> tk_UINT16
%token <tok> tk_UINT32
%token <tok> tk_UINT64
%token <tok> tk_WORD

%token <tok> tk_BITFIELD
%token <tok> tk_FILL
%token <tok> tk_RESERVED // not to be confused with tk_Reserved
%token <tok> tk_WHERE
  
%token <tok> tk_BITC_VERSION

%token <tok> tk_THE
%token <tok> tk_IF
%token <tok> tk_WHEN
%token <tok> tk_AND
%token <tok> tk_OR
%token <tok> tk_NOT
%token <tok> tk_COND
%token <tok> tk_SWITCH
%token <tok> tk_CASE
%token <tok> tk_OTHERWISE

%token <tok> tk_PAIR
%token <tok> tk_VECTOR
%token <tok> tk_ARRAY
 //%token <tok> tk_MAKE_VECTOR
%token <tok> tk_MAKE_VECTORL
%token <tok> tk_ARRAY_LENGTH
%token <tok> tk_VECTOR_LENGTH
%token <tok> tk_ARRAY_NTH
%token <tok> tk_VECTOR_NTH

%token <tok> tk_DEFSTRUCT
%token <tok> tk_DEFUNION
%token <tok> tk_DEFREPR
%token <tok> tk_DEFTHM
%token <tok> tk_DEFINE
%token <tok> tk_DECLARE
%token <tok> tk_PROCLAIM
%token <tok> tk_EXTERNAL
%token <tok> tk_TAG
%token <tok> tk_USE
%token <tok> tk_DEFEXCEPTION

%token <tok> tk_MUTABLE
%token <tok> tk_SET
%token <tok> tk_DEREF
%token <tok> tk_REF
%token <tok> tk_INNER_REF
%token <tok> tk_VAL
%token <tok> tk_OPAQUE
%token <tok> tk_MEMBER
%token <tok> tk_LAMBDA
%token <tok> tk_LET
%token <tok> tk_LETREC
%token <tok> tk_FN
%token <tok> tk_BEGIN
%token <tok> tk_DO
%token <tok> tk_APPLY
%token <tok> tk_BY_REF

%token <tok> tk_EXCEPTION
%token <tok> tk_TRY
%token <tok> tk_CATCH
%token <tok> tk_THROW

%token <tok> tk_METHOD
%token <tok> tk_DEFTYPECLASS
%token <tok> tk_DEFINSTANCE
%token <tok> tk_FORALL

%token <tok> tk_INTERFACE
%token <tok> tk_MODULE
// token tk_USESEL historic significance only 
// %token <tok> tk_USESEL
%token <tok> tk_PACKAGE
%token <tok> tk_IMPORT
%token <tok> tk_PROVIDE

%token <tok> tk_TYFN
//%token <tok> tk_SUPER
%token <tok> tk_SUSPEND
%token <tok> tk_HIDE
%type <tok>  ifident

//%token <tok> tk_MODULE
//%token <tok> tk_EXPORT
//%token <tok> tk_NAT
//%token <tok> tk_IMMUTABLE
//%token <tok> tk_RESTRICTEDREF
//%token <tok> tk_MUTUAL_RECURSION
//%token <tok> tk_SEQUENCE
//%token <tok> tk_SEQUENCEREF
//%token <tok> tk_SEQUENCELENGTH
   
%type <ast> module implicit_module module_seq
%type <ast> interface
%type <ast> defpattern
%type <ast> mod_definitions mod_definition
%type <ast> if_definitions if_definition
%type <ast> common_definition
%type <ast> proclaim_definition
%type <ast> ptype_name val
%type <ast> type_definition typapp use_cases use_case
%type <ast> type_decl externals impList alias
%type <ast> type_cpair eform_cpair
%type <ast> value_definition tc_definition ti_definition
%type <ast> import_definition provide_definition
%type <ast> tc_decls tc_decl ow
%type <ast> declares declare decls decl
%type <ast> constructors constructor 
%type <ast> repr_constructors repr_constructor 
%type <ast> repr_reprs repr_repr 
%type <ast> bindingpattern lambdapatterns lambdapattern
%type <ast> expr_seq 
%type <ast> qual_type qual_constraint
%type <ast> expr the_expr eform method_decls method_decl method_seq
%type <ast> constraints constraint_seq constraint 
%type <ast> types type bitfieldtype bool_type 
%type <ast> type_pl_bf type_pl_byref types_pl_byref
%type <ast> int_type uint_type any_int_type float_type
%type <ast> tvlist fields field 
%type <ast> literal typevar //mod_ident
%type <ast> ident defident useident switch_matches switch_match
%type <ast> exident
%type <ast> docstring optdocstring
%type <ast> condcases condcase fntype
 //%type <ast> reprbody reprbodyitems reprbodyitem reprtags reprcase reprcaseleg
//%type <ast> typecase_leg typecase_legs 
%type <ast> sw_legs sw_leg otherwise
//%type <ast> catchclauses catchclause
%type <ast> letbindings letbinding
%type <ast> dobindings ne_dobindings dobinding dotest
%type <ast> intLit floatLit charlit strLit boolLit  
%type <ast> let_eform
%type <ast> type_val_definition
%type <ast> constrained_definition
%%

// Parser built for version 0.9
// Section Numbers indicated within []

// COMPILATION UNITS [2.5]
// This definition od start mus be changed as it ignores
// junk after the body.

start: version uoc_body {
  SHOWPARSE("start -> version uoc_body");
  return 0;
};

start: uoc_body {
  SHOWPARSE("start -> uoc_body");
  return 0;
};

//start: interface {
//  SHOWPARSE("start -> interface");
//
//  return 0;
//};

uoc_body: interface {
  SHOWPARSE("uocbody -> interface");
}

uoc_body: implicit_module {
  SHOWPARSE("uocbody -> implicit_module");
}

uoc_body: module_seq {
  SHOWPARSE("uocbody -> module_seq");
}

// VERSION [2.5]

// We cannot do optversion, because that would require two token look-ahead.
version: '(' tk_BITC_VERSION strLit ')' {
  SHOWPARSE("version -> ( BITC-VERSION strLit )");
  if(!CheckVersionCompatibility($3->s)) {
    std::string s = ": Warning: BitC version conflict " + $3->s + " vs " + Version();
    lexer->ReportParseWarning($3->loc, s);
  }
}; 

// It would be really nice to support this, but the tokenizer will
// report the literal as a floating point value. We could, I suppose,
// crack that back into its constituent parts, but I don't have the
// energy to implement that today.
// version: '(' tk_BITC_VERSION intLit '.' intLit ')' {
//   SHOWPARSE("version -> ( BITC-VERSION intLit '.' intLit )");
//   if(!CheckVersionCompatibility($3->litValue.i, $5->litValue.i)) {
//     std::string s = ": Warning: BitC version conflict " + $3->s + " vs " + Version();
//     lexer->ReportParseWarning($3->loc, s);
//   }
// }; 

// Documentation comments. These are added only in productions where
// they do NOT appear before expr_seq. If a string literal appears as
// the first form of a multiform expr_seq, it won't hurt anything. If
// it is the *only* form, then it is the value in any case, and that
// is fine. We can figure out which case is which in the documentation
// extractor.
docstring: tk_String {
  SHOWPARSE("docstring -> STRING");
  $$ = new AST(at_docString, $1.loc, AST::makeStringLit($1));
};
optdocstring: docstring {
  SHOWPARSE("optdocstring -> docstring");
  $$ = $1;
};
optdocstring: {
  SHOWPARSE("optdocstring -> ");
  $$ = new AST(at_docString);
};

// TODO: The ident in interface rule should be restricted to 
// ({ALPHA} | [_\-]) (({ALPHA} | {DECDIGIT} | [_\-])*

// INTERFACES [8.1]
interface: '(' tk_INTERFACE ifident {
    if ($3.str.find("bitc.") == 0)
      lexer->isRuntimeUoc = true;
  }
  optdocstring if_definitions ')' {
  SHOWPARSE("interface -> INTERFACE ifident optdocstring if_definitions");  
  GCPtr<AST> ifIdent = new AST(at_ident, $3);
  $$ = new AST(at_interface, $2.loc, ifIdent);
  $$->addChildrenFrom($6);

  if (lexer->isCommandLineInput) {
    char *s =
      ": Warning: interface units of compilation should no longer\n"
      "    be given on the command line.\n";
    lexer->ReportParseWarning($$->loc, s);
  }

  std::string uocName = ifIdent->s;
  GCPtr<UocInfo> uoc = 
    new UocInfo(uocName, lexer->here.origin, $$);

  if(uocName == "bitc.prelude")
    uoc->flags |= (UOC_IS_BUILTIN | UOC_IS_PRELUDE);

  GCPtr<UocInfo> existingUoc = UocInfo::findInterface(uocName);

  if (existingUoc) {
    std::string s = "Error: This interface has already been loaded from "
      + existingUoc->uocAst->loc.asString();
    lexer->ReportParseError($$->loc, s);

  }
  else  {
    /* Put the UoC onto the interface list so that we do not recurse on
       import. */
    UocInfo::ifList->append(uoc);
  }
    
  // Regardless, compile the new interface to check for further
  // warnings and/or errors:
  uoc->Compile();

#if 0
  // The following code was in the old implementation of
  // importInterface, but it seems that it must have been broken,
  // because UocInfo::ifList was never replaced with newIfList. I am
  // retaining it here long enough to check with Swaroop why this was
  // okay.

  // The interface that we just compiled may have (recursively)
  // imported other interfaces. Now that we have successfully compiled
  // this interface, migrate it to the end of the interface list.
  GCPtr< CVector<GCPtr<UocInfo> > > newIfList = new CVector<GCPtr<UocInfo> >;  

  for (size_t i = 0; i < UocInfo::ifList->size(); i++)
    if (UocInfo::ifList->elem(i) != puoci) 
      newIfList->append(UocInfo::ifList->elem(i));  
  newIfList->append(puoci);
#endif
};

ifident: {
    lexer->setIfIdentMode(true);
  } ident {
  lexer->setIfIdentMode(false);
  $$ = LToken($2->loc, $2->s);
};
ifident: ifident '.' {
    lexer->setIfIdentMode(true);
  } tk_Ident {
  lexer->setIfIdentMode(false);
  $$ = LToken($1.loc, $1.str + "." + $4.str);
};

// MODULES [2.5]
implicit_module: mod_definitions  {
 SHOWPARSE("implicit_module -> mod_definitions");
 $$ = $1;
 $$->astType = at_module;
 $$->printVariant = 1;

 // Construct, compile, and admit the parsed UoC:
 GCPtr<UocInfo> uoc = 
   new UocInfo(UocInfo::UocNameFromSrcName(lexer->here.origin, lexer->nModules),
	       lexer->here.origin, $$);
 lexer->nModules++;

 uoc->Compile();
 UocInfo::srcList->append(uoc);
};

module: '(' tk_MODULE optdocstring mod_definitions ')' {
 SHOWPARSE("module -> ( tk_MODULE optdocstring mod_definitions )");
 $$ = $4;
 $$->astType = at_module;

 // Construct, compile, and admit the parsed UoC:
 GCPtr<UocInfo> uoc = 
   new UocInfo(UocInfo::UocNameFromSrcName(lexer->here.origin, lexer->nModules),
	       lexer->here.origin, $$);
 lexer->nModules++;
 uoc->Compile();
 UocInfo::srcList->append(uoc);
};

module: '(' tk_MODULE ifident optdocstring mod_definitions ')' {
 SHOWPARSE("module -> ( tk_MODULE ifident optdocstring mod_definitions )");
 $$ = $5;
 $$->astType = at_module;

 // Construct, compile, and admit the parsed UoC.
 // Note that we do not even consider the user-provided module name
 // for purposes of internal naming, because it is not significant.
 GCPtr<UocInfo> uoc = 
   new UocInfo(UocInfo::UocNameFromSrcName(lexer->here.origin, lexer->nModules),
	       lexer->here.origin, $$);
 lexer->nModules++;
 uoc->Compile();
 UocInfo::srcList->append(uoc);
};

module_seq: module {
 SHOWPARSE("module_seq -> module");
}

module_seq: module_seq module {
 SHOWPARSE("module_seq -> module_seq module");
}

// INTERFACE TOP LEVEL DEFINITIONS
if_definitions: if_definition {
  SHOWPARSE("if_definitions -> if_definition");
  $$ = new AST(at_Null, $1->loc, $1);
};

if_definitions: if_definitions if_definition {
  SHOWPARSE("if_definitions -> if_definitions if_definition");
  $$ = $1;
  $$->addChild($2);   
};

if_definition: common_definition {
  SHOWPARSE("if_definition -> common_definition");
  $$ = $1;
};

// TOP LEVEL DEFINITIONS [2.5.1]
constrained_definition: '(' tk_FORALL constraints type_val_definition ')' {
  // HACK ALERT!!! For reasons of ancient history, there is no
  // at_forall AST. Instead, all of the type_val_definition ASTs have
  // their constraints tacked on at the end. This is rather badly
  // glitched and we need to fix it, but the immediate goal was to
  // move the (forall ...) syntax to the outside without doing major
  // surgery on the compiler internals.

  uint32_t nChildren = $4->children->size();

  GCPtr<AST> tvConstraints= $4->child(nChildren-1);
  tvConstraints->addChildrenFrom($3);
  $$ = $4;
};

mod_definitions: mod_definition {
  SHOWPARSE("definitions -> definition");
  $$ = new AST(at_Null, $1->loc, $1);
};

mod_definitions: mod_definitions mod_definition {
  SHOWPARSE("definitions -> definitions definition");
  $$ = $1;
  $$->addChild($2);   
};

mod_definition: provide_definition {
  SHOWPARSE("definition -> provide_definition");
  $$ = $1;
};
mod_definition: common_definition {
  SHOWPARSE("definition -> common_definition");
  $$ = $1;
};

common_definition: import_definition {
  SHOWPARSE("type_val_definition -> import_definition");
  $$ = $1;
};

common_definition: type_val_definition {
  SHOWPARSE("common_definition -> type_val_definition");
  $$ = $1;
};

common_definition: constrained_definition {
  SHOWPARSE("common_definition -> constrained_definition");
  $$ = $1;
};

type_val_definition: type_definition {
  SHOWPARSE("type_val_definition -> type_definition");
  $$ = $1;
};

type_val_definition: type_decl {
  SHOWPARSE("type_val_definition -> type_decl");
  $$ = $1;
};

type_val_definition: value_definition {
  SHOWPARSE("type_val_definition -> value_definition");
  $$ = $1;
};

type_val_definition: proclaim_definition {
  SHOWPARSE("type_val_definition -> proclaim_definition");
  $$ = $1;
};

type_val_definition: tc_definition {
  SHOWPARSE("type_val_definition -> tc_definition");
  $$ = $1;
};

type_val_definition: ti_definition {
  SHOWPARSE("type_val_definition -> ti_definition");
  $$ = $1;
};

// DECLARE [8.4.2]
common_definition: declare {
  SHOWPARSE("common_definition -> declare");
  $$ = $1;
};

//Typeclass constraint declarations

constraints: '(' constraint_seq ')' {
 SHOWPARSE("constraints -> ( constraint_seq )");  
 $$ = $2; 
};

constraint_seq: constraint_seq constraint {
 SHOWPARSE("constraint_seq -> constraint_seq constraint");  
 $$ = $1;
 $$->addChild($2);
};
 
constraint_seq: constraint {
 SHOWPARSE("constraint_seq -> constraint");  
 $$ = new AST(at_constraints, $1->loc, $1);
};

constraint: typapp {
 SHOWPARSE("constraint -> typapp");  
 $1->astType = at_tcapp;
 $$ = $1;
};

constraint: useident {
 SHOWPARSE("constraint -> useident");  
 $$ = new AST(at_tcapp, $1->loc, $1); 
};


// FIX: This should probably get its own AST type.
ptype_name: defident {
  SHOWPARSE("ptype_name -> defident");
  GCPtr<AST> tvlist = new AST(at_tvlist, $1->loc);
  GCPtr<AST> constraints = new AST(at_constraints, $1->loc);
  $$ = new AST(at_Null, $1->loc, $1, tvlist, constraints);  
};

ptype_name: '(' defident tvlist ')' {
  SHOWPARSE("ptype_name -> '(' defident tvlist ')'");
  GCPtr<AST> constraints = new AST(at_constraints, $2->loc);
  $$ = new AST(at_Null, $2->loc, $2, $3, constraints);
};

ptype_name: '(' tk_FORALL constraints defident ')' {
  SHOWPARSE("ptype_name -> '(' FORALL constraints '(' defident tvlist ')' ')' ");
  GCPtr<AST> tvlist = new AST(at_tvlist, $4->loc);
  $$ = new AST(at_Null, $2.loc, $4, tvlist, $3);
};

ptype_name: '(' tk_FORALL constraints '(' defident tvlist ')' ')' {
  SHOWPARSE("ptype_name -> '(' FORALL constraints '(' defident tvlist ')' ')' ");
  $$ = new AST(at_Null, $2.loc, $5, $6, $3);
};


// STRUCTURE TYPES [3.6.1]           
type_definition: '(' tk_DEFSTRUCT ptype_name val optdocstring declares fields ')'  {
  SHOWPARSE("type_definition -> ( DEFSTRUCT ptype_name val "
	    "optdocstring declares fields )");
  $$ = new AST(at_defstruct, $2.loc, $3->child(0), $3->child(1), $4,
	       $6, $7);
  $$->child(0)->defForm = $$;
  $$->addChild($3->child(2));
};


// UNION TYPES [3.6.2]                
type_definition: '(' tk_DEFUNION ptype_name val optdocstring declares constructors  ')'  {
  SHOWPARSE("type_definition -> ( DEFUNION ptype_name val "
	    "optdocstring declares constructors");
  $$ = new AST(at_defunion, $2.loc, $3->child(0), $3->child(1), $4,
	       $6, $7);
  $$->child(0)->defForm = $$;
  $$->addChild($3->child(2));  
};

/* // REPR TYPES */
/* type_definition: '(' tk_DEFREPR defident val optdocstring declares reprbody  ')'  { */
/*   SHOWPARSE("type_definition -> ( DEFUNION ptype_name val " */
/* 	    "optdocstring declares reprbody"); */
/*   $$ = new AST(at_defrepr, $2.loc, $3->child(0), $3->child(1), $4, */
/* 	       $6, $7); */
/*   $$->addChild($3->child(2));   */
/* }; */
/* reprbody: '(' reprbodyitems ')' { */
/*   SHOWPARSE("reprbody -> reprbodyitems"); */
/*   $$ = $2; */
/* }; */

/* reprbodyitems: reprbodyitem { */
/*   SHOWPARSE("reprbodyitems -> reprbodyitem"); */
/*   $$ = new AST(at_reprbody, $1->loc, $1); */
/* }; */

/* reprbodyitems: reprbodyitems reprbodyitem { */
/*   SHOWPARSE("reprbodyitems -> reprbodyitems reprbodyitem"); */
/*   $$ = $1; */
/*   $$->addChild($2); */
/* }; */

/* reprbodyitem: field { */
/*   SHOWPARSE("reprbodyitem -> field"); */
/*   $$ = $1; */
/* }; */

/* reprbodyitem: '(' tk_TAG reprtags ')' { */
/*   SHOWPARSE("reprbodyitem -> '(' TAG reprtags ')' "); */
/*   $$ = $3; */
/*   $$->loc = $2.loc; */
/* }; */

/* reprbodyitem: '(' tk_THE type_pl_bf '(' tk_TAG reprtags ')' ')' { */
/*   SHOWPARSE("reprbodyitem -> '(' TAG reprtags ')' "); */
/*   $$ = $6; */
/*   $$->loc = $5.loc; */
/* }; */

/* reprbodyitem: '(' tk_CASE reprcase ')' { */
/*   SHOWPARSE("reprbodyitem -> '(' CASE reprcases ')' "); */
/*   $$ = $3; */
/*   $$->loc = $2.loc; */
/* }; */

/* reprcase: reprcaseleg { */
/*   SHOWPARSE("reprcase -> reprcaseleg"); */
/*   $$ = new AST(at_reprcase, $1->loc, $1); */
/* }; */

/* reprcase: reprcase reprcaseleg { */
/*   SHOWPARSE("reprcase -> reprcase reprcaseleg"); */
/*   $$ = $1; */
/*   $$->addChild($2); */
/* }; */

/* reprcaseleg: '(' reprtags reprbody ')' { */
/*   SHOWPARSE("reprcaseleg -> reprtags reprbody"); */
/*   $$ = new AST(at_reprcaselegR, $1.loc, $3); */
/*   $$->addChildrenFrom($2); */
/* }; */

/* reprtags: ident { */
/*   SHOWPARSE("reprtags -> ident"); */
/*   $$ = new AST(at_reprtag, $1->loc, $1); /\* dummy AST type *\/ */
/* }; */

/* reprtags: reprtags ident { */
/*   SHOWPARSE("reprtags -> reprtags ident"); */
/*   $$ = $1; */
/*   $$->addChild($2); */
/* }; */

// REPR TYPES
type_definition: '(' tk_DEFREPR defident val optdocstring declares repr_constructors  ')'  {
  SHOWPARSE("type_definition -> ( DEFREPR defident val "
	    "optdocstring declares repr_constructors");
  $$ = new AST(at_defrepr, $2.loc, $3, $4, $6, $7);
  $$->child(0)->defForm = $$;
};

// Type Declarations
// External declarations
externals: /* nothing */ {
  SHOWPARSE("externals -> ");
  $$ = new AST(at_Null);
  $$->Flags = 0;
};

externals: tk_EXTERNAL {
  SHOWPARSE("externals -> EXTERNAL");
  $$ = new AST(at_Null, $1.loc);
  $$->Flags = DEF_IS_EXTERNAL;
};

externals: tk_EXTERNAL exident {
  SHOWPARSE("externals -> EXTERNAL exident");
  $$ = new AST(at_Null, $1.loc);
  $$->Flags = DEF_IS_EXTERNAL;  
  $$->externalName = $2->s;
};


// STRUCTURE DECLARATIONS
type_decl: '(' tk_DEFSTRUCT ptype_name val externals ')' {
  SHOWPARSE("type_decl -> ( DEFSTRUCT ptype_name val externals )");
  $$ = new AST(at_declstruct, $2.loc, $3->child(0), $3->child(1), $4,
	       $3->child(2));
  $$->child(0)->defForm = $$;
  $$->Flags |= $5->Flags;
  $$->getID()->Flags |= $5->Flags;
  $$->getID()->externalName = $5->externalName;
};

// UNION DECLARATIONS
type_decl: '(' tk_DEFUNION ptype_name val externals ')' {
  SHOWPARSE("type_decl -> ( DEFUNION ptype_name val )");
  $$ = new AST(at_declunion, $2.loc, $3->child(0), $3->child(1), $4,
	       $3->child(2));
  $$->child(0)->defForm = $$;
  $$->Flags |= $5->Flags;
  $$->getID()->Flags |= $5->Flags;
  $$->getID()->externalName = $5->externalName;
};

// REPR DECLARATIONS
type_decl: '(' tk_DEFREPR defident val externals ')' {
  SHOWPARSE("type_decl -> ( DEFREPR defident val externals )");
  $$ = new AST(at_declrepr, $2.loc, $3, $4);
  $$->child(0)->defForm = $$;
  $$->Flags |= $5->Flags;
  $$->getID()->Flags |= $5->Flags;
  $$->getID()->externalName = $5->externalName;
};

// CATEGORIES

val: { 
  SHOWPARSE("val -> <empty>");
  $$ = new AST(at_refCat);
  $$->printVariant = 1;
};

val: ':' tk_VAL {
  SHOWPARSE("val -> ':' VAL");
  $$ = new AST(at_valCat, $2);
};
val: ':' tk_OPAQUE {
  SHOWPARSE("val -> ':' OPAQUE");
  $$ = new AST(at_opaqueCat, $2);
};
val: ':' tk_REF {
  /* Same as :ref, since that is the default. */
  SHOWPARSE("val -> ':' REF");
  $$ = new AST(at_refCat, $2);
};
 
// EXCEPTION DEFINITION [3.10]
type_definition: '(' tk_DEFEXCEPTION ident optdocstring ')' {
  SHOWPARSE("type_definition -> ( defexception ident )");
  $3->Flags |= ID_IS_GLOBAL;
  $$ = new AST(at_defexception, $2.loc, $3);
  $$->child(0)->defForm = $$;
};

type_definition: '(' tk_DEFEXCEPTION ident optdocstring fields ')' {
  SHOWPARSE("type_definition -> ( defexception ident fields )");
  $3->Flags |= ID_IS_GLOBAL;
  $$ = new AST(at_defexception, $2.loc, $3);
  $$->child(0)->defForm = $$;
  $$->addChildrenFrom($5);
};

// TYPE CLASSES [4]
// TYPE CLASS DEFINITION [4.1]

tc_definition: '(' tk_DEFTYPECLASS ptype_name optdocstring tc_decls method_decls ')' {
  SHOWPARSE("tc_definition -> ( DEFTYPECLASS ptype_name optdocstring tc_decls method_decls)");
  $$ = new AST(at_deftypeclass, $2.loc, $3->child(0), 
	       $3->child(1), $5, $6, $3->child(2));  
  $$->child(0)->defForm = $$;
};

tc_decls: {
  SHOWPARSE("tcdecls -> <empty>");
  $$ = new AST(at_tcdecls);
};

tc_decls: tc_decls tc_decl {
  SHOWPARSE("tcdecls -> tcdelcs tcdecl");
  $$ = $1;
  $$->addChild($2);
};

tc_decl: '(' tk_TYFN '(' tvlist ')' typevar ')' {
  //                     ^^^^^^
  // I really mean tvlist here, arbitraty types
  // are not accepptable.
  SHOWPARSE("tc_decl -> ( TYFN ( tvlist ) typevar )");
  $4->astType = at_fnargVec;
  $$ = new AST(at_tyfn, $2.loc, $4, $6);  
};

method_decls: /* Nothing */ {
  SHOWPARSE("method_decls -> ");
  LexLoc loc;
  $$ = new AST(at_method_decls, loc);
};

method_decls: method_decls method_decl {
  SHOWPARSE("method_decls -> method_decls method_decl");
  $$ = $1;
  $$->addChild($2);
};

method_decl: ident ':' fntype {
  SHOWPARSE("method_decl -> ident : fntype");
  $1->Flags |= (ID_IS_GLOBAL | ID_IS_METHOD);
  $$ = new AST(at_method_decl, $1->loc, $1, $3);
};

// TYPE CLASS INSTANTIATIONS [4.2]
// No docstring here because method_seq is really a potentially empty
// expr_seq
ti_definition: '(' tk_DEFINSTANCE qual_constraint method_seq ')' {
  SHOWPARSE("ti_definition -> ( DEFINSTANCE qual_constraint [docstring] method_seq)");
  $4 = stripDocString($4);
  $$ = new AST(at_definstance, $2.loc, $3->child(1), $4, 
	       $3->child(0));  
};

method_seq: /* Nothing */ {
  SHOWPARSE("method_seq -> ");
  LexLoc loc;
  $$ = new AST(at_methods, loc);
};

method_seq: expr_seq {
  SHOWPARSE("method_seq -> expr_seq");
  $$ = $1;
  $$->astType = at_methods;
};

// DEFINE  [5.1]
value_definition: '(' tk_DEFINE defpattern expr ')'  {
  SHOWPARSE("value_definition -> ( DEFINE  defpattern expr )");
  $$ = new AST(at_define, $2.loc, $3, $4);
  $$->addChild(new AST(at_constraints));
};
value_definition: '(' tk_DEFINE defpattern docstring expr ')'  {
  SHOWPARSE("value_definition -> ( DEFINE  defpattern docstring expr )");
  $$ = new AST(at_define, $2.loc, $3, $5);
  $$->addChild(new AST(at_constraints));
};

// Define convenience syntax case 1: no arguments
// No docstring here because of expr_seq
value_definition: '(' tk_DEFINE '(' defident ')' expr_seq ')'  {
  SHOWPARSE("value_definition -> ( DEFINE  ( defident ) [docstring] expr_seq )");
  $6 = stripDocString($6);
  GCPtr<AST> iLambda =
    new AST(at_lambda, $2.loc, new AST(at_argVec, $5.loc), $6);
  iLambda->printVariant = 1;
  GCPtr<AST> iP = new AST(at_identPattern, $4->loc, $4);
  $$ = new AST(at_define, $2.loc, iP, iLambda);
  $$->addChild(new AST(at_constraints));
};

// Define convenience syntax case 3: one or more arguments
// No docstring here because of expr_seq
value_definition: '(' tk_DEFINE '(' defident lambdapatterns ')' 
                  expr_seq ')'  {
  SHOWPARSE("value_definition -> ( DEFINE  ( defident lambdapatterns ) "
	    "[docstring] expr_seq )");
  $7 = stripDocString($7);
  GCPtr<AST> iLambda = new AST(at_lambda, $2.loc, $5, $7);
  iLambda->printVariant = 1;
  GCPtr<AST> iP = new AST(at_identPattern, $4->loc, $4);
  $$ = new AST(at_define, $2.loc, iP, iLambda);
  $$->addChild(new AST(at_constraints));
};

// PROCLAIM DEFINITION -- VALUES [6.2]
proclaim_definition: '(' tk_PROCLAIM defident ':' qual_type optdocstring externals ')' {
  SHOWPARSE("if_definition -> ( PROCLAIM ident : qual_type externals optdocstring )");
  $$ = new AST(at_proclaim, $2.loc, $3, $5);
  $$->Flags |= $7->Flags;
  $$->getID()->Flags |= $7->Flags;
  $$->getID()->externalName = $7->externalName;
  $$->addChild(new AST(at_constraints));
};

// TODO: The second ident in import rule, and the ident in the provide rule 
//  should be restricted to 
// ({ALPHA} | [_\-]) (({ALPHA} | {DECDIGIT} | [_\-])*
// IMPORT DEFINITIONS [8.2]

import_definition: '(' tk_IMPORT ident ifident ')' {
  SHOWPARSE("import_definition -> ( IMPORT ident ifident )");
  $3->uoc = UocInfo::importInterface(lexer->errStream, $4.loc, $4.str);
  GCPtr<AST> ifIdent = new AST(at_ifident, $4);
  $$ = new AST(at_import, $2.loc, $3, ifIdent); 
};

// USE [8.2]
import_definition:  '(' tk_USE use_cases ')' {
  SHOWPARSE("import_definition -> ( USE uses )");
  $$ = $3;
  $$->loc = $2.loc;
  $$->astType = at_use;
};

import_definition: '(' tk_FROM ifident ')' {
  SHOWPARSE("import_definition -> (FROM ifident)");
  UocInfo::importInterface(lexer->errStream, $3.loc, $3.str);
  GCPtr<AST> ifIdent = new AST(at_ifident, $3);
  $$ = new AST(at_from, $2.loc, ifIdent);
};

import_definition: '(' tk_FROM ifident tk_IMPORT impList ')' {
  SHOWPARSE("import_definition -> (FROM ifident IMPORT impList)");
  UocInfo::importInterface(lexer->errStream, $3.loc, $3.str);
  GCPtr<AST> ifIdent = new AST(at_ifident, $3);
  $$ = new AST(at_from, $2.loc, ifIdent);
  $$->addChildrenFrom($5);
};

// PROVIDE DEFINITION [8.3]
provide_definition: '(' tk_PROVIDE ident ifident ')' {
  SHOWPARSE("provide_definition -> (PROVIDE ident ifident)");
  $3->uoc = UocInfo::importInterface(lexer->errStream, $4.loc, $4.str);
  GCPtr<AST> ifIdent = new AST(at_ifident, $4);
  $$ = new AST(at_provide, $2.loc, $3, ifIdent); 
};

use_cases: use_case {
  SHOWPARSE("use_cases -> use_case");
  $$ = new AST(at_Null, $1->loc, $1);
};
use_cases: use_cases use_case {
  SHOWPARSE("use_cases -> use_cases use_case");
  $$ = $1;
  $$->addChild($2);
};
use_case: ident '.' ident { 
  SHOWPARSE("use_case -> ident . ident");
  GCPtr<AST> newident = new AST(at_ident, $3->loc);
  newident->s = $3->s;
  newident->Flags |= (ID_IS_GLOBAL);
  GCPtr<AST> usesel = new AST(at_usesel, $1->loc, $1, $3);
  $$ = new AST(at_use_case, $1->loc, newident, usesel);
  $$->printVariant = 1;
};
use_case: '(' ident ident '.' ident ')' { 
  SHOWPARSE("use_case -> (ident ident :: ident)");
  $2->Flags |= (ID_IS_GLOBAL);
  GCPtr<AST> usesel = new AST(at_usesel, $3->loc, $3, $5);
  $$ = new AST(at_use_case, $2->loc, $2, usesel);
};

impList: alias {
  SHOWPARSE("impList -> alias");
  $$ = new AST(at_Null, $1->loc, $1);
};
impList: impList alias {
  SHOWPARSE("impList -> impList alias");
  $$ = $1;
  $$->addChild($2);
};

alias: ident {
  SHOWPARSE("alias -> ident");
  $$ = new AST(at_ifsel, $1->loc, $1, $1->getDCopy());
};
alias: '(' ident ident ident ')' {
  SHOWPARSE("alias -> ( = ident ident )");
  
  if($2->s != "=") {
    cerr << $2->loc << ": Syntax error, expecting `='.\n";
    lexer->num_errors++;
  }

  $$ = new AST(at_ifsel, $2->loc, $3, $4);
};


// definition: '(' tk_DEFTHM ident expr ')'  {
//    SHOWPARSE("definition -> ( DEFTHM ident expr )");
//    $$ = new AST(at_defthm, $2.loc, $3, $4);
// };
// definition: '(' tk_MUTUAL_RECURSION definitions ')'  {
//    SHOWPARSE("definition -> ( MUTUAL-RECURSION Defs )");
//    $$ = new AST(at_mutual_recursion, $2.loc);   
//    $$->addChildrenFrom($3);
// };

declares: {
  SHOWPARSE("declares -> <empty>");
  $$ = new AST(at_declares);
};
declares: declares declare {
  SHOWPARSE("declares -> declares declare");
  $$ = $1;
  $$->addChildrenFrom($2);
};

declare: '(' tk_DECLARE decls ')' {
  SHOWPARSE("declare -> ( DECLARE decls )");
  $$ = $3;
};

decls: decl {
  SHOWPARSE("decls -> decl");
  $$ = new AST(at_declares, $1->loc, $1);
};

decls: decls decl {
  SHOWPARSE("decls -> decls decl");
  $$ = $1;
  $$->addChild($2);
};

decl: '(' ident type_pl_bf ')' {
  SHOWPARSE("decl -> ( ident type_pl_bf )");
  $$ = new AST(at_declare, $2->loc, $2, $3);
};
//decl: '(' ident ')' {
//  SHOWPARSE("decl -> ( ident )");
//  $$ = new AST(at_declare, $2->loc, $2);
//};
decl: ident {
  SHOWPARSE("decl -> ident");
  $$ = new AST(at_declare, $1->loc, $1);
};


/* defunion Constructors */
constructors: constructor {
  SHOWPARSE("constructors -> constructor");
  $$ = new AST(at_constructors, $1->loc, $1);
};
constructors: constructors constructor {
  SHOWPARSE("constructors -> constructors constructor");
  $$ = $1;
  $$->addChild($2);
};
constructor: ident { 	       	  /* simple constructor */ 
  SHOWPARSE("constructor -> defident");
  $1->Flags |= (ID_IS_GLOBAL);
  $$ = new AST(at_constructor, $1->loc, $1);
};
constructor: '(' ident fields ')' {  /* compound constructor */ 
  SHOWPARSE("constructor ->  ( ident fields )");
  $2->Flags |= (ID_IS_GLOBAL);
  $$ = new AST(at_constructor, $2->loc, $2);
  $$->addChildrenFrom($3);
};

/* defrepr Constructors */
repr_constructors: repr_constructor {
  SHOWPARSE("repr_constructors -> repr_constructor");
  $$ = new AST(at_reprctrs, $1->loc, $1);
};
repr_constructors: repr_constructors repr_constructor {
  SHOWPARSE("repr_constructors -> repr_constructors repr_constructor");
  $$ = $1;
  $$->addChild($2);
};
/* repr_constructor: ident repr_reprs { 	       	  /\* simple constructor *\/  */
/*   SHOWPARSE("repr_constructor -> defident"); */
/*   $1->Flags |= (ID_IS_GLOBAL); */
/*   $$ = new AST(at_reprctr, $1->loc, $1); */
/* }; */
repr_constructor: '(' ident fields '('tk_WHERE repr_reprs')' ')' {  /* compound constructor */ 
  SHOWPARSE("repr_constructor ->  ( ident fields ( WHERE repr_reprs ) )");
  $2->Flags |= (ID_IS_GLOBAL);
  GCPtr<AST> ctr = new AST(at_constructor, $2->loc, $2);
  ctr->addChildrenFrom($3);
  $$ = new AST(at_reprctr, $2->loc, ctr);
  $$->addChildrenFrom($6);
};

repr_reprs: repr_repr {
  SHOWPARSE("repr_reprs -> repr_repr");
  $$ = new AST(at_Null, $1->loc, $1);
};
repr_reprs: repr_reprs repr_repr {
  SHOWPARSE("repr_reprs -> repr_reprs repr_repr");
  $$ = $1;
  $$->addChild($2);
};
repr_repr: '(' ident ident intLit')' {
  SHOWPARSE("repr_repr ->  ( = ident intLit )");

  if($2->s != "=") {
    cerr << $2->loc << ": Syntax error, expecting `='.\n";
    lexer->num_errors++;
  }
  
  $$ = new AST(at_reprrepr, $2->loc, $3, $4);
};


/* defstruct / constructor / exception fields */
fields: field  {
  SHOWPARSE("fields -> field");
  $$ = new AST(at_fields, $1->loc, $1);
};

fields: fields field {
  SHOWPARSE("fields -> fields field ");
  $$ = $1;
  $$->addChild($2);
};

field: ident ':' type_pl_bf  {
  SHOWPARSE("field -> ident : type_pl_bf");
  $1->Flags |= ID_IS_FIELD;
  $$ = new AST(at_field, $1->loc, $1, $3);
};

field: '(' tk_THE type_pl_bf ident ')'  {
  SHOWPARSE("field -> '(' THE type_pl_bf ident ')'");
  $4->Flags |= ID_IS_FIELD;
  $$ = new AST(at_field, $1.loc, $4, $3);
};

field: '(' tk_FILL bitfieldtype ')'  {
  SHOWPARSE("field -> '(' FILL type ')'");
  $$ = new AST(at_fill, $1.loc, $3);
};

field: '(' tk_RESERVED bitfieldtype intLit ')'  {
  SHOWPARSE("field -> '(' RESERVED bitfieldtype intLit ')'");
  $$ = new AST(at_fill, $1.loc, $3);
};

tvlist: typevar  {
  SHOWPARSE("tvlist -> typevar");
  $$ = new AST(at_tvlist, $1->loc, $1);
};
tvlist: tvlist typevar {
  SHOWPARSE("tvlist -> tvlist typevar");
  $$ = $1;
  $1->addChild($2);
};

// TYPES [3]
types: type  {
  SHOWPARSE("types -> type");
  $$ = new AST(at_Null);
  $$->addChild($1);
}; 
types: types type {
  SHOWPARSE("types -> types type");
  $$ = $1;
  $1->addChild($2);
};

type: useident  { 			/* previously defined type */ 
  SHOWPARSE("type -> useident");
  $$ = $1;
};

// PRIMARY TYPES [3.2]             
type: '(' ')' {
  SHOWPARSE("type -> ( )");
  $$ = new AST(at_primaryType, $1.loc);
  $$->s = "unit";		/* for lookup! */
};

bool_type: tk_BOOL {
  SHOWPARSE("bool_type -> BOOL");
  $$ = new AST(at_primaryType, $1);
};

type: bool_type {
  SHOWPARSE("type -> bool_type");
  $$ = $1;
};

type: tk_CHAR {
  SHOWPARSE("type -> CHAR");
  $$ = new AST(at_primaryType, $1);
};
type: tk_STRING {
  SHOWPARSE("type -> STRING");
  $$ = new AST(at_primaryType, $1);
};

int_type: tk_INT8 {
  SHOWPARSE("int_type -> INT8");
  $$ = new AST(at_primaryType, $1);
};
int_type: tk_INT16 {
  SHOWPARSE("int_type -> INT16");
  $$ = new AST(at_primaryType, $1);
};
int_type: tk_INT32 {
  SHOWPARSE("int_type -> INT32");
  $$ = new AST(at_primaryType, $1);
};
int_type: tk_INT64 {
  SHOWPARSE("int_type -> INT64");
  $$ = new AST(at_primaryType, $1);
};
uint_type: tk_UINT8 {
  SHOWPARSE("uint_type -> UINT8");
  $$ = new AST(at_primaryType, $1);
};
uint_type: tk_UINT16 {
  SHOWPARSE("uint_type -> UINT16");
  $$ = new AST(at_primaryType, $1);
};
uint_type: tk_UINT32 {
  SHOWPARSE("uint_type -> UINT32");
  $$ = new AST(at_primaryType, $1);
};
uint_type: tk_UINT64 {
  SHOWPARSE("type -> UINT64");
  $$ = new AST(at_primaryType, $1);
};

any_int_type: int_type {
  SHOWPARSE("any_int_type -> int_type");
  $$ = $1;
};
any_int_type: uint_type {
  SHOWPARSE("any_int_type -> uint_type");
  $$ = $1;
};
any_int_type: tk_WORD {
  SHOWPARSE("any_int_type -> WORD");
  $$ = new AST(at_primaryType, $1);
};

float_type: tk_FLOAT {
  SHOWPARSE("float_type -> FLOAT");
  $$ = new AST(at_primaryType, $1);
};
float_type: tk_DOUBLE {
  SHOWPARSE("float_type -> DOUBLE");
  $$ = new AST(at_primaryType, $1);
};
float_type: tk_QUAD {
  SHOWPARSE("float_type -> QUAD");
  $$ = new AST(at_primaryType, $1);
};

type: any_int_type {
  SHOWPARSE("type -> any_int_type");
  $$ = $1;
};
type: float_type {
  SHOWPARSE("type -> float_type");
  $$ = $1;
};

// EXCEPTION type
type: tk_EXCEPTION {
  SHOWPARSE("type -> EXCEPTION");
  $$ = new AST(at_exceptionType, $1.loc);
};

// TYPE VARIABLES [3.3]            
type: typevar  { 		
  SHOWPARSE("type -> typevar");
  $$ = $1;
};

// REF TYPES [3.4.1]               
type: '(' tk_REF type ')' {
  SHOWPARSE("type -> ( REF type )");
  $$ = new AST(at_refType, $2.loc, $3);
};

// VAL TYPES [3.4.2]
type: '(' tk_VAL type ')' {
  SHOWPARSE("type -> ( VAL type )");
  $$ = new AST(at_valType, $2.loc, $3);
};

// FUNCTION TYPES [3.4.3]
type: fntype {
  SHOWPARSE("type -> fntype");
  $$ = $1;
}
fntype: '(' tk_FN '(' ')' type ')' {
  SHOWPARSE("fntype -> ( FN ( types ) type )");
  GCPtr<AST> fnargVec = new AST(at_fnargVec, $3.loc);
  $$ = new AST(at_fn, $2.loc, fnargVec, $5);
};
fntype: '(' tk_FN '(' types_pl_byref ')' type ')'  {
  SHOWPARSE("fntype -> ( FN ( types_pl_byref ) type )");
  $$ = new AST(at_fn, $2.loc, $4, $6);
};

type_cpair: type ',' type {
  SHOWPARSE("type_cpair -> type ',' type");
  $$ = new AST(at_typeapp, $2.loc,
	       new AST(at_ident, LToken($2.loc, "pair")),
	       $1, $3);
  $$->printVariant = 1;
};
type_cpair: type ',' type_cpair {
  SHOWPARSE("type_cpair -> type ',' type_cpair");
  $$ = new AST(at_typeapp, $2.loc,
	       new AST(at_ident, LToken($2.loc, "pair")),
	       $1, $3);
  $$->printVariant = 1;
};

type: '(' type_cpair ')' {
  SHOWPARSE("type -> type_cpair");
  $$ = $2;
};

// ARRAY TYPE [3.5.1]                 
type: '(' tk_ARRAY type intLit ')'  {
  SHOWPARSE("type -> ( ARRAY type intLit )");
  $$ = new AST(at_arrayType, $2.loc, $3, $4);
};
// VECTOR TYPE [3.5.2]               
type: '(' tk_VECTOR type ')' {
  SHOWPARSE("type -> (VECTOR type )");
  $$ = new AST(at_vectorType, $2.loc, $3);
};

// TYPE CONSTRUCTORS (typapp)
type: typapp {
  SHOWPARSE("type -> typapp");
  $$ = $1;
};

typapp: '(' useident types ')' {
  SHOWPARSE("typeapp -> ( useident types )");
  $$ = new AST(at_typeapp, $2->loc, $2);
  $$->addChildrenFrom($3);
};

// MUTABLE TYPE [3.7]              
type: '(' tk_MUTABLE type ')' {
  SHOWPARSE("type -> ( MUTABLE type )");
  $$ = new AST(at_mutableType, $2.loc, $3);
};

// BITFIELD TYPE
bitfieldtype: '(' tk_BITFIELD any_int_type intLit ')' {
  SHOWPARSE("bitfieldtype -> ( BITFIELD any_int_type intLit )");
  $$ = new AST(at_bitfield, $2.loc, $3, $4);
};
bitfieldtype: '(' tk_BITFIELD bool_type intLit ')' {
  SHOWPARSE("bitfieldtype -> ( BITFIELD bool_type intLit )");
  $$ = new AST(at_bitfield, $2.loc, $3, $4);
};

// Any-type, including bitfield type
type_pl_bf: bitfieldtype {
  SHOWPARSE("type_pl_bf -> bitfieldtype");
  $$ = $1;
};

type_pl_bf: type {
  SHOWPARSE("type_pl_bf -> type");
  $$ = $1;
};

// by-ref types are not a part of general `type' rule. 
// They are gramatiocally restricted to apprae only on 
// formal function arguments and function types. 
type_pl_byref: type {
  SHOWPARSE("type_pl_byref -> type");
  $$ = $1;
};

type_pl_byref: '(' tk_BY_REF type ')' {
  SHOWPARSE("type_pl_byref -> ( BY-REF type )");
  $$ = new AST(at_byrefType, $2.loc, $3);
};

types_pl_byref: type_pl_byref {
  SHOWPARSE("types_pl_byref -> type_pl_byref");
  $$ = new AST(at_fnargVec);
  $$->addChild($1);
}; 
types_pl_byref: types_pl_byref type_pl_byref {
  SHOWPARSE("types_pl_byref -> types_pl_byref type_pl_byref");
  $$ = $1;
  $1->addChild($2);
};


// Qualified types:

qual_type: type {
  SHOWPARSE("qual_type -> type");
  $$ = $1;  
};

qual_type: '(' tk_FORALL constraints type ')' {
 SHOWPARSE("qual_type -> ( FORALL constraints type )");
 $$ = new AST(at_qualType, $2.loc, $3, $4);
};

qual_constraint: constraint {
  SHOWPARSE("qual_typeapp -> constraint");
  GCPtr<AST> constrs = new AST(at_constraints, $1->loc);
  $$ = new AST(at_qualType, $1->loc, constrs, $1);
}; 
qual_constraint: '(' tk_FORALL constraints constraint ')' {
  SHOWPARSE("qual_typeapp -> ( FORALL constraints constraint )");
  $$ = new AST(at_qualType, $2.loc, $3, $4);
}; 

// METHOD TYPES [3.9]
/* type: '(' tk_METHOD tvapp fntype')' { */
/*   SHOWPARSE("METHOD tcapp fntype )"); */
/*   GCPtr<AST> tcreqs = new AST(at_tcreqs, $3->loc, $3); */
/*   $$ = new AST(at_methodType, $2.loc, tcreqs, $4); */
/* }; */

/* type: '(' tk_METHOD '(' tk_AND tcreqs ')' fntype')' { */
/*   SHOWPARSE("tcreq -> ( var tvlist )"); */
/*   $$ = new AST(at_methodType, $2.loc, $5, $7); */
/* }; */

// BINDING PATTERNS [5.1]
bindingpattern: ident {
  SHOWPARSE("bindingpattern -> ident");
  $$ = new AST(at_identPattern, $1->loc, $1);
};
bindingpattern: ident ':' type {
  SHOWPARSE("bindingpattern -> ident : type");
  $$ = new AST(at_identPattern, $1->loc, $1, $3);
};
bindingpattern: '(' tk_THE type ident ')' {
  SHOWPARSE("bindingpattern -> ident : type");
  $$ = new AST(at_identPattern, $1.loc, $4, $3);
};

// There are no defpattern sequences, because there is no top-level
// pattern application 
// DEFPATTERN
defpattern: defident {
  SHOWPARSE("defpattern -> defident");
  $$ = new AST(at_identPattern, $1->loc, $1);
};
defpattern: defident ':' qual_type {
  SHOWPARSE("defpattern -> defident : qual_type");
  $$ = new AST(at_identPattern, $1->loc, $1, $3);
};
defpattern: '(' tk_THE qual_type defident ')' {
  SHOWPARSE("defpattern -> (THE qual_type defident)");
  $$ = new AST(at_identPattern, $1.loc, $4, $3);
};


/* Lambda Patterns -- with an additional by-ref annotation */
lambdapatterns: lambdapattern {
  SHOWPARSE("lambdapatterns -> lambdapattern");
  $$ = new AST(at_argVec, $1->loc);
  $$->addChild($1);
};
lambdapatterns: lambdapatterns lambdapattern {
  SHOWPARSE("lambdapatterns -> lambdapatterns lambdapattern");
  $$ = $1;
  $$->addChild($2);
};

lambdapattern: ident {
  SHOWPARSE("lambdapattern -> ident");
  $$ = new AST(at_identPattern, $1->loc, $1);
};

lambdapattern: ident ':' type_pl_byref {
  SHOWPARSE("lambdapattern -> ident : type_pl_byref");
  $$ = new AST(at_identPattern, $1->loc, $1, $3);
  if($3->astType == at_byrefType)
    $1->Flags2 |= ARG_BYREF;
};

lambdapattern: '(' tk_THE type ident ')' {
  SHOWPARSE("lambdapattern -> ( the type ident ) ");
  $$ = new AST(at_identPattern, $1.loc, $4, $3);  
};

lambdapattern: '(' tk_THE '(' tk_BY_REF type ')' ident ')' {
  SHOWPARSE("lambdapattern -> ( the ( by-ref type ) ident )");
  $$ = new AST(at_identPattern, $1.loc, $7, $5);
  $5->Flags2 |= ARG_BYREF;
};

// EXPRESSIONS [7]
//
// expr   -- an expression form with an optional type qualifier
// eform  -- a naked expression form
//
// As a practical matter, every expression on the RHS of a production
// should be an expr

expr_seq: expr {
  SHOWPARSE("expr_seq -> expr");
  $$ = new AST(at_begin, $1->loc, $1);
  $$->printVariant = 1;
};
expr_seq: value_definition {
  SHOWPARSE("expr_seq -> value_definition");
  $$ = new AST(at_begin, $1->loc, $1);
  $$->printVariant = 1;
};
expr_seq: expr_seq expr {
  SHOWPARSE("expr_seq -> expr_seq expr");
  $$ = $1;
  $$->addChild($2);
};
expr_seq: expr_seq value_definition {
  SHOWPARSE("expr_seq -> expr_seq value_definition");
  $$ = $1;
  $$->addChild($2);
};
//expr_seq: value_definition expr_seq {
//  // AST define = identPattern expr constraints;
//  SHOWPARSE("expr_seq -> value_definition expr_seq");
//  GCPtr<AST> letBinding = new AST(at_letbinding, $1->loc, 
//			    $1->child(0), $1->child(1));
//  $$ = new AST(at_letrec, $1->loc, 
//	       new AST(at_letbindings, $1->loc, letBinding),
//	       $2,
//	       $1->child(2));
//};


// TYPE QUALIFIED EXPRESSIONS  [7.3]
expr: eform {
  SHOWPARSE("expr -> eform");
  $$ = $1;
};
expr: eform ':' type {
  SHOWPARSE("expr -> eform : type");
  $$ = new AST(at_tqexpr, $1->loc, $1, $3);
};
expr: the_expr {
  SHOWPARSE("expr -> the_expr");
  $$ = $1;
};

the_expr: '(' tk_THE type eform ')' {
  SHOWPARSE("the_expr -> ( THE type eform )");
  // Note: argument order swapped for historical reasons.
  $$ = new AST(at_tqexpr, $2.loc, $4, $3);

  if($4->Flags2 & AST_IS_LOCATION)
    $$->Flags2 |= AST_IS_LOCATION;  
};

// SUSPENDED EXPRESSIONS
expr: '(' tk_SUSPEND useident expr ')' {
  SHOWPARSE("expr -> ( SUSPEND useident expr )");
  $$ = new AST(at_suspend, $2.loc, $3, $4);
};

// LITERALS  [7.1]
eform: literal {
  SHOWPARSE("eform -> Literal");
  $$ = $1;
};

// UNIT EXPRESSIONS   [7.4.1]
eform: '(' ')' {
  SHOWPARSE("eform -> ()");
  $$ = new AST(at_unit, $1.loc);
};

// Expressions that involve locations:

// IDENTIFIERS [7.2]
/* This would actually have been 
eform: useident {
  SHOWPARSE("eform -> useident");
  $$ = $1;
};
but for the ambiguity with record (field) selection. 
So, the burden is now passed to further stages */

eform: ident {
  SHOWPARSE("eform -> ident");
  $$ = $1;
  $$->Flags2 |= AST_IS_LOCATION;
};

// MEMBER [7.7]
// In principle, we would like to accept expr.ident here, but that
// creates a parse conflict with expr:type, because the sequence
//
//    expr : type . ident
//
// can turn into:
//
//    expr : Id . Id . ident
//           ^^^^^^^
//             type
//
// which creates a shift-reduce conflict. It's all fine as long as the
// expr is fully bracket, so there is no problem accepting:
//
//    (the type expr).ident
//
// We have adopted the solution of declaring that the ":" convenience
// syntax cannot be used inside a member selection. This is not likely
// to be burdensome. If the expr on the LHS is a locally computed
// result, the type will be inferred through chaining from the
// constructor expression. The only case where this is likely to be
// annoying is for arguments of structure type, but we do not infer
// structure types from field names in any case, so those will
// probably require argument declarations in any case.
eform: eform '.' ident {
  SHOWPARSE("eform -> eform . ident");
  $$ = new AST(at_select, $1->loc, $1, $3);
  $$->Flags2 |= AST_IS_LOCATION;
};

eform: the_expr '.' ident {
  SHOWPARSE("eform -> the_expr . ident");
  $$ = new AST(at_select, $1->loc, $1, $3);
  $$->Flags2 |= AST_IS_LOCATION;
};

eform: '(' tk_MEMBER expr ident ')' {
  SHOWPARSE("eform -> ( member expr ident )");
  $$ = new AST(at_select, $2.loc, $3, $4);
  $$->Flags2 |= AST_IS_LOCATION;
};

// NTH-REF [7.8.2]          
eform: expr '[' expr ']' {
  SHOWPARSE("eform -> expr [ expr ]");
  $$ = new AST(at_vector_nth, $1->loc, $1, $3);
};

eform: '(' tk_ARRAY_NTH expr expr ')' {
  SHOWPARSE("eform -> ( ARRAY-NTH expr expr )");
  $$ = new AST(at_array_nth, $2.loc, $3, $4);
  $$->Flags2 |= AST_IS_LOCATION;
};
eform: '(' tk_VECTOR_NTH expr expr ')' {
  SHOWPARSE("eform -> ( VECTOR-NTH expr expr )");
  $$ = new AST(at_vector_nth, $2.loc, $3, $4);
  $$->Flags2 |= AST_IS_LOCATION;
};

// DEREF [7.13.2]                
eform: expr '^' {
  SHOWPARSE("eform -> expr ^");
  $$ = new AST(at_deref, $1->loc, $1); 
  $$->printVariant = 1;
  $$->Flags2 |= AST_IS_LOCATION;
};
eform: '(' tk_DEREF expr ')' {
  SHOWPARSE("eform -> ( DEREF expr )");
  $$ = new AST(at_deref, $2.loc, $3);
  $$->Flags2 |= AST_IS_LOCATION;
};

// INNER-REF
// In the case of structures, the second "expression"
// must be a label. This cannot be checked until 
// type-checking phase.
/* eform: '(' tk_INNER_REF expr expr ')' { */
/*   SHOWPARSE("eform -> ( INNER_REF expr expr)"); */
/*   $$ = new AST(at_inner_ref, $2.loc, $3, $4); */
/* }; */

// End of locations

// PAIR EXPRESSIONS
eform_cpair: expr ',' expr {
  SHOWPARSE("eform_cpair -> expr ',' expr");
  $$ = new AST(at_apply, $2.loc,
	       new AST(at_ident, LToken($2.loc, "pair")),
	       $1, $3);
  $$->printVariant = 1;
};
eform_cpair: expr ',' eform_cpair {
  SHOWPARSE("eform_cpair -> expr ',' eform_cpair");
  $$ = new AST(at_apply, $2.loc,
	       new AST(at_ident, LToken($2.loc, "pair")),
	       $1, $3);
  $$->printVariant = 1;
};
eform: '(' eform_cpair ')' {
  SHOWPARSE("eform -> ( eform_cpair )");
  $$ = $2;
};

eform: '(' tk_MAKE_VECTORL expr expr ')' {
  SHOWPARSE("eform -> ( MAKE-VECTOR expr expr )");
  $$ = new AST(at_makevectorL, $2.loc, $3, $4);
};

// VECTORS [7.4.3]
eform: '(' tk_VECTOR expr_seq ')' {
  SHOWPARSE("eform -> (VECTOR expr_seq)");
  $$ = $3;
  $$->astType = at_vector;
  $$->loc = $2.loc;
};

// ARRAYS [7.4.3]
eform: '(' tk_ARRAY expr_seq ')' {
  SHOWPARSE("eform -> (ARRAY expr_seq)");
  $$ = $3;
  $$->astType = at_array;
  $$->loc = $2.loc;
};

// BEGIN [7.5] 
eform: '(' tk_BEGIN expr_seq ')' {
  SHOWPARSE("eform -> ( BEGIN expr_seq )");
  $$ = $3;
  $$->loc = $2.loc;
  $$->astType = at_begin;
};

// ARRAY-LENGTH [7.8.1]
eform: '(' tk_ARRAY_LENGTH expr ')' {
  SHOWPARSE("eform -> ( ARRAY-LENGTH expr expr )");
  $$ = new AST(at_array_length, $2.loc, $3);
};
// VECTOR-LENGTH [7.8.1]
eform: '(' tk_VECTOR_LENGTH expr ')' {
  SHOWPARSE("eform -> ( VECTOR-LENGTH expr expr )");
  $$ = new AST(at_vector_length, $2.loc, $3);
};

// LAMBDA [7.9]
// handles unit argument
//eform: '(' tk_LAMBDA lambdapattern expr_seq ')'  {
//  SHOWPARSE("lambda -> ( LAMBDA lambdapattern expr_seq )");
//  $4->astType = at_ibegin;
//  $$ = new AST(at_xlambda, $2.loc, $3, $4);
//};
// convenience syntax: multiple arguments
eform: '(' tk_LAMBDA '(' ')' expr_seq ')'  {
  SHOWPARSE("lambda -> ( LAMBDA lambdapatterns expr_seq )");
  if ($5->children->size() == 1 && $5->child(0)->astType == at_begin)
    $5 = $5->child(0);
  GCPtr<AST> argVec = new AST(at_argVec, $3.loc);
  $$ = new AST(at_lambda, $2.loc, argVec, $5);
};

eform: '(' tk_LAMBDA '(' lambdapatterns ')' expr_seq ')'  {
  SHOWPARSE("lambda -> ( LAMBDA lambdapatterns expr_seq )");
  if ($6->children->size() == 1 && $6->child(0)->astType == at_begin)
    $6 = $6->child(0);
  $$ = new AST(at_lambda, $2.loc, $4, $6);
};

// APPLICATION [7.10]          
eform: '(' expr ')' { /* apply to zero args */ 
  SHOWPARSE("eform -> ( expr )");
  $$ = new AST(at_apply, $2->loc, $2);
};
eform: '(' expr expr_seq ')' { /* apply to one or more args */ 
  SHOWPARSE("eform -> ( expr expr_seq )");
  $$ = new AST(at_apply, $2->loc, $2);
  $$->addChildrenFrom($3);
};
 
// IF [7.11.1]
eform: '(' tk_IF expr expr expr ')' {
  SHOWPARSE("eform -> (IF expr expr expr )");
  $$ = new AST(at_if, $2.loc, $3, $4, $5);
};

// WHEN [7.11.2]
eform: '(' tk_WHEN expr expr_seq ')' {
  SHOWPARSE("eform -> (WHEN expr_seq )");
  $4->astType = at_begin;
  $4->printVariant = 1;
  $$ = new AST(at_when, $2.loc, $3, $4);
};

// NOT [7.11.3]
eform: '(' tk_NOT expr ')'  {
  SHOWPARSE("eform -> ( NOT expr )");
  $$ = new AST(at_not, $2.loc, $3);
};

// AND [7.11.4]                  
eform: '(' tk_AND expr_seq ')'  {
  SHOWPARSE("eform -> ( AND expr_seq )");
  $$ = $3;
  $$->loc = $2.loc;
  $$->astType = at_and;
};

// OR [7.11.5]
eform: '(' tk_OR expr_seq ')'  {
  SHOWPARSE("eform -> ( OR expr_seq )");
  $$ = $3;
  $$->loc = $2.loc;
  $$->astType = at_or;
};

// COND [7.11.6]           
eform: '(' tk_COND  condcases  otherwise ')'  {
  SHOWPARSE("eform -> (COND  ( condcases ) ) ");
  $$ = new AST(at_cond, $2.loc, $3, $4);
};

condcases: condcase {
  SHOWPARSE("condcases -> condcase");
  $$ = new AST(at_cond_legs, $1->loc, $1);
};

condcases: condcases condcase {
  SHOWPARSE("condcases -> condcases condcase");
  $$ = $1;
  $$->addChild($2);
};

condcase: '(' expr expr_seq ')'  {
  SHOWPARSE("condcase -> ( expr expr_seq )");
  $$ = new AST(at_cond_leg, $1.loc, $2, $3);
};

// SET! [7.12.1]                 
eform: '(' tk_SET expr expr ')' {
  SHOWPARSE("eform -> ( SET! expr expr )");
  $$ = new AST(at_setbang, $2.loc, $3, $4);
};

// READ-ONLY [7.12.2]
//eform: '(' tk_READ_ONLY expr ')' {
//  SHOWPARSE("eform -> ( MUTABLE expr )");
//  $$ = new AST(at_read_only, $2.loc, $3);
//};

// DUP
eform: '(' tk_DUP expr ')' {
  SHOWPARSE("eform -> ( DUP expr )");
  $$ = new AST(at_dup, $2.loc, $3);
};

// SWITCH
eform: '(' tk_SWITCH ident expr sw_legs ow ')' {
  SHOWPARSE("eform -> ( SWITCH ident expr sw_legs ow)");
  $$ = new AST(at_switch, $2.loc, $3, $4, $5, $6);
  for(size_t c =0; c < $5->children->size(); c++) {
    GCPtr<AST> sw_leg = $5->child(c);
    sw_leg->children->insert(0, $3->getDCopy());
  }
};

sw_legs: sw_leg {
  SHOWPARSE("sw_legs -> sw_leg");
  $$ = new AST(at_sw_legs, $1->loc, $1);
};

sw_legs: sw_legs sw_leg {
  SHOWPARSE("sw_legs -> sw_legs sw_leg");
  $$ = $1;
  $$->addChild($2);
};

sw_leg: '(' switch_match expr_seq ')'  {
  SHOWPARSE("sw_leg -> ( switch_match expr_seq )");
  $$ = new AST(at_sw_leg, $1.loc, $3, $2);
};

sw_leg: '(' '(' switch_matches ')' expr_seq ')'  {
  SHOWPARSE("sw_leg -> ( ( switch_matches ) expr_seq )");
  $$ = new AST(at_sw_leg, $1.loc, $5);
  $$->addChildrenFrom($3);
};

switch_matches: switch_match {
  SHOWPARSE("switch_matches -> switch_match");
  $$ = new AST(at_Null, $1->loc, $1);
};

switch_matches: switch_matches switch_match {
  SHOWPARSE("switch_matches -> switch_matches switch_match");
  $$ = $1;
  $$->addChild($2);
};

/* Constructors may be expressed as:
   i) cons
   ii) list.cons
   iii) bitc.list.cons

   If we find the double-dotted version, we are sure that we have
   found the useident.ctr version, otherwise, this is ambiguous, and
   leave the burden on the resolver to find out */
switch_match: ident {
  SHOWPARSE("switch_match -> useident");
  $$ = $1;
};

switch_match: ident '.' ident {
  SHOWPARSE("switch_match -> useident");  
  $$ = new AST(at_select, $1->loc, $1, $3);
};

switch_match: ident '.' ident '.' ident {
  SHOWPARSE("switch_match -> useident '.' useident");
  GCPtr<AST> usesel = new AST(at_usesel, $1->loc, $1, $3);  
  $$ = new AST(at_select, $1->loc, usesel, $5);
};

ow: otherwise {
  SHOWPARSE("ow -> otherwise");
  $$ = $1;
};

ow: { //empty
  SHOWPARSE("ow -> Null");
  $$ = new AST(at_Null);
};

otherwise: '(' tk_OTHERWISE expr_seq')' {
  SHOWPARSE("otherwise -> ( OTHERWISE expr_seq)");
  $$ = new AST(at_otherwise, $2.loc, $3);
};

/* // TYPECASE  [11]            
eform: '(' tk_TYPECASE '(' typecase_legs ')' ')'  {
  SHOWPARSE("eform -> ( typecase ( typecase_legs ) )");
  $$ = $4;
  $$->loc = $2.loc;
};
typecase_legs: typecase_leg {
  SHOWPARSE("typecase_legs -> typecase_leg");
  $$ = new AST(at_typecase, $1->loc);
  $$->addChild($1);
};
typecase_legs: typecase_legs typecase_leg {
  SHOWPARSE("typecase_legs -> typecase_legs typecase_leg");
  $$ = $1;
  $$->addChild($2);
};
typecase_leg: '(' bindingpattern expr ')'  {
  SHOWPARSE("typecase_leg -> ( Bindingpattern expr )");
  $$ = new AST(at_typecase_leg, $1.loc, $2, $3);
  }; */

// TRY/CATCH [7.15.1]
eform: '(' tk_TRY expr '(' tk_CATCH  ident sw_legs ow ')' ')'  {
  SHOWPARSE("eform -> ( TRY expr ( CATCH ( ident sw_legs ) ) )");
  $$ = new AST(at_try, $2.loc, $3, $6, $7, $8);
  for(size_t c =0; c < $7->children->size(); c++) {
    GCPtr<AST> sw_leg = $7->child(c);
    sw_leg->children->insert(0, $6->getDCopy());
  }
};

// THROW  [7.15.2]               
eform: '(' tk_THROW expr ')' {
  SHOWPARSE("eform -> ( THROW expr )");
  $$ = new AST(at_throw, $2.loc, $3);
};

// let / letrec forms

eform: let_eform {
  SHOWPARSE("eform -> let_eform");
  $$ = $1;
};

// LET [7.16.1]                  
let_eform: '(' tk_LET '(' letbindings ')' expr_seq ')' {
  SHOWPARSE("eform -> (LET (letbindings) expr_seq)");
  $6->astType = at_begin;
  $6->printVariant = 1;
  $$ = new AST(at_let, $2.loc, $4, $6);
  $$->addChild(new AST(at_constraints));
};
letbindings: letbinding {
  SHOWPARSE("letbindings -> letbinding");
  $$ = new AST(at_letbindings, $1->loc, $1);
};
letbindings: letbindings letbinding {
  SHOWPARSE("letbindings -> letbindings letbinding");
  $$ = $1;
  $$->addChild($2);
};
letbinding: '(' bindingpattern expr ')' {
  SHOWPARSE("letbinding -> ( bindingpattern expr )");
  $$ = new AST(at_letbinding, $2->loc, $2, $3);
};

// LETREC [7.16.2]               
let_eform: '(' tk_LETREC '(' letbindings ')' expr_seq ')' {
  SHOWPARSE("eform -> (LETREC (letbindings) expr_seq)");
  $6->astType = at_begin;
  $6->printVariant = 1;
  GCPtr<AST> lbs = $4;
  for(size_t c=0; c < lbs->children->size(); c++)
    lbs->child(c)->Flags |= LB_REC_BIND;
  
  $$ = new AST(at_letrec, $2.loc, $4, $6);
  $$->addChild(new AST(at_constraints));
};

eform: '(' tk_DO '(' dobindings ')' dotest expr_seq ')' {
  SHOWPARSE("eform -> (DO (dobindings) dotest expr_seq)");

#if 1
  $$ = new AST(at_do, $2.loc, $4, $6, $7);
#else
  GCPtr<AST> doBindings = $4;

  GCPtr<AST> theIdent = AST::genIdent("do");
  theIdent->Flags |= ID_IS_DO;

  // The body proper of the DO is going to turn into a BEGIN
  // consisting of the provided DO body and ending with a recursive
  // call.

  // Formulate the recursive call
  GCPtr<AST> recCall = new AST(at_apply, $4->loc);
  recCall->addChild(theIdent->Use());

  for (size_t b = 0; b < doBindings->children->size(); b++) {
    GCPtr<AST> binding = doBindings->child(b);
    recCall->addChild(binding->child(2));
  }

  // Make up the begin:
  GCPtr<AST> doBody = new AST(at_begin, $7->loc);
  doBody->addChild($7);		// add the existing body
  doBody->addChild(recCall);	// add the recursive call

  // The lambda args are the left-most elements of the DO bindings
  GCPtr<AST> lamArgs = new AST(at_argVec, $4->loc);
  for (size_t b = 0; b < doBindings->children->size(); b++) {
    GCPtr<AST> binding = doBindings->child(b);
    lamArgs->addChild(binding->child(0));
  }

  // The lambda body is going to be an IF testing termination and
  // conditionally performing the initial call.
  GCPtr<AST> lamBody = new AST(at_if, $6->loc);
  {
    lamBody->addChildrenFrom($6); /* adds IF-TEST and THEN expr */
    lamBody->addChild(doBody);	/* adds ELSE expr */
  }

  GCPtr<AST> theLambda = new AST(at_lambda, $7->loc, lamArgs, lamBody);
  
  // The letrec body proper consists entirely of an APPLY form that
  // makes the initial lambda call:
  GCPtr<AST> letBody = new AST(at_apply, $1.loc, theIdent->Use());
  for (size_t b = 0; b < doBindings->children->size(); b++) {
    GCPtr<AST> binding = doBindings->child(b);
    letBody->addChild(binding->child(1));
  }

  GCPtr<AST> theBinding = new AST(at_letbinding, $1.loc);
  theBinding->Flags |= LB_REC_BIND;
  
  theBinding->addChild(new AST(at_identPattern, $1.loc, theIdent));
  theBinding->addChild(theLambda);

  GCPtr<AST> letBindings = new AST(at_letbindings, $1.loc, theBinding);

  GCPtr<AST> letRec = new AST(at_letrec, $1.loc, letBindings, letBody, 
			new AST(at_constraints));
  letRec->printVariant = 1;

  $$ = letRec;
#endif
};


dobindings: {
  SHOWPARSE("dobindings -> <empty>");
  $$ = new AST(at_dobindings);
};
dobindings: ne_dobindings {
  SHOWPARSE("dobindings -> ne_dobindings");
  $$ = $1;
};
ne_dobindings: dobinding {
  SHOWPARSE("ne_dobindings -> dobinding");
  $$ = new AST(at_dobindings, $1->loc, $1);
};
ne_dobindings: ne_dobindings dobinding {
  SHOWPARSE("ne_dobindings -> ne_dobindings dobinding");
  $$ = $1;
  $$->addChild($2);
};
dobinding: '(' bindingpattern expr expr ')' {
  SHOWPARSE("dobinding -> ( bindingpattern expr )");
  $$ = new AST(at_dobinding, $2->loc, $2, $3, $4);
};
dotest: '(' expr expr ')' {
  SHOWPARSE("dobinding -> ( expr expr )");
  $$ = new AST(at_dotest, $2->loc, $2, $3);  
};

/* Literals and Variables */
// INTEGER LITERALS [2.4.1]
literal: boolLit {
  SHOWPARSE("literal -> boolLit");
  $$ = $1;
};
literal: intLit {
  SHOWPARSE("literal -> intLit");
  $$ = $1;
};
// FLOATING POINT LITERALS [2.4.2]
literal: floatLit {
  SHOWPARSE("literal -> floatLit");
  $$ = $1;
};
// CHARACTER LITERALS [2.4.3]
literal: charlit {
  SHOWPARSE("literal -> Charlit");
  $$ = $1;
};
// STRING LITERALS [2.4.4]
literal: strLit {
  SHOWPARSE("literal -> strLit");
  $$ = $1;
};

// External identifiers are not subject to reserved word restrictions...
exident: tk_Ident {
  SHOWPARSE("exident -> <Ident " + $1.str + ">");
  $$ = new AST(at_ident, $1);
};

exident: tk_Reserved {
  SHOWPARSE("exident -> <Reserved " + $1.str + ">");
  $$ = new AST(at_ident, $1);
};

// IDENTIFIERS [2.2]
ident: tk_Ident {
  SHOWPARSE("ident -> <Ident " + $1.str + ">");
  $$ = new AST(at_ident, $1);
};

ident: tk_Reserved {
  SHOWPARSE("ident -> <RESERVED=" + $1.str + ">");
  cerr << $1.loc.asString() << ": The token \"" << $1.str 
       << "\" is reserved for future use.\n";
  lexer->num_errors++;
  $$ = new AST(at_ident, $1);
};

useident: ident {
  SHOWPARSE("useident -> ident");
  $$ = $1;
};

useident: ident '.' ident { 
  SHOWPARSE("useident -> ident . ident");
  $$ = new AST(at_usesel, $1->loc, $1, $3);
};

defident: useident {
  SHOWPARSE("defident -> useident");
  $1->Flags |= (ID_IS_GLOBAL);
  $$ = $1;
};

// TYPE VARIABLES [3.3]
typevar: tk_TypeVar {
  SHOWPARSE("typevar -> <TypeVar=" + $1.str + ">");
  $$ = new AST(at_ident, $1);
  $$->Flags |= ID_IS_TVAR;
}; 

// /* Literal Value Representations */

boolLit: tk_TRUE {
  SHOWPARSE("boolLit -> <Bool=" + $1.str +">");
  $$ = AST::makeBoolLit($1);
};
boolLit: tk_FALSE {
  SHOWPARSE("boolLit -> <Bool=" + $1.str +">");
  $$ = AST::makeBoolLit($1);
};

charlit: tk_Char {
  SHOWPARSE("charlit -> <Char=" + $1.str +">");
  $$ = AST::makeCharLit($1);
};

intLit: tk_Int {
  SHOWPARSE("intLit -> <Int=" + $1.str +">");
  $$ = AST::makeIntLit($1);
};

floatLit: tk_Float {
  SHOWPARSE("floatLit -> <Float=" + $1.str +">");
  $$ = AST::makeFloatLit($1);
};

strLit: tk_String {
  SHOWPARSE("strLit -> <String=" + $1.str +">");
  $$ = AST::makeStringLit($1);
};

%%
