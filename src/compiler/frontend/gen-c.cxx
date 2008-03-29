/**************************************************************************
 *
 * Copyright (C) 2006, Johns Hopkins University.
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
 *
 **************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include "Version.hxx"
#include "UocInfo.hxx"
#include "AST.hxx"
#include "Environment.hxx"
#include "Symtab.hxx"
#include "inter-pass.hxx"
#include <errno.h>
#include "backend.hxx"
#include "Options.hxx"
#include "INOstream.hxx"
#include "Clconv.hxx"
#include "gen-c.hxx"
#include <libsherpa/utf8.hxx>
#include <cctype>

using namespace sherpa;
using namespace std;

const char *TY_PFX   = "ty_";
const char *CTOR_PFX = "ct_";
const char *CVAL_PFX = "val_";
const char *TAG_PFX  = "tag_";
const char *ENUM_PFX = "en_";
const char *ARG_PFX  = "arg_";
const char *RET_PFX  = "ret_";
const char *ENV_PFX  = "env_"; // Environment argument
const char *XFN_PFX  = "xfn_"; // transition function for closures
const char *MFN_PFX  = "mfn_"; // Real Function (label) in the case of
			       // mutable top-level functions.
const char *WFN_PFX  = "wfn_"; // If a immutable function-pointer is
			       // not a lambda, we will emit a
			       // wrapper-label function with the true
			       // name. This prifix is for the inner
			       // function (that actually does all the
			       // word).
#if 0
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// As of now, ALL identifiers that the use has input will be mangled
// to have a leading _ so I don't need to check for keywords, unless
// the implementer is careless enough to generate names that collide
// with the C keywords. If this *ever* happens, re-enable this check. 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Very important to keep this sorted (excluding the guard entry),
// since it is searched with bsearch.
static char *kwd_blacklist[] = {
  "asm",			// c++
  "auto",
  "bitc_bool_t",	        // bitc runtime
  "bitc_char_t",	        // bitc runtime
  "bitc_double_t",		// bitc runtime
  "bitc_float_t",		// bitc runtime
  "bitc_int16_t",		// bitc runtime
  "bitc_int32_t",		// bitc runtime
  "bitc_int64_t",		// bitc runtime
  "bitc_int8_t",		// bitc runtime
  "bitc_quad_t",		// bitc runtime
  "bitc_string",		// bitc runtime
  "bitc_uns16_t",		// bitc runtime
  "bitc_uns32_t",		// bitc runtime
  "bitc_uns64_t",		// bitc runtime
  "bitc_uns8_t",		// bitc runtime
  "bitc_vector",		// bitc runtime
  "bitc_word_t",		// bitc runtime
  "bool",			// c++, later ANSI C
  "break",
  "case",
  "catch",			// c++
  "cdecl",			// c++
  "char",
  "class",			// c++
  "const",
  "const_cast",			// c++
  "continue",
  "default",
  "defined",			// some extensions
  "delete",			// c++
  "do",
  "double",
  "dynamic_cast",		// c++
  "else",
  "entry",			// some extensions
  "enum",
  "explicit",			// c++
  "extern",
  "false",			// c++, C99
  "far",			// MS-DOS
  "float",
  "for",
  "fortran",			// VMS extension
  "friend",			// c++
  "generic",			// ???
  "globaldef",			// ???
  "globalref",			// ???
  "globalvalue",		// ???
  "goto",
  "huge",			// MS-DOS
  "if",
  "inline",
  "int",
  "int16",			// BSD, POSIX
  "int32",			// BSD, POSIX
  "int64",			// BSD, POSIX
  "int8",			// BSD, POSIX
  "interface",			// c++ (MS)
  "long",
  "mutable",			// c++
  "namespace",
  "near",			// MS-DOS
  "new",			// C++
  "operator",			// C++
  "pascal",			// VMS extension
  "pragma",			// various
  "private",			// c++
  "protected",			// c++
  "public",			// c++
  "quad",			// shap
  "readonly",			// various
  "register",
  "reinterpret_cast",		// c++
  "return",
  "short",
  "signed",
  "sizeof",
  "static",
  "static_cast",		// c++
  "string",			// shap
  "struct",
  "super",			// c++ (MS)
  "switch",
  "template",			// c++
  "this",			// c++
  "throw",			// c++
  "true",			// c++, C99
  "try",			// c++
  "typedef",
  "typeid",			// c++
  "typename",			// c++
  "uint16",			// BSD, POSIX
  "uint32",			// BSD, POSIX
  "uint64",			// BSD, POSIX
  "uint8",			// BSD, POSIX
  "union",
  "unsigned",
  "use",
  "using",
  "virtual",			// c++
  "void",
  "volatile",
  "while",
  "word",			// just begging for trouble


  0				// GUARD ENTRY
};

enum { nkwd = (sizeof(kwd_blacklist)/sizeof(kwd_blacklist[0])) - 1 };

bool 
is_kwd(const std::string& s)
{
  const char *cs = s.c_str();

  // &OK
  if (bsearch(&cs, kwd_blacklist, nkwd, sizeof(kwd_blacklist[0]), 
	      ((int (*)(const void*, const void*)) strcmp)))
    return true;
  return false;
}
#endif

// Very important that none of these ever use _UC, because that is our
// prefix for a unicode character encoding.
const struct MangleMap {
  char c;
  const char *s;
} mangleMap[] = {
  { '#',  "_SH" },
  { '!',  "_EX" },
  { '$',  "_DL" },
  { '%',  "_PC" },
  { '*',  "_ST" },
  { '+',  "_PL" },
  { '\\', "_RS" },
  { '-',  "_HY" },
  { '/',  "_FS" },
  { '<',  "_LT" },
  { '>',  "_GT" },
  { '=',  "_EQ" },
  { '?',  "_QU" },
  { '@',  "_AT" },
  { '~',  "_TL" },
  { '_',  "_" },
  { '.',  "_DT"},
  { '\'', "_QT" }
};
enum { nSpecial = sizeof(mangleMap) / sizeof(MangleMap) };

const char *
punctMangle(uint32_t codePoint)
{
  for (size_t sp = 0; sp < nSpecial; sp++) {
    if (mangleMap[sp].c == (char) codePoint) {
      return mangleMap[sp].s;
    }
  }

  return 0;
}

std::string
CMangle(std::string idName)
{
  std::stringstream ss;      
  const char *s = idName.c_str();
  
  while (*s) {
    const char *snext;
    uint32_t codePoint = sherpa::utf8_decode(s, &snext);
    const char *pm;
    
    if (isalnum(codePoint)) {
      ss << *s;
    }
    else if ((pm=punctMangle(codePoint))) {
      ss << pm;
    }
    else {
      ss << "_UC"
	 << hex
	 << codePoint
	 << dec
	 << "_";
    }
    s = snext;
  }
  
  return ss.str();
}

#define CMGL_ID_FLD 0x1u
std::string
CMangle(GCPtr<AST> ast, unsigned long flags = 0)
{  
  assert(ast->astType == at_ident);
  
  GCPtr<AST> id = ast;

  if(id->symbolDef)
    id = id->symbolDef;
  
  if(id->isDecl && (id->defn))
    id = id->defn;

  if (id->externalName.size())
    return id->externalName;

  std::stringstream ss;      
  if((id->Flags & ID_IS_GLOBAL) ||
     (id->Flags & ID_IS_GENSYM) ||
     (flags & CMGL_ID_FLD))
    ss << id->s;
  else 
    ss << id->s << "#" << id->ID;
  string idName = ss.str();
  
  return CMangle(ss.str());
}

/* Forward Declaration */
bool
toc(std::ostream& errStream, GCPtr<UocInfo> uoc,
    GCPtr<AST> ast, INOstream &out, const string &IDname,
    GCPtr<AST> parent, const size_t chno, unsigned long flags);

#define CTYP_EMIT_BF      0x01u	// bitfield
#define CTYP_BYREF        0x02u // Used to declare by-ref 
                                // arguments in decl() routine.
 
static string
toCtype(GCPtr<Type> typ, string IDname="", unsigned long flags=0, 
	uint64_t arrsz = 0) 
{
  GCPtr<Type> t = typ->getBareType();
  stringstream out;
  
  switch(t->kind) {
  case ty_tvar:
    {
      // This case is not wrong; for example, consider
      // (define (main) (throw Xn) (the int32 10))
      // The intermediate result that holds (throw Xn)
      // has no concrete type.
      out << "bitc_tvar_t";
      break;
    }

  case ty_letGather:
    assert(false);
    break;
    
  case ty_mbTop:
  case ty_mbFull:
    out << toCtype(t->Core(), IDname, flags, arrsz);
    break;

  case ty_mutable:
    out << toCtype(t->Base(), IDname, flags, arrsz);
    break;
    
  case ty_dummy:
    // Dummy types fixed to unit
  case ty_unit:
    out << "bitc_unit_t";
    break;
    
  case ty_bool:
    out << "bitc_bool_t";
    break;
  case ty_char:
    out << "bitc_char_t";
    break;
  case ty_string:
    out << "bitc_string_t *";
    break;
  case ty_int8:
    out << "bitc_int8_t";
    break;
  case ty_int16:
    out << "bitc_int16_t";
    break;
  case ty_int32:
    out << "bitc_int32_t";
    break;
  case ty_int64:
    out << "bitc_int64_t";
    break;
  case ty_uint8:
    out << "bitc_uns8_t";
    break;
  case ty_uint16:
    out << "bitc_uns16_t";
    break;
  case ty_uint32:
    out << "bitc_uns32_t";
    break;
  case ty_uint64:
    out << "bitc_uns64_t";
    break;
  case ty_word:
    out << "bitc_word_t";
    break;
  case ty_float:
    out << "bitc_float_t";
    break;
  case ty_double:
    out << "bitc_double_t";
    break;
  case ty_quad:
    out << "bitc_quad_t";
    break;

#ifdef KEEP_BF
  case ty_bitfield:
    if(IDname.size() && (flags & CTYP_EMIT_BF))
      out << toCtype(t->CompType(0), IDname, flags, arrsz)      
	  << " " << IDname
	  << ":" << t->Isize;
    else
      out << toCtype(t->CompType(0), IDname, flags, arrsz);
#endif
    
    break;
    
  case ty_fn:
    {
      out << CMangle(t->mangledString(true));
      break;
    }

  case ty_fnarg:
    {
      for(size_t i=0; i<t->components->size(); i++) {
	if (i > 0) 
	  out << ", ";
	out << toCtype(t->CompType(i), IDname, flags, arrsz);
      }
      break;
    }

  case ty_tyfn:
    {
      assert(false);
      break;
    }

  case ty_structv:
  case ty_unionv:
  case ty_reprv:
    {
      out << TY_PFX << CMangle(t->defAst);
      break;
    }

  case ty_structr:
  case ty_unionr:
  case ty_reprr:
    {
      out << TY_PFX << CMangle(t->defAst) << "*";
      break;      
    }

  case ty_uvalv:
  case ty_uconv:
    {
      out << TY_PFX <<CMangle(t->myContainer);
      break;
    }

  case ty_uvalr:
  case ty_uconr:
    {
      out << TY_PFX << CMangle(t->myContainer) << "*";
      break;
    }

  case ty_array:
    {      
      out << CMangle(t->mangledString(true));
      break;
    }
    
  case ty_vector:
    {
      out << CMangle(t->mangledString(true)) << "*";
      break;
    }

  case ty_byref:
  case ty_ref:
    {
      out << toCtype(t->Base(), IDname, flags, arrsz)
	  << "*";
      break;
    }

  case ty_exn:
    {
      if(t->defAst)
	out << TY_PFX << CMangle(t->defAst) << "*";
      else
	out << "bitc_exception_t *";
      break;
    }

  case ty_typeclass:
  case ty_subtype:
  case ty_pcst:
  case ty_kvar:
  case ty_kfix:
    assert(false);
    break;
  }
  return out.str();
}

static inline std::string
decl(GCPtr<Type> typ, string idName, unsigned flags=0,
     size_t field_bits=0)
{
  stringstream ss;
  ss << toCtype(typ) << " ";
  
  if (flags & CTYP_BYREF)
    ss << "*";
  
  ss << idName;
  
  if ((flags & CTYP_EMIT_BF) && field_bits)
    ss << ":" << field_bits;
  
  return ss.str();
}

static inline std::string
decl(GCPtr<AST> id, string idPrefix="", unsigned flags=0, 
     unsigned long cmFlags=0)
{
  assert(id->astType == at_ident);
  stringstream ss;
  
  ss << toCtype(id->symType) << " ";  
  
  if (flags & CTYP_BYREF)
    ss << "*";
  
  ss << idPrefix << CMangle(id, cmFlags);
  
  if((flags & CTYP_EMIT_BF) && (id->field_bits > 0))
    ss << ":" << id->field_bits;
  
  return ss.str();
}

static inline void
declare(INOstream &out, GCPtr<AST> id, string prefix="", 
	unsigned flags=0)
{  
  out << decl(id, prefix, flags) << ";" << endl;
}


static void
emit_ct_args(INOstream &out, GCPtr<AST> fields, size_t start=0)
{  
  out << "(";

  bool emitted1=false;
  for(size_t c = start; c < fields->children->size(); c++) {
    GCPtr<AST> field = fields->child(c);
    
    if(field->astType == at_fill || 
       field->astType == at_reserved || 
       (field->Flags2 & FLD_IS_DISCM))
      continue;

    if(emitted1)
      out << ", ";
    
    emitted1=true;

    out << decl(field->child(1)->symType, 
		"_" + CMangle(field->child(0), CMGL_ID_FLD));
  }
  out << ")" << endl;  
}

static void
emit_ct_inits(INOstream &out, GCPtr<AST> fields, 
	      string pre="", size_t start=0)
{
  for(size_t i = start; i < fields->children->size(); i++) {
    GCPtr<AST> field = fields->child(i);
    if(field->astType == at_fill)
      continue;
	  
    if(field->astType == at_reserved) {
      out << pre << "__reserved" << field->ID
	  << " = " 
	  << field->child(1)->litValue.i;
      out << ";" << endl;
      continue;
    }

    string fMang = CMangle(field->child(0), CMGL_ID_FLD);
    if(field->Flags2 & FLD_IS_DISCM) {
      out << pre << fMang << " = "
	  << field->unin_discm
	  << ";" << endl;
    }
    else {
      out << pre << fMang << " = "
	  << "_" << CMangle(field->child(0), CMGL_ID_FLD)
	  << ";" << endl;
    }
  }
}

static void
emit_fnxn_decl(INOstream &out, GCPtr<AST> ast, 
	       bool oneLine, std::string pfx="", 
	       size_t startParam=0)
{
  GCPtr<AST> id = ast->child(0)->child(0);
  GCPtr<Type> fnType = id->symType->getBareType();
  GCPtr<Type> retType = fnType->Ret()->getType();
  GCPtr<AST> lam = ast->child(1);
  GCPtr<AST> argvec = lam->child(0);
  GCPtr<Type> fnargvec = fnType->Args()->getBareType();
  assert(argvec->children->size() == fnargvec->components->size());

  out << toCtype(retType);
  if(!oneLine)
    out << endl;
  
  out << pfx << CMangle(id) << " ";
  out << "(";
  assert(startParam <= argvec->children->size());
  for(size_t i=startParam; i < argvec->children->size(); i++) {
    if(i > startParam)
      out << ", ";
    
    GCPtr<AST> pat = argvec->child(i);
    assert(pat->astType == at_identPattern);
    GCPtr<AST> arg = pat->child(0);
    unsigned long flags = ((fnargvec->CompFlags(i) &
			    COMP_BYREF)?CTYP_BYREF:0);
    
    out << decl(arg, "", flags);
  }
  out << ")";
}

static bool
emit_fnxn_label(std::ostream& errStream, 
		GCPtr<UocInfo> uoc,
		GCPtr<AST> ast,
		INOstream &out,
		GCPtr<AST> parent, 
		const size_t chno,
		unsigned long flags)
{
  bool errFree = true;
  assert(ast->astType == at_define);
  GCPtr<AST> id = ast->child(0)->child(0);
  assert(id->isFnxn());
  bool isHeader = (flags & TOC_HEADER_MODE);
  
  emit_fnxn_decl(out, ast, isHeader);
		
  if(isHeader) {
    out << ";" << endl;
    return errFree;
  }
	
  out << endl;	
  out << "{"
      << endl;	    
  out.more();
  
  GCPtr<AST> lam = ast->child(1);
  GCPtr<AST> body = lam->child(1);
  GCPtr<AST> ret=0;
  
  assert(body->astType == at_container);
  if(body->child(1)->astType == at_letStar) {
    CHKERR(errFree, toc(errStream, uoc, body, out, CMangle(id), 
			lam, 1, flags));
    out << ";" << endl; 
	  
    ret = FEXPR(body->child(1));	
  }
  else {
    ret = body->child(1); // trivial return
  }
  assert(ret);
  out <<  "return ";
  CHKERR(errFree, toc(errStream, uoc, ret, out, CMangle(id), 
		      lam, 1, flags));	
  out << ";" << endl;
  out.less();
  out << "}" << endl;

  /* Need to emit a transition function if this is a hoisted 
     function that is a part of closure conversion operation */
  if(ast->Flags2 & LAM_NEEDS_TRANS) {

    // Top level mutable function pointers are not cl-lambdas
    // There are converted into function+init in the fix4C pass.
    assert(!id->symType->isMutable());

    out << endl;
    out << "/* Transition Function */" << endl;    
    emit_fnxn_decl(out, ast, false, XFN_PFX, 1);
    out << endl;	
    out << "{"
	<< endl;	    
    out.more();
    
    GCPtr<AST> argvec = lam->child(0);    
    out << "return " << CMangle(id) << "(currentClosurePtr";
    for(size_t i=1; i < argvec->children->size(); i++) {
      GCPtr<AST> pat = argvec->child(i);
      GCPtr<AST> arg = pat->child(0);
      out << ", " << CMangle(arg);
    }
    out << ");" << endl; 
    out.less();
    out << "}" << endl <<endl;
  }

  return errFree;
}

bool 
typeIsUnmangled(GCPtr<Type> typ)
{
  typ = typ->getBareType();
  switch(typ->kind) {

  case ty_tvar:
  case ty_unit:
  case ty_dummy:
  case ty_bool:
  case ty_char:
  case ty_string:
  case ty_int8:
  case ty_int16:
  case ty_int32:
  case ty_int64:
  case ty_uint8:
  case ty_uint16:
  case ty_uint32:
  case ty_uint64:
  case ty_word:
  case ty_float:
  case ty_double:
  case ty_quad:
  case ty_exn:
#ifdef KEEP_BF
  case ty_bitfield:
#endif    
      return true;
    
  case ty_ref:
    return typeIsUnmangled(typ->Base());

  default:
    return false;
  }
}

bool
needsBackslashEscape(uint32_t c)
{
  return (c == '"' || c == '\'' || c == '\\');
}

bool
asciiPrintableCharacter(uint32_t c)
{
  /* ASCII printable glyphs are in the range [0x20,0x7e], but a few 
     of these require special escaping. */
  return (c >= 0x20 && c < 0x7f);
}


//WARNING: **REQUIRES** answer and errorFree.
#define TOC(errStream, uoc, ast, out, IDname,			\
	    parent, chno,  flags)				\
  do {								\
    answer = toc((errStream), (uoc), (ast), (out), (IDname), 	\
		 (parent), (chno), (flags));			\
    if(answer == false)						\
      errorFree = false;					\
  }while(0)

bool
toc(std::ostream& errStream, 
    GCPtr<UocInfo> uoc,
    GCPtr<AST> ast, 
    INOstream &out,
    const string &IDname,
    GCPtr<AST> parent, 
    const size_t chno,
    unsigned long flags)
{
  bool errorFree = true, answer = false;
  // GCPtr<AST> res = NULL;
  
  //cout << "---- " << ast->astTypeName() << " flags = " << flags << endl;
  
  switch(ast->astType) {

  case at_Null:
  case at_refCat:
  case at_valCat:
  case at_opaqueCat:
  case agt_category:
  case at_AnyGroup:
  case agt_literal:
  case agt_tvar:
  case agt_var:
  case agt_definition:
  case agt_type:
  case agt_expr:
  case agt_expr_or_define:
  case agt_eform:
  case agt_type_definition:
  case agt_value_definition:
  case agt_CompilationUnit:
  case agt_tc_definition:
  case agt_if_definition:
  case agt_ow:
  case agt_qtype:
  case agt_fielditem:
  case at_localFrame:
  case at_frameBindings:
  case at_use_case:
  case at_ifident:
  case agt_ucon:

  case at_declares:
  case at_declare:

  case at_tcdecls:
  case at_tyfn:
  case at_tcapp:
  case at_method_decls:
  case at_method_decl:
  case at_usesel:

  case at_refType:
  case at_byrefType:
  case at_exceptionType:
  case at_dummyType:
  case at_valType:
  case at_fn:
  case at_fnargVec:
  case at_primaryType:
  case at_arrayType:
  case at_vectorType:
  case at_mutableType:
  case at_typeapp:
  case at_bitfield:
  case at_qualType:
  case at_constraints:
    
  case at_deftypeclass:
  case at_definstance:
  case at_methods:
  case at_use:
  case at_import:
  case at_provide:
  case at_from:
  case at_ifsel:
  case at_tvlist:

  case at_lambda:
  case at_argVec:
  case at_and:
  case at_or:
  case at_dobindings:
  case at_dobinding:
  case at_dotest:

  case at_docString:

  case at_cond:
  case at_cond_legs:
  case at_cond_leg:
  case at_letGather:
    {
      errStream << ast->loc << "Internal Compiler Error. " 
		<< "Function toc, unexpected astType: " 
		<< ast->astTypeName()
		<< endl;
       
      errorFree = false;
      break;
    }

  case at_allocREF:
    {      
      out << "GC_ALLOC_ATOMIC(sizeof("
	  << toCtype(ast->child(0)->symType)
	  << "))";
      break;
    }
    
  case at_copyREF:
    {
      out << "*";
      TOC(errStream, uoc, ast->child(0), out, IDname, ast, 0, flags);     
      out << " = ";
      out << "*";
      TOC(errStream, uoc, ast->child(1), out, IDname, ast, 1, flags);     
      out << ";" << endl;	  
      break;
    }

  case at_mkClosure:
    {
      GCPtr<AST> fn = ast->child(1);
      GCPtr<AST> env = ast->child(0);
      assert(IDname.size());
      assert(fn->astType == at_ident);
      
      out << IDname << " = " 
	  << "(" << toCtype(ast->symType) << ")"
	  << "bitc_emit_closure_object("
	  << XFN_PFX << CMangle(fn)
	  << ", ";
      TOC(errStream, uoc, env, out, IDname, ast, 0, flags);
      out << ");" << endl;
      break;
    }

  case at_setClosure:
    {
      /* This has also now become obselete */
      assert(false);      
      break;
    }

  case at_ident:
    {
      GCPtr<AST> id;
      if(ast->symbolDef)
	id = ast->symbolDef;
      else
	id = ast;
      
      if(id->Flags & ID_IS_CTOR) {
	GCPtr<Type> t = id->symType->getBareType(); 
	if(t->kind == ty_uvalv || t->kind == ty_uvalr ||
	   t->kind == ty_exn) {
	  GCPtr<AST> dummy = new AST(at_ucon_apply, ast->loc, ast);
	  dummy->symType = ast->symType;
	  TOC(errStream, uoc, dummy, out, IDname, 
	      NULL, 0, flags);	  
	}
	break;
      }


      if(id->Flags2 & ARG_BYREF)
	out << "(*";      
      
      out << CMangle(ast);

      if(id->Flags2 & ARG_BYREF)
	out << ")";      

      break;
    }
    
  case at_identPattern:
    {
      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);
      break;
    }

    // Should have skipped through these
  case at_start:
  case at_version:
  case at_interface:
  case at_module:
    {  
      assert(false);
      break;
    }
    
  case at_boolLiteral:
    {
      if(ast->litValue.b == true)
	out << "true";
      else
	out << "false";
      break;
    }

  case at_charLiteral:
    {
      uint32_t c = ast->litValue.c;

      if (asciiPrintableCharacter(c))
	out << (needsBackslashEscape(c) ? "'\\" : "'")
	    << (unsigned char) c << "'";
      else
	out << (unsigned long)(ast->litValue.c);
      break;
    }
    
  case at_intLiteral:
    {
      out << ast->litValue.i;
      break;
    }

  case at_floatLiteral:
    {
      //       mp_exp_t expptr;
      //       std::string s = mpf_get_str(NULL, &expptr, 10, 0, 
      // 				  ast->litValue.d);
      
      //       if(s.size())
      // 	for(size_t i=0; i < s.size(); i++) {
      // 	  if(i == (size_t)(expptr-1))
      // 	    out << ".";
      // 	  out << s[i];
      // 	}
      //       else
      // 	out << "0.0";
      
      char buf[256];
      snprintf(buf, sizeof(buf), " %f\n", ast->litValue.d);
      out << buf;
      break;      
    } 

  case at_stringLiteral:
    // This one is going to need a helper function in the runtime.
    {
      // needs to initialize the vector properly...
      const char *s = ast->litValue.s.c_str();
      const char *send = s + ast->litValue.s.size();

      out << "mkStringLiteral(\"";
      
      while (s != send) {
	const char *snext;
	char utf8[7];
	uint32_t codePoint = LitValue::DecodeStringCharacter(s, &snext);
	unsigned len = sherpa::utf8_encode(codePoint, utf8);

	if (asciiPrintableCharacter(codePoint)) {
	  if (needsBackslashEscape(codePoint)) out << '\\';
	  out << (unsigned char) codePoint;
	}
	else {
	  for (unsigned pos = 0; pos < len; pos++) {
	    unsigned char c = utf8[pos];
	    char d2 = '0' + (c >> 6);
	    char d1 = '0' + ((c >> 3) % 8);
	    char d0 = '0' + (c % 8);

	    out << '\\' << d2 << d1 << d0;
	  }
	}

	s = snext;
      }

      out << "\")";

      break;
    }

  case at_unit:
    {
      out << "0";
      break;
    }
    
  case at_defstruct:
    {
      GCPtr<AST> ident = ast->child(0);
      GCPtr<AST> fields = ast->child(4);
      out << "struct " << TY_PFX << CMangle(ident) << "{" << endl;
      out.more();
      TOC(errStream, uoc, fields, out, IDname, ast, 4, flags);      
      out.less();
      out << "};" << endl
	  << endl;
       
      //emit the constructor      
      out << "/* Constructor */" << endl;
      out << "INLINE " << toCtype(ident->symType) << endl
	  << CTOR_PFX << CMangle(ident) << " ";

      emit_ct_args(out, fields); 
      out << "{" << endl;

      out.more();
      std::string pre;
      if(ident->symType->getBareType()->kind == ty_structv) {
	out << toCtype(ident->symType)
	    << " val;";
	pre = "val.";
      }
      else {
	out << toCtype(ident->symType)
	    << " val = "
	    << "(" << toCtype(ident->symType) << ") "
	    << "GC_ALLOC(sizeof(" << TY_PFX << CMangle(ident) << "));" << endl;
	pre = "val->";
      }

      emit_ct_inits(out, fields, pre);
      out << "return val;" << endl;
      out.less();
      out << "}" << endl;
      break;
    }

  case at_fields:
    {
      for(size_t c=0; c < ast->children->size(); c++) {
	TOC(errStream, uoc, ast->child(c), out, IDname, 
	    ast, c, flags);	
      }
      break;
    }

  case at_field:
    {
      out << decl(ast->child(1)->symType, 
		  CMangle(ast->child(0), CMGL_ID_FLD),
		  CTYP_EMIT_BF, ast->field_bits) 
	  << ";" << endl;
      break;
    }

  case at_fill:
    {
      out << decl(ast->child(0)->symType, "/* fill */", 
		  CTYP_EMIT_BF, ast->field_bits) 
	  << ";" << endl;
      break;
    }

  case at_reserved:
    {
      stringstream ss;
      ss << "__reserved" << ast->ID;
      string s = ss.str();
      
      out << decl(ast->child(0)->symType, 
		  CMangle(s),
		  CTYP_EMIT_BF, ast->field_bits) 
	  << ";" << endl;
      
      break;
    }

  case at_ucon_apply:     
  case at_struct_apply:
    {
      GCPtr<AST> ctr = ast->child(0)->getCtr();
      
      if(ctr->symType->isException() && 
	 (ast->children->size() == 1)) {
      	out << "&" << CVAL_PFX << CMangle(ast->child(0));
      	break;
      }

      out << CTOR_PFX << CMangle(ctr)
	  << "(";
      for(size_t c=1; c < ast->children->size(); c++) {
	if(c > 1)
	  out << ", ";
	
	TOC(errStream, uoc, ast->child(c), out, IDname, 
	    ast, c, flags);
      }
      out << ")";

      break;
    }

  case at_fqCtr:
    {
      TOC(errStream, uoc, ast->child(1), out, IDname, 
	  ast, 1, flags);
      break;
    }

  case at_sel_ctr:
    {
      GCPtr<Type> t = ast->child(0)->symType->getBareType();
      out << "(" <<  "TAG_" << CMangle(t->myContainer) << "(";	

      if (!ast->child(0)->symType->isRefType())
	out << "&";

      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);
	
      out << ") == ";	
      out << ENUM_PFX << CMangle(ast->child(1), CMGL_ID_FLD)
	  << ")";
      break;
    }

  case at_select:
    {
      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);
      
      out << ((ast->child(0)->symType->isRefType()) ? "->" : "." );
      out << CMangle(ast->child(1), CMGL_ID_FLD);
      break;
    }

  case at_defunion:
    {
      GCPtr<AST> ident = ast->child(0);
      GCPtr<AST> ctrs = ast->child(4);

      bool repr = ast->Flags2 & UNION_IS_REPR;
      out << "/*** Tag Enumerations ***/" << endl;
      out << "typedef enum {" << endl;
      out.more(); 
	
      if (ident->Flags & CARDELLI_UN) {
	assert(!repr);
	for(size_t c=0; c < ctrs->children->size(); c++) {
	  GCPtr<AST> ctr = ctrs->child(c);
	  if(ctr->children->size() > 1)
	    out << ENUM_PFX << CMangle(ctr->child(0))
		<< " = 0," << endl;	
	  else {
	    out << ENUM_PFX << CMangle(ctr->child(0));
	    out << " = ";
	    out << ((2*c)+1);
	    out << "," << endl;	
	  }
	}
      }
      else {
	for(size_t c=0; c < ctrs->children->size(); c++) {
	  GCPtr<AST> ctr = ctrs->child(c);
	  out << ENUM_PFX << CMangle(ctr->child(0))
	      << " = " << c
	      << "," << endl;
	}
      }
      
      out.less();
      out << "} " << TAG_PFX << CMangle(ident) << ";" 
	  << endl << endl;
      
      
      out << "/*** Structures for constructor legs ***/" << endl;
      for(size_t c = 0; c < ctrs->children->size(); c++) {
	GCPtr<AST> ctr = ctrs->child(c);
	GCPtr<AST> ctrID = ctr->child(0);
	
	if(ctrID->stCtr == ctrID) {	
	  out << "typedef struct {" << endl;
	  out.more();
	  
	  if (!repr)
	    if ((ident->Flags & SINGLE_LEG_UN) == 0)
	      if(((ident->Flags & CARDELLI_UN) == 0) || 
		 (ctr->children->size() == 1)) {
		out << decl(ident->tagType, "tag", CTYP_EMIT_BF,
			    ident->field_bits)<< ";" << endl;
	      }
	  
	  for(size_t i = 1; i < ctr->children->size(); i++) {
	    GCPtr<AST> field = ctr->child(i);
	    TOC(errStream, uoc, field, out, IDname, ctr, i, flags);      
	  }
	  
	  out.less();
	  out << "} " << TY_PFX << CMangle(ctrID)
	      << ";" << endl << endl;
	}
	else { 
	  out << "typedef "
	      << TY_PFX << CMangle(ctrID->stCtr) << " "
	      << TY_PFX << CMangle(ctrID)
	      << ";" << endl << endl;
	}
      }

      out << "/*** Main union ***/"  << endl;
      out << "union " << TY_PFX << CMangle(ident) << "{" << endl;
      out.more();
      
      if (!repr)
	out << TAG_PFX << CMangle(ident) << " tag;" << endl;
      
      for(size_t c = 0; c < ctrs->children->size(); c++) {
	GCPtr<AST> ctr = ctrs->child(c);
	out << TY_PFX << CMangle(ctr->child(0)) << " "
	    << "leg_" << CMangle(ctr->child(0)) << ";" << endl;	
      }
      out.less();
      out << "};" << endl << endl;


      out << "/*** Tag Accessor ***/" << endl;
      if (!repr)
	out << "INLINE ";
      out << TAG_PFX << CMangle(ident) << endl
	  << "TAG_" << CMangle(ident) << "("
	  << toCtype(ident->symType);
      if (!ident->symType->isRefType())
	out << "*";
      out << " arg)" 
	  << endl
	  << "{"
	  << endl;
      out.more();
      
      string accessor = "arg->tag";
      
      if(repr) {
	for(size_t c = 0; c < ctrs->children->size(); c++) {
	  GCPtr<AST> ctr = ctrs->child(c);
	  out << "if (";
	  bool emitted1=false;
	  for(size_t i = 1; i < ctr->children->size(); i++) {
	    GCPtr<AST> field = ctr->child(i);
	    if(field->Flags2 & FLD_IS_DISCM) {
	      if(emitted1)
		out << " && ";
	      out << "("
		  << "((" << TY_PFX << CMangle(ctr->child(0)) << " *)" 
		  << "arg)->" 
		  << CMangle(field->child(0), CMGL_ID_FLD)
		  << " == "
		  << field->field_bits
		  << ")";
	      emitted1=true;
	    }
	  }
	  assert(emitted1);
	  out << ")" << endl;
	  out.more();
	  out << "return " << ENUM_PFX << CMangle(ctr->child(0)) << ";" << endl;
	  out.less();
	}
	
	out << " /* Keep the Compiler happy */ " << endl;
	out << " assert(false);" << endl;
	out << " return 0;" << endl;	
      }
      else if (ident->Flags & SINGLE_LEG_UN) {
	out << "return 0;" << endl;
      }
      else if (ident->Flags & CARDELLI_UN) {
	out << "if (" << accessor << " & 0x1u)" << endl;
	out.more();
	out << "return " << accessor << ";" << endl;
	out.less();
	out << "else" << endl;
	out.more();
	out << "return 0;" << endl;
	out.less();
      }
      else {
	out << "return " << accessor << ";" << endl;
      }
      out.less();      
      out << "}" 
	  << endl;
      
      out << "/*** Constructors: ***/" << endl;      
      std::string pre;
      stringstream udecl;
      if(ident->symType->getBareType()->kind == ty_unionv) {
	udecl << toCtype(ident->symType) << " val;" << endl;
	pre = "val.";
      }
      else {
	udecl << toCtype(ident->symType)
	      << " val = "
	      << "(" << toCtype(ident->symType) << ") "
	      << "GC_ALLOC(sizeof(" << TY_PFX << CMangle(ident) << "));" 
	      << endl;
	pre = "val->";
      }

      for(size_t c = 0; c < ctrs->children->size(); c++) {
	GCPtr<AST> ctr = ctrs->child(c);	
	GCPtr<AST> ctrID = ctr->child(0);
	out << "INLINE " << toCtype(ident->symType) << endl
	    << CTOR_PFX << CMangle(ctrID) << " ";

	emit_ct_args(out, ctr, 1);

	out << "{" << endl;
	out.more();
	out << udecl.str();
	out << TY_PFX << CMangle(ctrID) << " leg;" << endl;
	
	if(!repr)
	  if ((ident->Flags & SINGLE_LEG_UN) == 0)
	    if(((ident->Flags & CARDELLI_UN) == 0) || 
	       (ctr->children->size() == 1)) {
	      out << "leg.tag = " << ENUM_PFX << CMangle(ctrID) 
		  << ";" << endl;
	    }

	emit_ct_inits(out, ctr, "leg.", 1);

	out << pre << "leg_" << CMangle(ctrID) << " = leg;" << endl;
	out << "return val;" << endl;
	out.less();
	out << "}" << endl;
	out << endl;
      }

      break;
    }

  case at_constructors:
  case at_constructor:
    {
      assert(false);
      break;
    }

  case at_defrepr:
    //case at_reprbody:
    //case at_reprcase:
    //case at_reprcaselegR:
    //case at_reprtag:
    //case agt_reprbodyitem:
  case at_reprctrs:
  case at_reprctr:
  case at_reprrepr:
    {
      assert(false);
      break;
    }

  case at_defexception:
    {
      GCPtr<AST> ident = ast->child(0);

      if(flags & TOC_HEADER_MODE) {
	out << "extern const char " << TAG_PFX << CMangle(ident) << "[]; " 
	    << endl;
      }
      else {
	out << "const char " << TAG_PFX << CMangle(ident) << "[] = " 
	    << "\"" << ident->fqn.ident << "\"" << ";" << endl;
      }

      if(ast->children->size() > 1) {
	out << "typedef struct {" << endl;
	out.more();
	out << "const char* __name;" << endl;
	for(size_t c = 1; c < ast->children->size(); c++) {
	  GCPtr<AST> field = ast->child(c);
	  if(field->astType == at_fill)
	    continue;

	  out << decl(field->child(1)->symType, 
		      field->child(0)->s, true) 
	      << ";" << endl;	
	}
	out.less();
	out << "} " << TY_PFX << CMangle(ident) << ";" << endl << endl;
      
	out << "/* Exception Constructor */" << endl;
	out << "INLINE " << TY_PFX << CMangle(ident) << "*" << endl
	    << CTOR_PFX << CMangle(ident) << " ";

	emit_ct_args(out, ast, 1);
	out << "{" << endl;
	out.more();      
	out << TY_PFX << CMangle(ident) << "* val = (" << TY_PFX << CMangle(ident) << "*)"
	    << "GC_ALLOC(sizeof(" << TY_PFX << CMangle(ident) << "));" << endl;
	out << "val->__name = " << TAG_PFX << CMangle(ident) << ";" << endl;      
	
	emit_ct_inits(out, ast, "val->", 1);
	out << "return val;" << endl;
	out.less();       
	out << "}" << endl << endl;
      }
      else {
	// Static allocation for singled valued exceptions.
	out << "typedef bitc_exception_t " << TY_PFX << CMangle(ident) 
	    << ";" << endl;

	if(flags & TOC_HEADER_MODE) {
	  out << "extern bitc_exception_t " << CVAL_PFX << CMangle(ident) 
	      << ";" << endl;
	}
 	else {
	  out << "bitc_exception_t " << CVAL_PFX << CMangle(ident) 
	      << " = { " << endl;
	  out.more();
	  out << TAG_PFX << CMangle(ident) << endl;
	  out.less();
	  out << "};" << endl << endl;
	}
      }
      break;
    }

  case at_declunion:
  case at_declstruct:
  case at_declrepr:
    {
      GCPtr<AST> ident = ast->child(0);      
      if(ident->defn)
	ident = ident->defn;
      if(ident->Flags & DEF_DECLARED) {	
	break;
      }
      
      ident->Flags |= DEF_DECLARED;
      out << "typedef ";
      if (ast->astType == at_declunion || ast->astType == at_declrepr)
	out << "union ";
      else
	out << "struct ";
      out << TY_PFX << CMangle(ident) << " " 
	  << TY_PFX << CMangle(ident) << ";" << endl;
      break;
    }

  case at_proclaim:
    {
      GCPtr<AST> id = ast->getID();
      bool hasDefn = false;

      if(id->defn) {
	id = id->defn;
	hasDefn = true;
      }

      out << "extern ";
      id->Flags |= DEF_DECLARED;      
      
      //GCPtr<Type> t = id->symType->getType();      
      //if (hasDefn && t->isSimpleTypeForC() && (t->kind != ty_mutable))
      // out << "const ";
            
      if(id->symType->isFnxn() && !id->symType->isMutable()) {
	GCPtr<Type> ret = id->symType->getBareType()->Ret();
	GCPtr<Type> args = id->symType->getBareType()->Args();
	
	out << toCtype(ret) << " "	
	    << CMangle(id) << " (";
	
	for(size_t i=0; i < args->components->size(); i++) {
	  GCPtr<Type> arg = args->CompType(i);
	  if(i > 0)
	    out << ", ";
	  
	  out << toCtype(arg) << " ";
	  if(args->CompFlags(i) & COMP_BYREF)
	    out << "*";
	  out << " arg" << i;
	}
	out << ");"
	    << std::endl;
      }
      else {
	declare(out, id, "");
      }

      /* If this declaration has an external name, and is a function,
	 emit typedefs of names relative to the external name so that
	 they can easily be accessed */
      if(id->symType->isFnxn() && id->externalName.size()) {       
	GCPtr<Type> fnType = id->symType->getBareType();
	GCPtr<Type> ret = fnType->Ret();
	GCPtr<Type> args = fnType->Args();
	
	if(!typeIsUnmangled(ret)) {
	  out << "typedef "
	      << decl(ret, RET_PFX + id->externalName)
	      << ";" << endl;
	}
	
	for(size_t i=0; i < args->components->size(); i++) {
	  GCPtr<Type> arg = args->CompType(i);
	  
	  if(!typeIsUnmangled(arg)) {
	    stringstream as;
	    as << ARG_PFX << i << "_"  << id->externalName;
	    
	    out << "typedef "
		<< decl(arg, as.str())
		<< ";" << endl;
	  }
	}
      }

      break;
    }
    
  case at_define:
    {
      GCPtr<AST> id = ast->child(0)->child(0);

      // If this name is file-local, emit it as a static.
      // However, if there was a previous declaration, 
      // that would have been declared as extern, don't emit
      // it as static, unless it is a function
      
      if((id->Flags & DEF_DECLARED) == 0)
	if (id->Flags & ID_IS_PRIVATE)	  
	  out << "static ";
      
      if(ast->child(1)->astType == at_lambda) {
	// Function Label case
	// Mutable or immutable, we do the same thing. The mutable
	// case and name adjustment is taken care of by
	// emitGlobalInitializers() function.
	CHKERR(errorFree, emit_fnxn_label(errStream, uoc, ast, out,
					  parent, chno, flags));
      }
      else if(flags & TOC_HEADER_MODE) {
	// Header Mode
	// Header mode for function labels taken care of 
	// within the helper function.
	// Note: Don't worry about function pointers, 
	//       they are emitted as typedefs
	out << "extern ";
	out << decl(id) << ";"
	    << endl;	
      }
      else {
	// Non-function
	GCPtr<AST> e = ast->child(1);
	GCPtr<AST> p = ast;
	size_t c = 1;
	
	while(e->astType == at_tqexpr) {
	  p = e;
	  c = 0;
	  e = e->child(0);
	}
	
	//if (t->isSimpleTypeForC() && t->kind != ty_mutable)
	// out << "const ";
	
	out << decl(id) << " = ";
	TOC(errStream, uoc, e, out, IDname, p, c, flags);	  
	out << ";" << endl;	  
      }
      break;
    }

  case at_container:
    {
      if(ast->child(0)->children->size() != 0) {
	//out++;
	TOC(errStream, uoc, ast->child(0), out, IDname, 
	    ast, 0, flags);      
	out << "_" << IDname << ":" << endl;
	TOC(errStream, uoc, ast->child(1), out, IDname, 
	    ast, 1, flags);      
	//out--;
      }
      else {
	TOC(errStream, uoc, ast->child(1), out, IDname, 
	    ast, 1, flags);      
      }
      break;
    }

  case at_identList:
    {
      for(size_t c=0; c < ast->children->size(); c++)
	declare(out, ast->child(c));	
      break;
    }

  case at_suspend:
    {
      TOC(errStream, uoc, ast->child(1), out, IDname, 
	  ast, 1, flags);      
      
      break;
    }

  case at_tqexpr:
    {
      // match agt_eform
      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);      
      
      break;
    }    
    
  case at_do:
    {
      GCPtr<AST> dbs = ast->child(0);
      
      for (size_t c = 0; c < dbs->children->size(); c++) {
	GCPtr<AST> db = dbs->child(c);
	GCPtr<AST> init = db->child(1);
	TOC(errStream, uoc, init, out, IDname, db, 1, flags);      
      }

      out << "loop_" << ast->ID << ":" << endl;
      out.indent(2);

      GCPtr<AST> dotest = ast->child(1);
      GCPtr<AST> cond = dotest->child(0);
      GCPtr<AST> res = dotest->child(1);
      GCPtr<AST> body = ast->child(2);
      GCPtr<AST> theCond;

      if(cond->astType == at_letStar) {
	TOC(errStream, uoc, cond, out, IDname, 
	    dotest, 0, flags);      
	theCond = FEXPR(cond);
      }
      else {
	theCond = cond;
      }
	
      out << "if(";
      TOC(errStream, uoc, theCond, out, IDname, 
	  NULL, 0, flags);      
      out << ") {" << endl;
      out.more();
      TOC(errStream, uoc, res, out, IDname, 
	  dotest, 1, flags);
      out << endl;
      out.less();
      out << "}" << endl;

      out << "else {" << endl;
      out.more();
      TOC(errStream, uoc, body, out, IDname, 
	  ast, 1, flags);

      if(body->astType != at_letStar)
	out << ";" << endl;

      for (size_t c = 0; c < dbs->children->size(); c++) {
	GCPtr<AST> db = dbs->child(c);
	GCPtr<AST> step = db->child(2);
	TOC(errStream, uoc, step, out, IDname, db, 2, flags);      
      }
      
      out << "goto " << "loop_" << ast->ID << ";" << endl;
      out << endl;
      out.less();
      out << "}" << endl;
      out.indent(-2);
      break;
    }

  case at_begin:
    {
      // out++;
      for (size_t c = 0; c < ast->children->size(); c++) {
	TOC(errStream, uoc, ast->child(c), out, IDname, 
	    ast, c, flags);      
	
	out << ";" << endl;
      }
      // out--;
      
      break;
    }

  case at_not:
    {
      out << "(! ";
      TOC(errStream, uoc, ast->child(0), out, IDname, ast, 0, flags);
      out << ")";
      break;
    }

  case at_apply:
    {
      // FIX: Shap has a test case that shows that the thing in apply
      // position can be an arbitrary location expression, so this
      // assert is wrong.
      // assert(ast->child(0)->astType == at_ident);

      // There is some additional work needed here to deal with
      // self-recursion within LETREC bodies...

      if (ast->child(0)->astType == at_ident) {
	GCPtr<AST> id = ast->child(0);
	assert(id->symbolDef);

	if(id->Flags & SELF_TAIL) {
	  GCPtr<AST> lbps = id->symbolDef->defbps;
	  assert(lbps);
	  assert(ast->children->size() == lbps->children->size() + 1);
	  out << "/* Tail recursive application: */ " << endl;
	  for(size_t c = 0; c < lbps->children->size(); c++) {
	    GCPtr<AST> ident = lbps->child(c)->child(0);
	    TOC(errStream, uoc, ident, out, IDname, 
		lbps->child(c), 0, flags);
	    out << " = ";
	    TOC(errStream, uoc, ast->child(c+1), out, IDname, 
		ast, c+1, flags);
	    out << ";" << endl;
	  }
	  out << "goto " << "_" << CMangle(id) << ";" << endl;
	  break;
	}
      }
      
      GCPtr<Type> clType = ast->child(0)->symType->getBareType();
      assert(clType->kind == ty_fn);
      GCPtr<Type> argsType = clType->Args()->getType();

      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);
      
      out << "(";
      for(size_t c=1; c < ast->children->size(); c++) {
	if(c > 1)
	  out << ", ";	    
	
	if(argsType->CompFlags(c-1) & COMP_BYREF) {	  
	  assert(ast->child(c)->isLocation());
	  out << "&";
	}
	
	TOC(errStream, uoc, ast->child(c), out, IDname, 
	    ast, c, flags);
      }
      out << ")";
      break;
    }

  case at_array:    
    {
      assert(IDname.size());
      for (size_t c = 0; c < ast->children->size(); c++) {
	out << IDname << ".elem[" << c << "] = ";
 	TOC(errStream, uoc, ast->child(c), out, IDname, 
	    ast, c, flags);
	out << ";" << endl;
      }
      out << endl;
      break;
    }
    
  case at_vector:
    {
      if(IDname.size() == 0) {
	assert(1);
      }
      assert(IDname.size());
      GCPtr<Type> t = ast->symType->getBareType();
      out << IDname << " = (" << toCtype(t) << ") " 
	  << "GC_ALLOC(sizeof("
	  << CMangle(t->mangledString(true)) << ") + " 
	  << "(" << ast->children->size() << " * sizeof("
	  << toCtype(t->Base()) << ")));"
	  << endl;
 
      out << IDname << "->len = " << ast->children->size() 
	  << ";" << endl;
      out << IDname << "->elem = (("
	  << toCtype(t->Base())
	  << " *) (((char *) " << IDname << ") + "
	  << "sizeof(" << CMangle(t->mangledString(true)) << ")));"
	  << endl;
      for (size_t c = 0; c < ast->children->size(); c++) {
	out << IDname << "->elem[" << c << "] = ";
 	TOC(errStream, uoc, ast->child(c), out, IDname, 
	    ast, c, flags);
	out << ";" << endl;
      }
      out << endl;
      break;      
    }

  case at_makevectorL:
    {      
      assert(IDname.size());
      GCPtr<Type> t = ast->symType->getBareType();
      out << IDname << " = (" << toCtype(ast->symType) << ") " 
	  << "GC_ALLOC(sizeof("
	  << CMangle(t->mangledString(true)) << ") + " 
	  << "("; 
      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);      
      out << " * sizeof("
	  << toCtype(t->Base()) << ")));"
	  << endl;

      out << IDname << "->len = ";
      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);            
      out << ";" << endl;

      out << IDname << "->elem = (("
	  << toCtype(t->Base())
	  << " *) (((char *) " << IDname << ") + "
	  << "sizeof(" << CMangle(t->mangledString(true)) << ")));"
	  << endl;

      out << "{" << endl;
      out.more();
      out << "bitc_word_t __bitc_temp_mvec;" << endl;
      out << "for (__bitc_temp_mvec = 0; __bitc_temp_mvec < ";
      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);      
      out << "; __bitc_temp_mvec++)" << endl;
      out.more();	
      out << IDname << "->elem[__bitc_temp_mvec] = ";
      /* This is actually an application, get at_apply case to do the
	 work :) */
      GCPtr<AST> hackIdent = new AST(at_ident, ast->loc);
      hackIdent->s = "__bitc_temp_mvec";
      hackIdent->Flags |= ID_IS_GENSYM; // don't add extra astID after name
      GCPtr<AST> apply = new AST(at_apply, ast->loc, 
			   ast->child(1), hackIdent);
      TOC(errStream, uoc, apply, out, IDname, ast, 1, flags);
      out << ";" << endl;
      out.less();
      out.less();
      out << "}" << endl;
      out << endl;
      break;      
    }

  case at_array_length:
    {
      out << ast->child(0)->symType->getBareType()->arrlen->len;
      break;
    } 

  case at_vector_length:
    { 
      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);
      out << "->len";
      break;
    }

  case at_array_nth:
  case at_vector_nth:
    {
      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);

      if(ast->astType == at_array_nth)
	out << ".";
      else
	out << "->";

      out << "elem[";
      TOC(errStream, uoc, ast->child(1), out, IDname, 
	  ast, 1, flags);      
      out << "]";
      break;
    }

  case at_switchR:
    {
      GCPtr<AST> topExp = ast->child(0);
      GCPtr<AST> cases = ast->child(1);
      GCPtr<Type> t = topExp->symType->getBareType();
      
      out << "switch(";
      
      assert(t->kind == ty_uvalv || t->kind == ty_unionv ||
	     t->kind == ty_uvalr || t->kind == ty_unionr || 
	     t->kind == ty_uconr || t->kind == ty_uconv);

      out << "TAG_" << CMangle(t->myContainer) << "(";
      if (!topExp->symType->isRefType())
	out << "&";
      TOC(errStream, uoc, topExp, out, IDname, ast, 0, flags);
      out << ")";	        
      out << ") {" << endl;;
      out.more();
      
      for(size_t c=0; c < cases->children->size(); c++) {
	
	GCPtr<AST> theCase = cases->child(c);
	GCPtr<AST> expr = theCase->child(1);
	GCPtr<AST> legIdent = theCase->child(0);

	for(size_t n=2; n < theCase->children->size(); n++) {
	  GCPtr<AST> ctr = theCase->child(n)->getCtr();
	  
	  std::string leg = CMangle(ctr);	  
	  out << "case " << ENUM_PFX << leg << " :" << endl;
	}

	out.more();
	out << "{" << endl;
	out.more();

	TOC(errStream, uoc, legIdent, out, IDname, theCase, 0, flags);
	out << " = ";
	
	TOC(errStream, uoc, topExp, out, IDname, ast, 0, flags);	
	if(t->isValType()) 
	  out << ".";
	else
	  out << "->";	      

	out << "leg_" << CMangle(theCase->child(2)->getCtr());
	out << ";" << endl;
		
	TOC(errStream, uoc, expr, out, IDname, theCase, 1, flags);
	out << "break;";
	out << endl;
	out.less();
	out << "}" << endl;
	out.less();
	out << endl;
      } // for each case

      if(ast->child(2)->astType != at_Null) {
	out << "default:" << endl;
	out.more();
	out << "{" << endl;
	out.more();
	TOC(errStream, uoc, ast->child(2)->child(0), 
	    out, IDname, ast->child(2), 2, flags);
	out << "break;";
	out << endl;
	out.less();
	out << "}" << endl;
	out.less();
	out << endl;
      }
      out.less();
      out << "}" << endl;
            
      break;
    }

  case at_sw_legs:
  case at_sw_leg:
  case at_otherwise:
    {
      assert(false);
      break;
    }

  case at_tryR:
    {
      GCPtr<AST> topExpr = ast->child(0);
      GCPtr<AST> cases = ast->child(1);
      GCPtr<AST> ow = ast->child(2);
      out << "{" << endl;
      out.more();
      out << "jmp_buf jb;" << endl;
      out << "jmp_buf *lastJB = curCatchBlock;" << endl;
      out << "curCatchBlock = &jb;" << endl;
      out << endl;
      out << "int result = setjmp(jb);" << endl;
      out << "if(!result) {" << endl;
      out.more();
      TOC(errStream, uoc, topExpr, out, IDname, 
	  ast, 0, flags);
      out << "curCatchBlock = lastJB;" << endl;
      out.less();
      out << "}" << endl;
      out << "else {" << endl;
      out.more();
      out << "curCatchBlock = lastJB;" << endl;

      // Too bad I cannot use a switch ... 
      for(size_t c = 0; c < cases->children->size(); c++) {	
	GCPtr<AST> theCase = cases->child(c);
	GCPtr<AST> expr = theCase->child(1);
	GCPtr<AST> legIdent = theCase->child(0);

	if(c > 0) 
	  out << "else " << endl;
	 
	out << "if(";
	for(size_t n=2; n < theCase->children->size(); n++) {
	  GCPtr<AST> exn = theCase->child(n);
	  
	  if(n > 2)
	    out << " || ";
	  out << "(curException->__name == "
	      << TAG_PFX << CMangle(exn) << ")";
	}
	out << ") {" << endl;
	out.more();
	TOC(errStream, uoc, legIdent, out, IDname, theCase, 0, flags);
	out << " = "
	    << "*((" << toCtype(legIdent->symType, legIdent->s) <<  " *)" 
	    << "curException);" << endl;

	TOC(errStream, uoc, expr, out, IDname, theCase, 1, flags);
	out.less();
	out << "}" << endl;
      } /* for each case */
      
      if(ow->astType != at_Null) {
	out << "else {" << endl;
	out.more();
	TOC(errStream, uoc, ow->child(0), 
	    out, IDname, ow, 0, flags);
	out.less();
	out << "}" << endl;
	out << endl;
      }
      else {
	out << "else {" << endl;
        out.more();
	out << "longjmp(*curCatchBlock, 1);" << endl;
	out.less();
	out << "}" << endl;
      }
      out.less();
      out << "}" << endl; // else case in setjmp () return
      out.less();
      out << "}" << endl; //Entire try/catch block
      
      break;
    }

  case at_throw:    
    {
      out << "bitc_throw(";
      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);
      out <<");" << endl;
      break;
    }

  case at_setbang:
    {
      assert(IDname.size());

      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);
      out << " = ";
      TOC(errStream, uoc, ast->child(1), out, IDname, 
	  ast, 1, flags);
      out << ";" << endl;
      
      out << IDname << " = (bitc_unit_t) 0;" << endl;
      break;      
    }

  case at_dup:
    {
      assert(IDname.size());      
      GCPtr<AST> arg = ast->child(0);      
      out << IDname << " = "
	  << "(("
	  << toCtype(ast->symType)
	  << ") GC_ALLOC(sizeof("
	  << toCtype(arg->symType)
	  << ")));" << endl;
      out << "*" << IDname << " = ";
      TOC(errStream, uoc, arg, out, IDname, ast, 0, flags);
      out << ";" << endl;
      break;
    }

  case at_deref:
    {
      out << "(* ";
      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);            
      out << ")";
      break;
    }

  case at_inner_ref:
    {      
      out << "&";
      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);
      out << "->";
      
      if(ast->Flags2 & INNER_REF_NDX) {
	out << "elem[";
	TOC(errStream, uoc, ast->child(1), out, IDname, 
	    ast, 1, flags);      
	out << "]";
      }
      else {
	out << CMangle(ast->child(1), CMGL_ID_FLD);
      }
      break;
    }
    
  case at_if:
    {
      GCPtr<AST> testAst = ast->child(0);
      GCPtr<AST> ifAst = ast->child(1);
      GCPtr<AST> elseAst = ast->child(2);
      out << "if(";
      TOC(errStream, uoc, testAst, out, IDname, 
	  ast, 0, flags);      
      out << ") {" << endl;
      out.more();
      TOC(errStream, uoc, ifAst, out, IDname, 
	  ast, 1, flags);
      if(ifAst->astType != at_letStar)
	out << ";";
      out << endl;
      out.less();
      out << "}" << endl;
      out << "else {" << endl;
      out.more();
      TOC(errStream, uoc, elseAst, out, IDname, 
	  ast, 2, flags);
      if(elseAst->astType != at_letStar)
	out << ";";
      out << endl;
      out.less();
      out << "}" << endl;
      break;
    }
    
  case at_letStar:
  case at_letrec:
  case at_let:
    {
      //++out;
      TOC(errStream, uoc, ast->child(0), out, IDname, 
	  ast, 0, flags);

      GCPtr<AST> lExpr = ast->child(1);
      if(lExpr->astType == at_tqexpr)
	lExpr = lExpr->child(0);

      switch(lExpr->astType) {
      case at_ident:
      case at_boolLiteral:
      case at_charLiteral:
      case at_intLiteral:
      case at_floatLiteral:
      case at_stringLiteral:
	break;
	
      default:
	// The only thing I expect here is due to a GrandLet	
	TOC(errStream, uoc, ast->child(1), out, IDname, 
	    ast, 1, flags);      
	out << ";" << endl;	
	break;
      }
      //--out;
      break;
    }

  case at_letbindings:
    {
      for(size_t i=0; i < ast->children->size(); i++)
	TOC(errStream, uoc, ast->child(i), out, IDname, 
	    ast, i, flags);      
      break;
    }

  case at_letbinding:
    {
      assert(ast->child(0)->astType == at_identPattern);
      GCPtr<AST> ident = ast->child(0)->child(0);
      if((ast->Flags & LB_IS_DUMMY) == 0) {	
	if ((ast->Flags & LB_POSTPONED) == 0)
	  out << CMangle(ident) << " = ";
      }

      TOC(errStream, uoc, ast->child(1), out, CMangle(ident), 
	  ast, 1, flags);
      if(((ast->Flags & LB_IS_DUMMY) == 0) &&
	 ((ast->Flags & LB_POSTPONED) == 0))
	out << ";" << endl;       
      break;
    }
  }
  return errorFree;
}

static bool
alreadyEmitted(GCPtr<Type> t, 
	       GCPtr< CVector<string> > vec)
{
  for(size_t c = 0; c < vec->size(); c++)
    if(vec->contains(CMangle(t->mangledString(true))))
      return true;
  
  //std::cerr << "Emiting: " << CMangle(t->mangledString(true))
  //	  << " for " << t->asString()
  // 	  << std::endl;
  
  return false;
}

void
emit_arr_vec_fn_types(GCPtr<Type> candidate, 
			 INOstream &out,
			 GCPtr< CVector<string> > arrVec,
			 GCPtr< CVector<string> > vecVec,
			 GCPtr< CVector<string> > fnVec)
{
  GCPtr<Type> t = candidate->getBareType();
  if(t->mark & MARK13)
    return;

  t->mark |= MARK13;

  for(size_t i=0; i<t->typeArgs->size(); i++)
    emit_arr_vec_fn_types(t->TypeArg(i), out, 
			     arrVec, vecVec, fnVec);
  
  for(size_t i=0; i<t->components->size(); i++)
    emit_arr_vec_fn_types(t->CompType(i), out, 
			     arrVec, vecVec, fnVec);
  
  switch(t->kind) {
  case ty_array:
    {
      if(alreadyEmitted(t, arrVec))
	break;

      if(t->arrlen->len == 0) {
	assert(false);
      }

      arrVec->append(CMangle(t->mangledString(true)));
      //emitted = true;
      //std::cerr << "Emitted: " << CMangle(t->mangledString(true))
      //	  << " for " << t->asString()
      // 	  << std::endl;
      
      GCPtr<Type> et = t->Base()->getBareType(); 
      out << "/* Typedef in anticipation of the array type:"
	  << endl
	  << t->asString()
	  << "*/" << endl;
	 
      out << "typedef struct {" << endl;
      out.more();

      out << toCtype(et) << " elem[" << t->arrlen->len << "];" << endl;
      out.less();
      out << "} " << CMangle(t->mangledString(true)) 
	  << ";" << endl << endl;
      break;
    }

  case ty_vector:
    {
      if(alreadyEmitted(t, vecVec))
	break;

      vecVec->append(CMangle(t->mangledString(true)));
      
      GCPtr<Type> et = t->Base()->getBareType(); 
      out << "/* Typedef in anticipation of the vector type:"
	  << endl
	  << t->asString()
	  << "*/" << endl;

      out << "typedef struct {" << endl;
      out.more();

      out << "bitc_word_t len;" << endl;      
      out << toCtype(et) << " *elem;" << endl;
      out.less();
      out << "} " << CMangle(t->mangledString(true)) 
	  << ";" << endl << endl;
      break;
    }

  case ty_fn:
    {
      if(alreadyEmitted(t, fnVec))
	break;

      fnVec->append(CMangle(t->mangledString(true)));
      
      GCPtr<Type> args = t->Args()->getBareType(); 
      GCPtr<Type> ret = t->Ret()->getBareType(); 
      
      out << "/* Typedef in anticipation of the function (pointer) type:"
	  << endl
	  << t->asString()
	  << "*/" << endl;
      out << "typedef " 
	  << toCtype(ret)
	  << "  (*"
	  << CMangle(t->mangledString(true))
	  << ") (";

      for(size_t i=0; i < args->components->size(); i++) {
	GCPtr<Type> arg = args->CompType(i);
	if(i > 0)
	  out << ", ";
	
	out << toCtype(arg)	
	    << " arg" << i;
      }
      out << ");"
	  << endl << endl;            
      break;
    }
    
  default:
    {
      break;
    }
  }
  
  t->mark &= ~MARK13;
} 

  
void
emit_arr_vec_fn_types(GCPtr<AST> ast, 
			 INOstream &out,
			 GCPtr< CVector<string> > arrVec,
			 GCPtr< CVector<string> > vecVec,
			 GCPtr< CVector<string> > fnVec)
{  
  if(ast->symType)
    emit_arr_vec_fn_types(ast->symType, out,
			     arrVec, vecVec, fnVec);
  

  for(size_t c = 0; c < ast->children->size(); c++)
    emit_arr_vec_fn_types(ast->child(c), out, 
			     arrVec, vecVec, fnVec);
} 


bool
TypesTOC(std::ostream& errStream, 
	 GCPtr<UocInfo> uoc,
	 GCPtr<AST> start,  
	 INOstream &out,
	 unsigned long flags)
{
  bool errFree = true;
  GCPtr<AST> mod = start->child(0);
  GCPtr< CVector<string> > arrVec = new CVector<string>; 
  GCPtr< CVector<string> > vecVec = new CVector<string>; 
  GCPtr< CVector<string> > fnVec = new CVector<string>; 
  
  for(size_t c=0; (c < mod->children->size()); c++) {
    GCPtr<AST> ast = mod->child(c);

    switch(ast->astType) {
    case at_declstruct:
    case at_declunion:
      {	 
	
	//out << "#line " << ast->loc.line 
	//    << " \"" << *(ast->loc.path) << "\""
	//    << std::endl;

	out << "/***************************************" << endl
	    << "   " << ast->loc << endl
	    << "   " << ast->asString() 
	    << "***************************************/" << endl;
	CHKERR(errFree, toc(errStream, uoc, ast, out, "", 
			    mod, c, flags));
	out << endl;	
	break;
      }

    case at_defstruct:
    case at_defunion:
      {

	//out << "#line " << ast->loc.line 
	//    << " \"" << *(ast->loc.path) << "\""
	//    << std::endl;

	emit_arr_vec_fn_types(ast, out, arrVec, vecVec, 
				 fnVec);
 
	out << "/***************************************" << endl
	    << "   " << ast->loc << endl
	    << "   " << ast->asString() 
	    << "***************************************/" << endl;
	GCPtr<AST> ident = ast->child(0);
	if((ident->Flags & DEF_DECLARED) == 0) 
	  {
	    ident->Flags |= DEF_DECLARED;

	    out << "/* Forward declaration */" << endl;
	    out << "typedef ";
	    out << ((ast->astType == at_defstruct) ? "struct " : "union ");
	    out << TY_PFX << CMangle(ident) << " " 
		<< TY_PFX << CMangle(ident) << ";" << endl;
	    out << endl;
	  }
 
	CHKERR(errFree, toc(errStream, uoc, ast, out, "", 
			    mod, c, flags));
	out << endl << endl;
	break;
      }

    case at_defexception:
      {

	//out << "#line " << ast->loc.line 
	//    << " \"" << *(ast->loc.path) << "\""
	//    << std::endl;
	
	emit_arr_vec_fn_types(ast, out, arrVec, vecVec, 
				 fnVec); 

	out << "/***************************************" << endl
	    << "   " << ast->loc << endl
	    << "   " << ast->asString() 
	    << "***************************************/" << endl;
	CHKERR(errFree, toc(errStream, uoc, ast, out, "", 
			    mod, c, flags));
	out << endl << endl;

	break;
      }
      
    case at_proclaim:
    case at_define:
      {
	emit_arr_vec_fn_types(ast, out, arrVec, vecVec, 
				 fnVec);
	break;
      }

    default:
      {
	break;      
      }
    }
  }
 
  return errFree;
}

static bool
emitInitProc(std::ostream& errStream, GCPtr<AST> ast, GCPtr<UocInfo> uoc,
	     INOstream &out, INOstream &initStream, 
	     unsigned long flags)
{
  bool answer = true;
  bool errorFree = true;
  assert(ast->astType == at_define);
  out << "static " 
      << toCtype(ast->getID()->symType)
      << endl
      << "__init" << ast->ID << "()" << endl
      << "{" << endl;
  out.more();    

  GCPtr<AST> id = ast->child(0)->child(0);
  GCPtr<AST> body = ast->child(1);
  GCPtr<AST> ret;
  assert(body->astType == at_container);
  if(body->child(1)->astType == at_letStar) {
    TOC(errStream, uoc, body, out, CMangle(id), ast, 1, flags);
    out << ";" << endl; 
    
    ret = FEXPR(body->child(1));	
  }
  else {
    ret = body->child(1); // trivial return
  }
  assert(ret);
  out <<  "return ";
  TOC(errStream, uoc, ret, out, CMangle(id), ast, 1, flags);	
  out << ";" << endl;
  out.less();
  out << "}" << endl;
	
  initStream << CMangle(ast->getID()) << " = "; 
  initStream << "__init" << ast->ID << "();" << endl;	    
  return errorFree;
}

bool
EmitGlobalInitializers(std::ostream& errStream, 
		       GCPtr<UocInfo> uoc,
		       GCPtr<AST> start, 
		       INOstream &out,
		       unsigned long flags)
{
  bool errFree = true;
  stringstream is;
  INOstream initStream(is);

  initStream.more();
  
  GCPtr<AST> mod = start->child(0);
  for(size_t c = 0; c < mod->children->size(); c++) {
    GCPtr<AST> ast = mod->child(c);

    switch(ast->astType) {
    case at_define:
      {
	// Later, we might consider: 
	//initStream << "#line " << ast->loc.line 
	//    << " \"" << *(ast->loc.path) << "\""
	//    << std::endl;

	out << "/***************************************" << endl
	    << "   " << ast->loc << endl
	    << "   " << ast->asString() 
	    << "***************************************/" << endl;    
	
	GCPtr<AST> id = ast->getID();
	GCPtr<AST> label = NULL;
	bool wrapperNeeded = false;

	// Case 0: Immutable functions that are not of the form
	//           (define f (lambda (...) ... ))
	//           These may be cases like
	//           (define plus +)
	//           (define f (let ((x 2)) (lambda ... )))
	// In this case too, we must emit a label. We emit the
	// actual function as a pointer, initialize it as applicable
	// (trivial or through initialization procedure), and finally
	// emit a wrapper function as a label.
	if ((id->symType->isFnxn()) && (!id->symType->isMutable()) &&
	    (ast->child(1)->astType != at_lambda)) {
	  wrapperNeeded = true;
	  label = new AST(id);
	  ast->rename(id, WFN_PFX + id->s);
	}
	
	if(ast->Flags2 & DEF_IS_TRIVIAL_INIT) {
	  // Case 1: marked trivial initializer, 
	  //         including immutable functions that are of the form
	  //         (define f (lambda (...) ... ))
	  // Header-Mode is taken care of by TOC()
	  CHKERR(errFree, toc(errStream, uoc, ast, out, "", 
			      mod, c, flags));
	}
	else if(ast->child(1)->astType == at_lambda) {	  
	  assert(!id->symType->isMutable()); // Immutable lambda
     	           // definitions are marked DEF_IS_TRIVIAL_INIT	
	  // Case 2: Mutable functions that are of the form
	  //         (define f (lambda (...) ... ))
	  // We emit a label and a pointer. First we must emit a
	  // declaration for the (mutable) pointer, then the label
	  // (full function), and finally, initialize the pointer.
	  
	  GCPtr<AST> ptr = new AST(id);
	  id->s = MFN_PFX + id->s;
	  out << "extern " << decl(ptr) << ";" << endl;
	  CHKERR(errFree, toc(errStream, uoc, ast, out, "", 
			      mod, c, flags));	  
	  out << decl(ptr) << " = " << CMangle(id) 
	      << ";" << endl << endl;
	  id->s = ptr->s; // just in case ...
	}	
	else {
	  // case 3: Non-trivial initialization value
	  //         Needs an initialization procedure
	  
	  // This is actually the definition, but the value
	  // will be initialized later from main()
	  declare(out, ast->getID());
	  
	  // Emit a procedure that will initialize this value
	  CHKERR(errFree, emitInitProc(errStream, ast, uoc, out, initStream, flags));
	}
	
	if (wrapperNeeded) {
	  GCPtr<Type> fnType = id->symType->getBareType();
	  GCPtr<Type> ret = fnType->Ret();
	  GCPtr<Type> args = fnType->Args()->getBareType();
	  
	  out << toCtype(ret) << endl	  
	      << CMangle(label);
	  out << "(";
	  for(size_t i=0; i < args->components->size(); i++) {
	    if(i > 0)
	      out << ", ";
	    
	    GCPtr<Type> arg = args->CompType(i);
	    out << toCtype(arg) << " arg" << i;
	  }
	  out << ")" << endl;
	  out << "{" << endl;
	  out.more();
	  
	  // We won't get caught here, BitC functions 
	  // must return something! 
	  out << "return " << CMangle(id);
	  out << "(";
	  for(size_t i=0; i < args->components->size(); i++) {
	    if(i > 0)
	      out << ", ";
	    
	    out << "arg" << i;
	  }
	  out << ");";

	  out.less();
	  out << "}" << endl << endl;	  
	} else {
	}
	
	break;
      }
      
    case at_proclaim:
      {
	//initStream << "#line " << ast->loc.line 
	//    << " \"" << *(ast->loc.path) << "\""
	//    << std::endl;

	out << "/***************************************" << endl
	    << "   " << ast->loc << endl
	    << "   " << ast->asString() 
	    << "***************************************/" << endl;    
	
	CHKERR(errFree, toc(errStream, uoc, ast, out, "", 
			    mod, c, flags));
	break;
      }

    default:
      {
	break;      
      }
    }
    out << endl << endl;      
  }  
  
  initStream.less();

  // Making this unconditional simplifies things, and it does not
  // really hurt us.
  out << "/***************************************"  << endl
      << "         THE   initializer              "  << endl
      << "***************************************/"  << endl;
  if (UocInfo::mainIsDefined)
    out << "static " ;
  out << "void"                                      << endl
      << "bitc_init_globals()"                       << endl
      << "{" << endl;
  out << is.str();
  out << "}" << endl;
  out << endl;

  
  return errFree;
}

bool
EmitMain(INOstream &out)
{
  bool errFree = true;
  
  out << "/***************************************"  << endl
      << "         THE   main()                   "  << endl
      << "***************************************/"  << endl
      << "int"                                       << endl
      << "main(int argc, char*argv[])"               << endl
      << "{"                                         << endl;
  out.more();
  out << "int result;" << endl; 
  out << "int i;" << endl << endl;

  out.less();
  out << "#if defined(__linux__)" << endl;
  out.more();
  out << "int my_personality = personality(-1);" << endl;
  out << "my_personality |= 0x0400000; /* READ_IMPLIES_EXEC */" << endl;
  out << "personality(my_personality);" << endl;
  out.less();
  out << "#endif" << endl;
  out.more();

  out << "result = setjmp(firstJB);" << endl
      << "if(!result) {" << endl;
  out.more();
  
  out << "bitc_init_globals();" << endl;
  
  out << "TY_VECTOR_OF_STRINGS *argVec = " << endl;
  out.more();
  out << "(TY_VECTOR_OF_STRINGS*) GC_ALLOC(sizeof(TY_VECTOR_OF_STRINGS));" 
      << endl;
  out.less();

  out << "argVec->len = argc;" << endl
      << "argVec->elem = " << endl;
  out.more();
  out << "(bitc_string_t **) GC_ALLOC(sizeof(bitc_string_t *) * argc);"
      << endl;
  out.less();
  out << "for(i = 0; i < argc; i++)" << endl;
  out.more(); 
  out << "argVec->elem[i] = mkStringLiteral(argv[i]);" << endl;
  out.less();
  out << endl;

  out << "return bitc_main(argVec)"
      << ";" << endl;
    
  out.less();
  out << "}" << endl;
  out << "else {" << endl;
  out.more();
  out << "printf(\"Uncaught Exception: %s\\n\", "
      << "curException->__name);" << endl
      << "exit(1);" << endl;
  out.less();
  out << "}" << endl;
  out.less();
  out  << "}"  << endl;
  
  return errFree;
}

bool
ValuesTOH(std::ostream& errStream, 
	  GCPtr<UocInfo> uoc,
	  GCPtr<AST> start, 
	  INOstream &out,
	  unsigned long flags)
{
  bool errFree = true;
  
  GCPtr<AST> mod = start->child(0);
  for(size_t c = 0; c < mod->children->size(); c++) {
    GCPtr<AST> ast = mod->child(c);
    switch(ast->astType) {
    case at_proclaim:
      {
	if(ast->getID()->Flags & DEF_IS_EXTERNAL) {
	  out << "/***************************************" << endl
	      << "   " << ast->loc << endl
	      << "   " << ast->asString() 
	      << "***************************************/" << endl;      
	  CHKERR(errFree, toc(errStream, uoc, ast, out, "", 
			      mod, c, flags));
	  out << endl;
	}
	break;
      }
    default:
      {
	break;      
      }
    }
  }
  return errFree;
}

bool
GenerateCoutput(std::ostream &errStream, INOstream &out,
		unsigned long flags, GCPtr<UocInfo> uoc)
{
  bool errFree = true;
  
  assert(uoc);

  out << "/********************************************" << endl
      << "   This code was automatically generated by "  << endl
      << "   BitC compiler version " << Version()        << endl
      << endl
      << "        !!!     DO NOT EDIT     !!!   "        << endl
      << "         !!  uness you are sure !!  "          << endl
      << "********************************************/" << endl;
  
  //  ifstream runtime(BITCCDIR"/runtime.h");
  
  // if (!runtime.is_open()) {
  //    errStream << BITCCDIR"/runtime.h cannot be found"
  //	      << endl;
  // return false;
  //}
  
  //string s;
  //while (!runtime.eof()) {
  //  getline(runtime, s);
  //  out << s << endl;
  // }
  //runtime.close();

  out << "#include <bitc/runtime.h>" << endl << endl;

  out << "#if defined(__linux__)" << endl;
  out << "#include <sys/personality.h>" << endl;
  out << "#endif" << endl << endl;
  
  if((flags & TOC_HEADER_MODE) == 0) {
    out << "jmp_buf firstJB;" << endl << endl;
    out << "jmp_buf *curCatchBlock = &firstJB;" << endl;
    out << "bitc_exception_t *curException;" << endl << endl;
  }

  CHKERR(errFree, TypesTOC(errStream, uoc, uoc->ast, 
			   out, flags));

  if(flags & TOC_HEADER_MODE) {
    CHKERR(errFree, ValuesTOH(errStream, uoc, uoc->ast, out, flags));
  }
  else {
    // If there are *any* entry points, emit the procedure that
    // handles global initialization so that it can be called:
    if (Options::entryPts->size())
      CHKERR(errFree, EmitGlobalInitializers(errStream, uoc, uoc->ast, out, flags));    

    // If bitc.main.main is an entry point, emit the wrapping main
    // procedure that calls the global initializers and processes the
    // argument vector.
    if (UocInfo::mainIsDefined)
      EmitMain(out);
  }

  return errFree;
}

bool
EmitHeader(std::ostream &optStream, std::ostream &errStream,
	   GCPtr<UocInfo> uoc)
{
  std::ofstream out(Options::outputFileName.c_str(),
		    std::ios_base::out|std::ios_base::trunc);
  
  if (!out.is_open()) 
    errStream << "Couldn't open output file \""
	      << Options::outputFileName
	      << "\" -- "
	      << strerror(errno)
	      << endl;
  
  INOstream ino_out(out);
  bool result = GenerateCoutput(errStream, ino_out, 
				TOC_HEADER_MODE, uoc);
  out.close();
  return result;
}

bool
EmitC(std::ostream &optStream, std::ostream &errStream,
      GCPtr<UocInfo> uoc)
{  
  std::ofstream out(Options::outputFileName.c_str(),
		    std::ios_base::out|std::ios_base::trunc);
  if (!out.is_open()) 
    errStream << "Couldn't open output file \""
	      << Options::outputFileName
	      << "\" -- "
	      << strerror(errno)
	      << endl;

  INOstream ino_out(out);
  bool result = GenerateCoutput(errStream, ino_out, 0, uoc);
  out.close();
  return result;
}

bool
EmitObj(std::ostream &optStream, std::ostream &errStream,
	GCPtr<UocInfo> uoc)
{
  std::ofstream csrc("bitc.out.c",
		     std::ios_base::out|std::ios_base::trunc);

  if (!csrc.is_open()) {
    errStream << "Couldn't open auxiliary file \""
	      << "bitc.out.c"
	      << "\" -- "
	      << strerror(errno)
	      << "\n";
    return false;
  }
  
  INOstream out(csrc);
  bool result = GenerateCoutput(errStream, out, 0, uoc);
  csrc.close();
  
  if(!result)
    return false;
  
  stringstream opt;
  opt << "gcc -O2 ";

  for (size_t i = 0; i < UocInfo::searchPath->size(); i++)
    opt << " -I " << *UocInfo::searchPath->elem(i);

  for (size_t i = 0; i < Options::libPath->size(); i++)
    opt << " -L " << *Options::libPath->elem(i);

  opt << " -o " << Options::outputFileName;
  opt << " bitc.out.c";
  opt << " -lbitc";
  opt << " -lgc";
  opt << " --std=c99";

  std::cerr  << opt.str() << std::endl;

  system(opt.str().c_str());
  //system("rm -f bitc.out.c");
  
  return true;
}
