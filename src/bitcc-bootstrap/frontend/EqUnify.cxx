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
#include <iostream>
#include <string>
#include <sstream>
#include <libsherpa/UExcept.hxx>
#include <libsherpa/CVector.hxx>
#include <assert.h>
#include "UocInfo.hxx"
#include "Options.hxx"
#include "AST.hxx"
#include "Type.hxx"
#include "TypeScheme.hxx"
#include "Typeclass.hxx"
#include "inter-pass.hxx"
#include "TypeEqInfer.hxx"
#include "TypeInferCommon.hxx"

/* This file implements an equational unification algorithm that
   (partially) solves the constraints generated through the inference
   algorithm coded in TypeEqInfer.cxx */

/**********************************************************************/
/*                 Constraint Set (Transitive) Closure                */
/**********************************************************************/


// Note: This function uses pointer comparison for transitive closure.
// The only important case for closure is the constraints of the form
// 'a <: t and t <: 'a, for which this algorithm suffices.
// This algorithm will not close constraints of the form:
// tx <: t1 -> t2 , t1 -> t2 <: ty

// Returns true if the constraint set has changed, otherwise false.
bool
TransClose(GCPtr<Constraints> cset)
{
  size_t start = cset->pred->size();
  size_t pass_start=0;
  do {
    pass_start = cset->pred->size();
    
    for(size_t i=0; i < cset->size(); i++) {
      GCPtr<Constraint> cti = cset->Pred(i)->getType();
      if(cti->kind != ty_subtype)
	continue;
      
      for(size_t j=i+1; j < cset->size(); j++) {
	GCPtr<Constraint> ctj = cset->Pred(j)->getType();
	if(ctj->kind != ty_subtype)
	  continue;
	
	// If we have cti = a <: b and ctj = b <: c, then add a <: c
	if(cti->CompType(1) == ctj->CompType(0))
	  addSubCst(cti->ast, cti->CompType(0), 
		    ctj->CompType(1), cset);

	// If we have cti = a <: b and ctj = c <: a, then add c <: a
	if(cti->CompType(0) == ctj->CompType(1))
	  addSubCst(ctj->ast, ctj->CompType(0), 
		    cti->CompType(1), cset);
      }
    }
  } while(cset->pred->size() > pass_start);
  
  size_t norm = cset->pred->size();
  cset->normalize();
  
  size_t final = cset->pred->size();
  return ((start != norm) || (norm != final));
}

#define CMPSET(var, val) \
  do {			 \
    if(var != true)	 \
      var = val;	 \
  } while(0);		 


bool
EqUnify(std::ostream& errStream, GCPtr<Constraints> cset, 
	GCPtr<Trail> trail)
{
  bool cset_changed = false;
  bool errFree = true;
  do {
    
    CMPSET(cset_changed, TransClose(cset));
    for(size_t i=0; i < cset->size(); i++) {
      /* U({}) */
      // Implementation is implicit
      
      GCPtr<Constraint> ct = cset->Pred(i)->getType();
      if(ct->flags & CT_REMOVE)
	continue;

      switch(ct->kind) {
      case ty_subtype:
	{
	  GCPtr<Type> lhs = ct->CompType(0)->getType();
	  GCPtr<Type> rhs = ct->CompType(1)->getType();
	  
	  /* U('a <: 'a) 
	     U(unit <: unit) 
	     U(bool <: bool) 	 
	     U(int <: int),  ... etc */
	  if(lhs == rhs) {
	    ct->flags |= CT_REMOVE;
	    break;
	  }

	  /* U(Mt <: t) */
	  if((lhs->kind == ty_mutable) &&
	     (lhs->CompType(0)->getType() == rhs)) {
	    ct->flags |= CT_REMOVE;
	  }

	  /* U('a <: t, t <: 'a) */
	  if(lhs->kind == ty_tvar) {
	    GCPtr<Type> reverse = new Constraint(ty_subtype, ct->ast,
						 rhs, lhs);
	    if(cset->contains(reverse)) {
	      ct->flags |= CT_REMOVE;
	      trail->subst(lhs, rhs);
	    }
	  }
	  
	  /* U(k = x) */
	  // This case is implicit. We don't generate k=x constraints, 
	  // but perform the substitution immediately.

	  /* U('a < t), t = maxz(t) */
	  if((lhs->kind == ty_tvar) && rhs->isMaxMutable()) {
	    ct->flags |= CT_REMOVE;
	    trail->subst(lhs, rhs);
	  }
	  
	  /* U(t < 'a), t = minz(t), forall 'b, t != 'b */
	  if((lhs->kind == ty_tvar) && (rhs->kind != ty_tvar) &&
	     rhs->isMinMutable()) {
	    ct->flags |= CT_REMOVE;
	    trail->subst(lhs, rhs);
	  }
	  break;
	}
	
      case ty_pcst:
	{
	  break;
	}

      default:
	{
	  assert(false);
	  break;
	}
      }
    }
  } while(cset_changed);
  
  return errFree;
}