/**************************************************************************
 *
 * Copyright (C) 2008, Johns Hopkins University.
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

/* In order to successfully polyinstantiate instances that contain
   immediate lambdas, we need to hoist them and give them proper
   names so that the polyinstantiator has something to mangle. */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <libsherpa/UExcept.hxx>

#include "UocInfo.hxx"
#include "AST.hxx"
#include "Type.hxx"
#include "TypeInfer.hxx"
#include "inter-pass.hxx"

using namespace boost;
using namespace sherpa;

static void
cl_HoistInstLam(shared_ptr<UocInfo> uoc)
{
  std::vector<shared_ptr<AST> > outAsts;

  shared_ptr<AST> modOrIf = uoc->uocAst;

  for (size_t c = 0;c < modOrIf->children.size(); c++) {
    shared_ptr<AST> child = modOrIf->child(c);

    if (child->astType == at_definstance) {
      shared_ptr<AST> methods = child->child(1);

      // FIX: This is **utterly failing** to hoist the constraints, so
      // any constraint that is not on an instantiated variable will
      // not do the right thing.
      for (size_t m = 0; m < methods->children.size(); m++) {
        shared_ptr<AST> method = methods->child(m);
        shared_ptr<AST> methodValue = method->child(1);
        
        if (methodValue->astType != at_ident) {
          // It's an expression. Since instance definitions consist
          // entirely of procedure bindings/mappings, and we know this
          // type checked, it must be a lambda. Need to hoist it into
          // a top-level binding and replace the expression occurrence
          // with the resulting identifier.
          shared_ptr<AST> newDef = AST::make(at_define, methodValue->loc);

          shared_ptr<AST> lamName = AST::genSym(methodValue, "lam");
          lamName->identType = id_value;
          lamName->flags |= ID_IS_GLOBAL;

          shared_ptr<AST> lamPat = AST::make(at_identPattern,
                                             methodValue->loc, lamName); 
          newDef->addChild(lamPat);
          newDef->addChild(methodValue);
          newDef->addChild(AST::make(at_constraints));

          outAsts.push_back(newDef);

          shared_ptr<AST> instName = lamName->Use();
          shared_ptr<AST> the = AST::make(at_typeAnnotation);
          the->addChild(instName);
          the->addChild(methodValue->symType->asAST(methodValue->loc));

          method->child(1) = the;
        }
      }
    }

    outAsts.push_back(child);
  }

  modOrIf->children = outAsts;
}

#if 0
bool
UocInfo::be_HoistInstLam(std::ostream& errStream,
                         bool init, unsigned long flags)
{
  bool errFree = true;

  shared_ptr<AST> &ast = UocInfo::linkedUoc.ast;

  DEBUG(ILH) if (isSourceUoc)
    BitcPP(errStream, &UocInfo::linkedUoc);

  DEBUG(ILH) std::cerr << "cl_HoistInstLam" << std::endl;
  cl_HoistInstLam(&UocInfo::linkedUoc);

  DEBUG(ILH) if (isSourceUoc)
    BitcPP(errStream, &UocInfo::linkedUoc);

  DEBUG(ILH) std::cerr << "RandT" << std::endl;
  // Re-run the type checker to propagate the changes:
  CHKERR(errFree, RandT(errStream, &UocInfo::linkedUoc, true, POLY_SYM_FLAGS, POLY_TYP_FLAGS));
  assert(errFree);

  return true;
}
#else
bool
UocInfo::fe_HoistInstLam(std::ostream& errStream,
                         bool init, unsigned long flags)
{
  bool errFree = true;

  DEBUG(ILH) if (isSourceUoc())
    PrettyPrint(errStream);

  DEBUG(ILH) std::cerr << "cl_HoistInstLam" << std::endl;
  cl_HoistInstLam(shared_from_this());
  
  DEBUG(ILH) if (isSourceUoc())
    PrettyPrint(errStream);
  
  DEBUG(ILH) std::cerr << "RandT" << std::endl;
  // Re-run the type checker to propagate the changes:
  CHKERR(errFree, RandT(errStream, true, PI_SYM_FLAGS, PI_TYP_FLAGS));
  assert(errFree);

  return true;
}
#endif
