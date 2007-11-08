#ifndef TYPESCHEME_HXX
#define TYPESCHEME_HXX

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

#include <stdlib.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <string>
#include <libsherpa/UExcept.hxx>
#include <libsherpa/avl.hxx>
#include "AST.hxx"
#include "Type.hxx"
#include "Typeclass.hxx"
#include "INOstream.hxx"

struct TypeScheme : public Countable {

  GCPtr<Type> tau;
  GCPtr<AST> ast; // Need to maintained the official version here,
            // Type pointers get linked, typeschemes don't
  GCPtr<CVector<GCPtr<Type> > > ftvs;
   

  // Type class constraints 
  // Note: Leave this as a pointer, not an embedded vector.
  // This helps tcc sharing for definitions that are 
  // defined within the same letrec, etc.
  GCPtr<TCConstraints> tcc;
  
  //trivial_TypeScheme
  TypeScheme(GCPtr<Type> ty,
      GCPtr<TCConstraints> _tcc = NULL);

  TypeScheme(GCPtr<Type> ty, GCPtr<AST> ast,
	     GCPtr<TCConstraints> _tcc = NULL);

  // The generalizer
  bool generalize(std::ostream& errStream,
		  LexLoc &errLoc,
		  GCPtr<const Environment<TypeScheme> > gamma,
		  GCPtr<const Environment<CVector<GCPtr<Instance> > > >instEnv, 
		  GCPtr<const AST> expr, 
		  GCPtr<TCConstraints >parentTCC,
		  GCPtr<Trail> trail,
		  const bool topLevel,
		  const bool VRIsError,
		  const bool mustSolve);

  // The function that actually makes a copy of the 
  // contained type. This function calls TypeSpecialize.
  // This function is called by type_instance()
  // and getDCopy() function in Type class.
  GCPtr<Type> type_instance_copy() const;

  // Type Instance, if there are no ftvs, returns the original tau  
  GCPtr<Type> type_instance() const;
  
  // Type scheme's instance (including TC constraints)
  GCPtr<TypeScheme> ts_instance() const;
  GCPtr<TypeScheme> ts_instance_copy() const;

  // Read carefully:
  // Appends constraints that corrrespond to at least one
  // free variable in this scheme's ftvs to _tcc
  void addConstraints(GCPtr<TCConstraints> _tcc) const;

  //GCPtr<Type> type_copy();
  std::string asString(GCPtr<TvPrinter> tvP = new TvPrinter) const;
  void asXML(GCPtr<TvPrinter> tvP, INOstream &out) const;
  std::string asXML(GCPtr<TvPrinter> tvP = new TvPrinter) const;

  // Collect all tvs wrt tau, and tcc->pored, but NOT tcc->fnDeps
  void collectAllFtvs();
  void collectftvs(GCPtr<const Environment<TypeScheme> > gamma);

  bool solvePredicates(std::ostream &errStream,
		       LexLoc &errLoc,
		       GCPtr<const Environment< CVector<GCPtr<Instance> > > > instEnv);  

  /* Note this is different from adjMaybe method in Type struct.
     This dunction calls that function on `tau' and calls 
     clearHints on all predicates 
     (by calling clearHintsOnPreds method of `tcc') */
  void adjMaybes(GCPtr<Trail> trail);

private:
  void markRigid(std::ostream &errStream, LexLoc &errLoc);

public:
  /* FIX: THIS MUST NEVER BE USED IN lhs OF ASSIGNMENT! */

  /* PUBLIC Accessors (Convenience Forms) */
  GCPtr<Type> & Ftv(size_t i)
  {
    return (*ftvs)[i];
  }  
  GCPtr<Type>  Ftv(size_t i) const
  {
    GCPtr<Type> t = (*ftvs)[i];
    return t;
  }  
};

#endif /* TYPESCHEME_HXX */


