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

#include "AST.hxx"
#include "inter-pass.hxx"

using namespace boost;
using namespace sherpa;

/// @brief Remove redundant BEGIN expressions.
///
/// This simplification pass was originally intended to locate all
/// cases where a BEGIN wraps a single expression and replaces this
/// with the single expression. The transform has no semantic effect,
/// but improves the readability of dumped ASTs. This is okay even in
/// the block syntax, because in any context where the block is
/// mandatory it will be re-inserted by layout.
///
/// Later, functionality was added by which local DEFINE forms are
/// converted to the corresponding LET and LETREC forms. After this
/// pass, the only remaining occurrences of DEFINE and RECDEF appear at
/// top level. The implementations of most later passes, notably the
/// resolver and symbol checker, rely on this.
///
/// All of the syntactic constructs that wrap implicit BEGIN blocks
/// are converted by the parser to wrap a single expression with an
/// inserted BEGIN block that is marked as compiler inserted. In
/// consequence, the transforms performed here apply to those
/// constructs as well.
///
/// This pass may now be misnamed, but all of the simplifications that
/// it performs are vaguely related to begin blocks.
static shared_ptr<AST>
beginSimp(shared_ptr<AST> ast, std::ostream& errStream, bool &errFree)
{
  for (size_t c = 0; c < ast->children.size(); c++)
    ast->child(c) = beginSimp(ast->child(c), errStream, errFree);

  if (ast->astType == at_begin) {
    // Note that the execution of this loop can have the effect of
    // shortening ast->children.size()!
    for (size_t c = 0; c < ast->children.size(); c++) {
      if (ast->child(c)->astType == at_define ||
          ast->child(c)->astType == at_recdef) {
        bool rec = (ast->child(c)->astType == at_recdef);
        shared_ptr<AST> def = ast->child(c);

        // at_usesel is not allowed to appear in a defining occurrence
        // of a local at_define or at_recdef.
        //
         // It might be better to catch this in the parser.
        if (def->child(0)->child(0)->astType == at_usesel) {
          errStream << def->loc << ": "
                    << "Hygienically aliased names cannot be defined locally."
                    << std::endl;
          errFree = false;
        }

        shared_ptr<AST> letBinding =
          AST::make(at_letbinding,
                  def->loc, def->child(0), def->child(1));
        if (rec)
          letBinding->flags |= LB_REC_BIND;
        
        // The definition is not a global
        def->child(0)->child(0)->flags &= ~ID_IS_GLOBAL;
        shared_ptr<AST> body = AST::make(at_begin, ast->child(c)->loc);
        
        for (size_t bc = c+1; bc < ast->children.size(); bc++)
          body->addChild(ast->child(bc));
        
        if (body->children.size() == 0) {
          errStream << def->loc << ": "
                    << "definition not permitted as the "
                    << "last expression in a sequece."
                    << std::endl;
          errFree = false;
        }
        
        // Trim the remaining children of this begin:
        // while (ast->children.size() > c+1)
        //   ast->children->Remove(ast->children.size()-1);
        std::vector<shared_ptr<AST> > newChildren;
        for (size_t i=0; i <= c; i++)
          newChildren.push_back(ast->child(i));
        ast->children = newChildren;
        
        // Insert the new letrec:
        shared_ptr<AST> theLetRec =
          AST::make((rec ? at_letrec : at_let), def->loc,
                  AST::make(at_letbindings, def->loc, letBinding),
                  body, def->child(2));
        ast->child(c) = beginSimp(theLetRec, errStream, errFree);

        // Stop processing this begin:
        break;
      }
    }
  }

  if (ast->astType == at_begin && ast->children.size() == 1)
    return ast->child(0);

  return ast;
}

bool
UocInfo::fe_beginSimp(std::ostream& errStream,
                      bool init, unsigned long flags)
{
  DEBUG(BEG_SIMP) if (isSourceUoc())
    PrettyPrint(errStream);

  DEBUG(BEG_SIMP) std::cerr << "fe_beginSimp" << std::endl;
  bool errFree = true;
  uocAst = beginSimp(uocAst, errStream, errFree);

  DEBUG(BEG_SIMP) if (isSourceUoc())
    PrettyPrint(errStream);

  return true;
}
