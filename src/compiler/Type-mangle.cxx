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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include <libsherpa/UExcept.hxx>

#include "UocInfo.hxx"
#include "AST.hxx"
#include "Type.hxx"
#include "TypeInfer.hxx"
#include "TypeScheme.hxx"
#include "TypeMut.hxx"
#include "Typeclass.hxx"
#include "inter-pass.hxx"
#include "Unify.hxx"

using namespace boost;
using namespace sherpa;
using namespace std;

/* arg1: Ignore all mutability in name 
   arg2: Ignore TopLevel mutability and top-level mutability at 
         the (first) toplevel fn-arg-ret positions in name */
string
Type::mangledString(bool igMut, bool igTlMut, bool maxArgMut)
{
  // Reserved start characters: (Do not use)
  // "__" "VS"

  stringstream ss;
  
  if (getType() != shared_from_this())
    return getType()->mangledString(igMut, igTlMut, maxArgMut);

  if (mark & MARK_MANGLED_STRING) {
    // Encountered Infinite Type
    assert(false);
  }

  mark |= MARK_MANGLED_STRING;

  switch(typeTag) {
  case ty_tvar:
    {
      stringstream avss;
      avss << "'a" << uniqueID;

      ss << "_" << avss.str().size() << avss.str();
      break;
    }
    
  case ty_dummy:
    {
      ss << "D";
      break;
    }

  case ty_unit:
    ss << "_4unit";
    break;

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
    {
      const char *tn = typeTagName();
      ss << "_" << strlen(tn) << tn;
      break;
    }

#ifdef KEEP_BF
  case  ty_bitfield:
    ss << "BF" << Isize << CompType(0)->mangledString(igMut, false);
    break;
#endif

  case ty_method:
    ss << "H" 
       << Args()->mangledString(igMut, true,maxArgMut)
       << Ret()->mangledString(igMut, true, maxArgMut);
    break;

  case ty_fn:
    ss << "FN" 
       << Args()->mangledString(igMut, true,maxArgMut)
       << Ret()->mangledString(igMut, true, maxArgMut);

    break;

  case ty_fnarg:
    ss << components.size();
    for (size_t i=0; i<components.size(); i++) {
      if (CompFlags(i) & COMP_BYREF)
        ss << "Y";
      
      ss << CompType(i)->mangledString(igMut, true, 
                                       maxArgMut);
    }
    break;

  case ty_letGather:
    ss << "LG";
    for (size_t i=0; i<components.size(); i++) {
      ss << CompType(i)->mangledString(igMut, igTlMut, 
                                              maxArgMut);
    }
    
    break;

  case ty_structv:
  case ty_structr:
    {
      ss << "S"
         << ((typeTag == ty_structv) ? "V" : "R")
         << typeArgs.size() 
         << "_" << defAst->s.size() << defAst->s;

      for (size_t i=0; i < typeArgs.size(); i++)
        ss << TypeArg(i)->mangledString(igMut, false, 
                                         maxArgMut);
      break;
    }

  case ty_typeclass:    
    {
      ss << "TC"
         << "_" << defAst->s.size() << defAst->s;
      for (size_t i=0; i < typeArgs.size(); i++)
        ss << TypeArg(i)->mangledString(igMut, false, 
                                         maxArgMut);
      
      break;
    }

  case ty_uconv:
  case ty_uconr:
  case ty_uvalv:
  case ty_uvalr:
  case ty_unionv:
  case ty_unionr:
    {
      ss << "U"
         << (isRefType() ? "R" : "V")
         << typeArgs.size() 
         << "_" << myContainer->s.size() << myContainer->s;

      for (size_t i=0; i < typeArgs.size(); i++)
        ss << TypeArg(i)->mangledString(igMut, false, maxArgMut);
    }
    break;

  case ty_array:
    {
      ss << "J" << Base()->mangledString(igMut, false, maxArgMut)
         << "__" << arrLen->len;
      
      break;
    }
    
  case ty_vector:
    {
      ss << "K" << Base()->mangledString(igMut, false, maxArgMut);
      break;
    }

  case ty_ref:
    ss << "R" << Base()->mangledString(igMut, false, maxArgMut);
    break;

  case ty_byref:
    ss << "Z" << Base()->mangledString(igMut, false, maxArgMut);
    break;
    
  case ty_array_ref:
    ss << "W" << Base()->mangledString(igMut, false, maxArgMut);
    break;

  case ty_mutable:
    if (!igMut && !igTlMut)
      ss << "M";
    ss << Base()->mangledString(igMut, false, maxArgMut);
    break;
    
    // mangleString is called by routines beyond polyinstantiation,
    // at which point, the const-ness of normalized the type 
    // does not matter (const is really only necessary to 
    // constrain type variables.
  case ty_const:
    ss << Base()->mangledString(igMut, false, maxArgMut);
    break;
    
  case ty_exn:
    {
      string s = "exception";
      ss << "_" << s.size() << s;
      //ss << "X" << defAst->s.size() << defAst->s;
      break;
    }
    
  case ty_mbFull:
  case ty_mbTop:
    {
      ss << Core()->mangledString(igMut, false, maxArgMut);
      break;
    }

  case ty_tyfn:
  case ty_pcst:
  case ty_kvar:
  case ty_kfix:
  case ty_field:
    {
      assert(false);
      break;
    }
  }
  
  mark &= ~MARK_MANGLED_STRING;
  return ss.str();
}
