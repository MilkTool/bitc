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
#include <sstream>
#include <string>
#include <libsherpa/UExcept.hxx>
#include <libsherpa/CVector.hxx>
#include <assert.h>

#include "UocInfo.hxx"
#include "Options.hxx"
#include "AST.hxx"
#include "Type.hxx"
#include "TypeInfer.hxx"
#include "inter-pass.hxx"

using namespace sherpa;

bool 
UocInfo::Resolve(std::ostream& errStream, bool init, 
		 unsigned long rflags, std::string mesg)
{
  bool errFree = true;
  errFree = fe_symresolve(errStream, init, rflags);
  if(!errFree)
    errStream << mesg
	      << std::endl;
  return errFree;
}

bool 
UocInfo::TypeCheck(std::ostream& errStream, bool init, 
		   unsigned long tflags, std::string mesg)
{
  bool errFree = true;

  // If one considers removing this clear clause,
  // be careful about old types. Pay attention to
  // bindIdentDef() function which preserves types
  // if a type already exists.
  ast->clearTypes();  

  errFree = fe_typeCheck(errStream, init, tflags);    
  if(!errFree) 
    errStream << mesg
	      << std::endl;
  return errFree;
}

bool 
UocInfo::RandT(std::ostream& errStream,
	       bool init, 
	       unsigned long rflags,
	       unsigned long tflags,
	       std::string mesg)
{
  bool errFree = true;

  CHKERR(errFree, Resolve(errStream, init, rflags, mesg));
  
  if(errFree)
    CHKERR(errFree, TypeCheck(errStream, init, tflags, mesg));
  
  if(!errFree)
    errStream << "WHILE R&Ting:" << std::endl
	      << ast->asString() << std::endl;
  
  return errFree;
}


void 
UocInfo::wrapEnvs()
{
  env = env->newDefScope();
  gamma = gamma->newDefScope();
  instEnv = instEnv->newDefScope();  
}

void 
UocInfo::unwrapEnvs()
{
  env = env->defEnv;
  gamma = gamma->defEnv;
  instEnv = instEnv->defEnv;
}

// The following is used to R&T any sub-expression

bool
resolve(std::ostream& errStream, 
	GCPtr<AST> ast, 
	GCPtr<Environment<AST> > env,
	GCPtr<Environment<AST> > lamLevel,
	int mode, 
	IdentType identType,
	GCPtr<AST> currLB,
	unsigned long flags);

bool
typeInfer(std::ostream& errStream, GCPtr<AST> ast, 
	  GCPtr<Environment<TypeScheme> > gamma,
	  GCPtr<Environment< CVector<GCPtr<Instance> > > > instEnv,
	  GCPtr<CVector<GCPtr<Type> > > impTypes,
	  bool isVP, 
	  GCPtr<TCConstraints> tcc,
	  unsigned long uflags,
	  GCPtr<Trail> trail,
	  int mode,
	  unsigned flags);

bool 
UocInfo::RandTexpr(std::ostream& errStream,
		   GCPtr<AST> expr,
		   unsigned long rflags,
		   unsigned long tflags,
		   std::string mesg,
		   bool keepResults,
		   GCPtr<envSet> altEnvSet)
{
  bool errFree = true;
  GCPtr<UocInfo> myUoc = new UocInfo(this);
  myUoc->ast = expr;
  
  if(altEnvSet) {
    myUoc->env = altEnvSet->env;
    myUoc->gamma = altEnvSet->gamma;
    myUoc->instEnv = altEnvSet->instEnv;    
  }
  
  if(!keepResults) 
    myUoc->wrapEnvs();
  
  CHKERR(errFree, myUoc->Resolve(errStream, false, rflags, mesg));
  if(errFree)
    CHKERR(errFree, myUoc->TypeCheck(errStream, false, tflags, mesg));
  
  if(!keepResults)
    myUoc->unwrapEnvs();
  
  if(!errFree)
    errStream << "WHILE R&Ting:" << std::endl
	      << expr->asString() << std::endl;

  return errFree;
}


#define MARKDEF(ast, def) do {\
    assert(def);	      \
    ast->defForm = def;	      \
  } while(0);
//std::cout << "Marked " << ast->asString() << "->defForm = "
//	<< def->asString() << std::endl;


/** @brief For every defining occurrence of an identifier, set up a
 * back pointer to the containing defining form. 
 *
 * See the explanation in AST.ast*/
void
UocInfo::findDefForms(GCPtr<AST> ast, GCPtr<AST> local, GCPtr<AST> top)
{
  ast->uoc = this;
  bool processChildren = true; 

  switch(ast->astType) {        
  case at_let:
  case at_letrec:
  case at_letStar:
    {
      MARKDEF(ast, top);
      local = ast;
      break;
    }
    
  case at_letbindings:
    {
      assert(local != top);
      MARKDEF(ast, local);
      local = ast;
      break;
    }

  case at_letbinding:
    {
      MARKDEF(ast, local);
      GCPtr<AST> id = ast->child(0)->child(0);
      MARKDEF(id, ast);
      break;
    }
    
  case at_define:
    {
      GCPtr<AST> id = ast->child(0)->child(0);
      MARKDEF(id, ast);
      top = ast;
      local = ast;
      break;
    }
    
  case at_declstruct:
  case at_declunion:
  case at_proclaim:
  case at_defstruct:
  case at_defexception:
    {
      GCPtr<AST> id = ast->child(0);
      MARKDEF(id, ast);
      processChildren = false;
      break;
    }

  case at_defunion:
    {
      GCPtr<AST> id = ast->child(0);
      MARKDEF(id, ast);
      GCPtr<AST> ctrs = ast->child(4);
      for(size_t i=0; i < ctrs->children->size(); i++) {
	GCPtr<AST> ctr = ctrs->child(i);
	GCPtr<AST> ctrID = ctr->child(0);
	MARKDEF(ctrID, ast);
      }
      processChildren = false;
      break;
    }

  case at_deftypeclass:
    {
      GCPtr<AST> id = ast->child(0);
      MARKDEF(id, ast);
	
      GCPtr<AST> methods = ast->child(3);
      for(size_t i = 0; i < methods->children->size(); i++) {
	GCPtr<AST> method = methods->child(i);
	GCPtr<AST> mID = method->child(0);
	MARKDEF(mID, ast);
      }
      processChildren = false;
      break;
    }
    
  case at_do:
    {
      local = ast;
      break;
    }

  case at_dobindings:
    {
      assert(local != top);
      MARKDEF(ast, local);
      local = ast;
      break;    
    }

  case at_dobinding:
    {
      MARKDEF(ast, local);
      GCPtr<AST> id = ast->child(0)->child(0);
      MARKDEF(id, ast);
      break;
    }
    
  default:
    {
      break;
    }
  }
  
  if(processChildren)
    for(size_t c=0; c < ast->children->size(); c++)
      findDefForms(ast->child(c), local, top);	  
}

/** @brief Make a pass over every AST, setting up back pointers to the
 * containing forms of all defining occurrences. */
void
UocInfo::findAllDefForms()
{
  for(size_t i = 0; i < UocInfo::srcList->size(); i++) {
    GCPtr<UocInfo> puoci = UocInfo::srcList->elem(i);
    puoci->findDefForms(puoci->ast);
  }

  for(size_t i = 0; i < UocInfo::ifList->size(); i++) {
    GCPtr<UocInfo> puoci = UocInfo::ifList->elem(i);
    puoci->findDefForms(puoci->ast);
  }
}

static void
addCandidates(GCPtr<AST> mod)
{
  for(size_t c = 0; c < mod->children->size(); c++) {
    GCPtr<AST> ast = mod->child(c);
    GCPtr<AST> id = ast->getID();
    switch(ast->astType) {
    case at_proclaim: // proclaims needed to keep externalNames
    case at_define:      
    case at_defexception:
      if(id->symType->isConcrete())
	Options::entryPts->append(id->fqn.asString());
      
      break;
      
    default:
	break;
    }
  }
}


void  
UocInfo::addAllCandidateEPs()
{
  for(size_t i = 0; i < UocInfo::ifList->size(); i++) {
    GCPtr<UocInfo> puoci = UocInfo::ifList->elem(i);
    addCandidates(puoci->ast->child(0));
  }  

  for(size_t i = 0; i < UocInfo::srcList->size(); i++) {
    GCPtr<UocInfo> puoci = UocInfo::srcList->elem(i);
    addCandidates(puoci->ast->child(0));
  }

  //for(size_t c=0; c < Options::entryPts.size(); c++)
  //  std::cerr << "Entry Point: " << Options::entryPts[c] << std::endl;
}
