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
#include <errno.h>
#include <sstream>

#include "UocInfo.hxx"
#include "AST.hxx"
#include "Type.hxx"
#include "TypeInfer.hxx"
#include "inter-pass.hxx"

using namespace sherpa;
using namespace std;


void
markTail(GCPtr<AST> ast, GCPtr<AST> fn, GCPtr<AST> bps, bool isTail)
{
  //std::cout << ast->astTypeName() << ": " << isTail << std::endl;

  switch(ast->astType) {
    
  case at_Null:
    break;

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
  case at_letbindings:
  case at_letbinding:
  case at_dobindings:
  case at_dobinding:
  case agt_CompilationUnit:
  case at_ifident:
  case at_localFrame:
  case at_frameBindings:
  case agt_tc_definition:
  case agt_if_definition:
  case agt_category:
  case agt_ow:
  case agt_qtype:
  case agt_fielditem:
  case at_refCat:
  case at_valCat:
  case at_opaqueCat:
  case at_tcdecls:
  case at_tyfn:
  case at_tcapp:
  case at_method_decls:
  case at_method_decl: 
  case at_usesel:
  case at_use_case:
  case at_letGather:
  case agt_ucon:
    {
      assert(false);
      break;
    } 
    
  case at_unit:
  case at_boolLiteral:
  case at_charLiteral:
  case at_intLiteral:
  case at_floatLiteral:
  case at_stringLiteral:
  case at_bitfield:
  case at_deftypeclass:
  case at_definstance:
  case at_methods:
  case at_declunion:
  case at_declstruct:
  case at_declrepr:
  case at_use:
  case at_import:
  case at_provide:
  case at_from:
  case at_ifsel:
  case at_declares:
  case at_declare:
  case at_tvlist:
  case at_typeapp:
  case at_arrayType:
  case at_vectorType:
  case at_refType:
  case at_byrefType:
  case at_exceptionType:
  case at_dummyType:
  case at_valType:
  case at_primaryType:
  case at_fnargVec:
  case at_mutableType: 
  case at_defunion:
  case at_defstruct:
  case at_constructors:
  case at_fields:
  case at_constructor:
  case at_field:
  case at_fill:
  case at_reserved:
  case at_defexception:
  case at_proclaim:
  case at_fn:
  case at_identPattern:
  case at_lambda:
  case at_argVec:
  case at_qualType:
  case at_constraints:
  case at_identList:
  case at_defrepr:
    //case at_reprbody:
    //case at_reprcase:
    //case at_reprcaselegR:
    //case at_reprtag:
    //case agt_reprbodyitem:
  case at_reprctrs:
  case at_reprctr:
  case at_reprrepr:
  case at_docString:
    {
      break;
    }

  case at_ident:
    {
      if(ast->symbolDef == fn && isTail) {
	//std::cout << "Marked " << ast->s << " at " << ast->loc << endl; 
	ast->Flags |= (SELF_TAIL);
      }
      
      break;
    }

  case at_container:
    {
      markTail(ast->child(1), fn, bps, isTail);
      break;
    }

  case at_interface:
  case at_module:
    {
      size_t c = (ast->astType == at_module)?0:1;
      for(; c < ast->children->size(); c++)
	markTail(ast->child(c), fn, bps, isTail); 

      break;
    }

  case at_define:
    {
      if(ast->child(1)->astType == at_lambda) {
	ast->child(0)->child(0)->defbps = ast->child(1)->child(0);
	markTail(ast->child(1)->child(1), 
		 ast->child(0)->child(0), 
		 ast->child(1)->child(0), true); 
      }
      break;
    }
    
  case at_suspend:
    {
      markTail(ast->child(1), fn, bps, isTail); 
      break;
    }

  case at_tqexpr:
    {
      markTail(ast->child(0), fn, bps, isTail); 
      break;
    }
    
  case at_begin:
    {
      //     for (size_t i = 0; i < ast->children->size() - 1; i++)
      //       markTail(ast->child(i), fn, bps, false);
      markTail(ast->child(ast->children->size() - 1), fn, bps, isTail);
      break;
    }
    
  case at_do:
    {
      markTail(ast->child(1), fn, bps, isTail);
      break;
    }
    
  case at_dotest:
    {
      markTail(ast->child(1), fn, bps, isTail);
      break;
    }
    
  case at_fqCtr:
  case at_sel_ctr:    
  case at_select:
    {
      //markTail(ast->child(0), fn, bps, false); 
      break;
    }

  case at_if:
    {
      //markTail(ast->child(0), fn, bps, false);
      markTail(ast->child(1), fn, bps, isTail);
      markTail(ast->child(2), fn, bps, isTail);
      break;
    }

  case at_when:
    {
      //markTail(ast->child(0), fn, bps, false);
      markTail(ast->child(1), fn, bps, isTail);
      break;
    }

  case at_apply:
    {
      markTail(ast->child(0), fn, bps, isTail);
      //       for (size_t i = 1; i < ast->children->size(); i++)
      // 	markTail(ast->child(i), fn, bps, false);
	
      break;
    }

  case at_ucon_apply:
  case at_struct_apply:
    {
      //       for(size_t c = 1; c < ast->children->size(); c++)
      // 	markTail(ast->child(i), fn, bps, false);
      
      break;
    }

  case at_allocREF:
  case at_and:
  case at_or:
  case at_not:
  case at_dup:
  case at_deref:
  case at_inner_ref:
  case at_array_length:
  case at_vector_length:
  case at_array_nth:
  case at_vector_nth:
  case at_vector:
  case at_array:
  case at_makevectorL:    
  case at_setbang:
  case at_mkClosure:
    {
      break;
    }

  case at_cond:
  case at_cond_legs:
  case at_sw_legs:
  case at_otherwise:
  case at_throw:
  case at_setClosure:
  case at_copyREF:
    {
      for(size_t c = 0; c < ast->children->size(); c++)
	markTail(ast->child(c), fn, bps, isTail);
      break;
    }

  case at_switch:
  case at_try:
    {
      for(size_t c = 0; c < ast->children->size(); c++)
	if(c != IGNORE(ast))
	  markTail(ast->child(c), fn, bps, isTail);
      break;
    }

  case at_cond_leg:
    {
      //markTail(ast->child(0), fn, bps, false);
      markTail(ast->child(1), fn, bps, isTail);
      break;
    }

  case at_sw_leg:
    {
      markTail(ast->child(1), fn, bps, isTail);
      break;
    }

  case at_let:
  case at_letrec:
  case at_letStar:
    {
      //       GCPtr<AST> lbs = ast->child(0);
      //       for(size_t c = 0; c < lbs->children->size(); c++) {
      // 	GCPtr<AST> lb = lbs->child(c);
      // 	markTail(lb->child(1), fn, bps, false);
      //       }
      markTail(ast->child(1), fn, bps, isTail);
      break;
    }    
  }  
}


bool
UocInfo::be_tail(std::ostream& errStream,
		 bool init, unsigned long flags)
{
  bool errFree = true;

  GCPtr<UocInfo> uoc = this;
  
  markTail(uoc->uocAst, NULL, NULL, true);
  return errFree;
}