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

/** @file
 *
 * @brief Command line driver for the static, whole-program BitC
 * compiler.
 *
 * There are three input file extensions:
 *
 *   .bitc  - bitc interface files
 *   .bits  - bitc source files
 *   .bito  - bitc "object" files.
 *
 * This compiler basically operates in three modes:
 *
 *  1. source->object mode, in which X.bits is checked and re-emitted
 *     as X.bito, which is a legal BitC file. We pretend that such
 *     files are object files for command line handling purposes.
 *
 *     This mode can be identified by the presence of the -c option on
 *     the command line. If -c is specified, no output will be emitted.
 *
 *  2. As a header file synthesizer, in which an interface file is
 *     re-emitted as a C header file, allowing portions of the
 *     low-level runtime to be implemented in C.
 *
 *     This usage can be identified by the presence of the -h option
 *     on the command line, but has no effect if -c is also specified.
 *
 *     Note that -h is a convenience shorthand for --lang h.
 *
 *  3. As a linker, in which some number of .bits and .bito files are
 *     combined to form an executable. This is actually the
 *     whole-program compiler mode.
 *
 *     This mode can be identified by the @em absence of either the -c
 *     or the -h options on the command line.
 */

#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>


#include <fstream>
#include <iostream>

#include <getopt.h>
#include <langinfo.h>

#include <libsherpa/Path.hxx>
#include <libsherpa/util.hxx>

#include "Version.hxx"
#include "UocInfo.hxx"
#include "Options.hxx"
#include "AST.hxx"
#include "backend.hxx"
#include "INOstream.hxx"
#include "Instantiate.hxx"
#include "TvPrinter.hxx"
using namespace sherpa;
using namespace std;

#define BITC_COMPILER_MODE        0x1u
#define BITC_INTERPRETER_MODE     0x2u
#define BITC_CURRENT_MODE BITC_COMPILER_MODE

bool Options::showParse = false;
bool Options::showTypes = false;
bool Options::xmlTypes = false;
bool Options::showLex = false;
bool Options::useStdInc = true;
bool Options::useStdLib = true;
bool Options::advisory = false;
bool Options::rawTvars = false;
bool Options::showMaybes = false;
bool Options::FQtypes = true;
bool Options::showAllTccs = false;
bool Options::showPasses = false;
bool Options::noPrelude = false; 
bool Options::dumpAfterMidEnd = false;
bool Options::dumpTypesAfterMidEnd = false;
GCPtr<CVector<std::string> > Options::showTypesUocs;
GCPtr<CVector<std::string> > Options::xmlTypesUocs;
bool Options::ppFQNS = false;
bool Options::ppDecorate = false;
unsigned Options::verbose = 0;
GCPtr<CVector<std::string> > Options::entryPts;
BackEnd *Options::backEnd = 0;
std::string Options::outputFileName;
GCPtr<CVector<GCPtr<Path> > > Options::libDirs;
GCPtr<CVector<std::string> > Options::inputs;
bool Options::Wall = false;
bool Options::noGC = false;
bool Options::noAlloc = false;
GCPtr<TvPrinter> Options::debugTvP = new TvPrinter;
bool Options::heuristicInference = false;

GCPtr<CVector<std::string> > Options::LinkPreOptionsGCC;
GCPtr<CVector<std::string> > Options::CompilePreOptionsGCC;
GCPtr<CVector<std::string> > Options::LinkPostOptionsGCC;

GCPtr<CVector<std::string> > Options::SystemDirs;

#define LOPT_SHOWLEX      257   /* Show tokens */
#define LOPT_SHOWPARSE    258   /* Show parse */
#define LOPT_DUMPAFTER    259   /* PP after this pass */
#define LOPT_SHOWPASSNMS  260   /* Show all pass names */
#define LOPT_NOSTDINC     261   /* Do not append std search paths */
#define LOPT_NOSTDLIB     262   /* Do not append std lib paths */
#define LOPT_ADVISORY     263   /* Show advisory information */
#define LOPT_RAW_TVARS    264   /* Show tvars as is */
#define LOPT_FQ_TYPES     265   /* Show fully qualified types */
#define LOPT_SA_TCC       266   /* Show all Type class constraints */
#define LOPT_SHOWPASSES   267   /* Show passes as they are run */
#define LOPT_PPFQNS       268   /* Show FQNs when pretty printing */
#define LOPT_DUMPTYPES    269   /* Show types after this pass */
#define LOPT_STOPAFTER    270   /* Stop after this pass */
#define LOPT_PPDECORATE   271   /* Decorate pretty printing with types */
#define LOPT_SHOW_MAYBES  272   /* Show all maybe wrapper types, hints */
#define LOPT_SHOW_TYPES   273   /* Dump types a particular uoc only */
#define LOPT_EQ_INFER     274   /* Equational (Complete) Type Inference */
#define LOPT_XML_TYPES    275   /* Dump XML types */
#define LOPT_NOGC         276   /* NO GC mode */
#define LOPT_NOPRELUDE    277   /* Don't process prelude */
#define LOPT_DBG_TVARS    278   /* Show globally unqiue tvar naming */
#define LOPT_HEURISTIC    279   /* Use Heuristic Inference */
#define LOPT_HELP         280   /* Display usage information. */
#define LOPT_EMIT         281   /* Specify the desired output
				   language. */
#define LOPT_SYSTEM       282   /* Specify the desired output
				   language. */
#define LOPT_NOALLOC      283   /* Statically reject heap-allocating
				   operations and constructs */

struct option longopts[] = {
  /*  name,           has-arg, flag, val           */
  { "debug-tvars",          0,  0, LOPT_DBG_TVARS },
  { "decorate",             0,  0, LOPT_PPDECORATE },
  { "dumpafter",            1,  0, LOPT_DUMPAFTER },
  { "dumptypes",            1,  0, LOPT_DUMPTYPES },
  { "emit",                 1,  0, LOPT_EMIT },
  { "eqinfer",              0,  0, LOPT_EQ_INFER },
  { "free-advice",          0,  0, LOPT_ADVISORY },
  { "full-qual-types",      0,  0, LOPT_FQ_TYPES },
  { "help",                 0,  0, LOPT_HELP },
  { "heuristic-inf",        0,  0, LOPT_HEURISTIC },
  { "no-alloc",             0,  0, LOPT_NOALLOC },
  { "no-gc",                0,  0, LOPT_NOGC },
  { "no-prelude",           0,  0, LOPT_NOPRELUDE },
  { "nostdinc",             0,  0, LOPT_NOSTDINC },
  { "nostdlib",             0,  0, LOPT_NOSTDLIB },
  { "ppfqns",               0,  0, LOPT_PPFQNS },
  { "raw-tvars",            0,  0, LOPT_RAW_TVARS },
  { "show-all-tccs",        0,  0, LOPT_SA_TCC },
  { "show-maybes",          0,  0, LOPT_SHOW_MAYBES },
  { "showlex",              0,  0, LOPT_SHOWLEX },
  { "showparse",            0,  0, LOPT_SHOWPARSE },
  { "showpasses",           0,  0, LOPT_SHOWPASSES },
  { "showpassnames",        0,  0, LOPT_SHOWPASSNMS },
  { "showtypes",            1,  0, LOPT_SHOW_TYPES },
  { "stopafter",            1,  0, LOPT_STOPAFTER },
  { "system",               1,  0, LOPT_SYSTEM },
  { "verbose",              0,  0, 'v' },
  { "version",              0,  0, 'V' },
  { "xmltypes",             1,  0, LOPT_XML_TYPES },
#if 0
  /* Options that have short-form equivalents: */
  { "debug",                0,  0, 'd' },

  { "dispatchers",          0,  0, 's' },
  { "entry",                1,  0, 'e' },
  { "execdir",              1,  0, 'X' },
  { "header",               1,  0, 'h' },
  { "include",              1,  0, 'I' },
  { "index"  ,              1,  0, 'n' },
  { "outdir",               1,  0, 'D' },
  { "output",               1,  0, 'o' },
  { "verbose",              0,  0, 'v' },
#endif
  {0,                       0,  0, 0}
};

void
help()
{
  std::cerr 
    << "Common Usage:" << endl
    //    << "  bitcc [-I include] -c file1.bits ...\n"
    << "  bitcc [-I include] [-o outfile.bito] -c file.bits\n"
    // << "  bitcc [-I include] -h file.bitc ...\n"
    << "  bitcc [-I include] [-o outfile.h] -h file.bits\n"
    << "  bitcc [file1.bito|library.a] ... [-o exefile]\n"
    << "  bitcc -V|--version\n"
    << "  bitcc --help\n"
    << "\n"
    << "Debugging options:\n"
    << "  --showlex --showparse --showpasses\n"
    << "  --stopafter 'pass'\n"
    << "  --decorate --dumpafter 'pass' --dumptypes 'pass'\n"
    << "  --showtypes 'uoc' \n"
    //<< "  --xmltypes 'uoc' \n"
    << "  --ppfqns --full-qual-types\n"
    << "  --raw-tvars --show-maybes --show-all-tccs \n"
    << "\n"
    << "Languages: xmlpp, xmldump, xmltypes, bitcpp, showtypes, bito,\n"
    << "           c, h, obj\n" 
    << flush;
}
 
void
fatal()
{
  cerr << "Confused due to previous errors, bailing."
       << endl;
  exit(1);
}


static bool SawFirstBitcInput = false;
void
AddLinkArgumentForGCC(const std::string& s)
{
  if (SawFirstBitcInput)
    Options::LinkPostOptionsGCC->append(s);
  else {
    Options::LinkPreOptionsGCC->append(s);
  }
}

void
AddCompileArgumentForGCC(const std::string& s)
{
  if (!SawFirstBitcInput)
    Options::CompilePreOptionsGCC->append(s);    
}

BackEnd *
FindBackEnd(const char *nm)
{
  for (size_t i = 0; i < BackEnd::nBackEnd; i++) {
    if (BackEnd::backends[i].name == nm)
      return &BackEnd::backends[i]; //&OK
  }

  return NULL;
}

void 
handle_sigsegv(int param)
{
  cerr << "Internal Compiler error: SIGSEGV. "
       << "Please report this problem along with sample source."
       << endl;
  exit(1);
}

bool
ResolveLibPath(std::string name, std::string& resolvedName)
{
  // We either saw -lname or -l name. What we need to do here is
  // check the currently known library paths for a resolution. If we
  // find a file matching "libname.bita" on the search path, we add it
  // to the list of inputs.
  //
  // In some cases, -lmumble will indicate simultaneously a need to
  // add an input file named ..../libmumble.bita and also an archive
  // library named .../libmumble.a. This arises in libbitc, for
  // example, where some of the library is implemented in C.
  //
  // Unfortunately, this means that the @em absence of
  // .../libmumble.bita does not reliably indicate an error.

  Path nmPath = Path("lib" + name + ".bita");

  for (size_t i = 0; i < Options::libDirs->size(); i++) {
    Path testPath = *Options::libDirs->elem(i) + nmPath;
    if (testPath.exists()) {
      resolvedName = testPath.asString();
      return true;
    }
  }

  return false;    
}

int
main(int argc, char *argv[]) 
{
  int c;
  //  extern int optind;
  int opterr = 0;
  bool userAddedEntryPts = false;

  // Allocate memory for some static members
  Options::showTypesUocs = new CVector<std::string>;
  Options::xmlTypesUocs = new CVector<std::string>;
  Options::entryPts = new CVector<std::string>;
  Options::libDirs = new CVector<GCPtr<Path> >;
  Options::inputs = new CVector<std::string>;
  Options::CompilePreOptionsGCC = new CVector<std::string>;
  Options::LinkPreOptionsGCC = new CVector<std::string>;
  Options::LinkPostOptionsGCC = new CVector<std::string>;
  Options::SystemDirs = new CVector<std::string>;
  UocInfo::searchPath = new CVector<GCPtr<Path> >;
  UocInfo::ifList = new CVector<GCPtr<UocInfo> >;
  UocInfo::srcList = new CVector<GCPtr<UocInfo> >;

  signal(SIGSEGV, handle_sigsegv);

#if 0
  // Shap thinks this is no longer necessary now that he gave up and
  // pulled in libicu.

  // Make sure that we are running in a UNICODE locale:
  setlocale(LC_ALL, "");
  if (strcmp("UTF-8", nl_langinfo(CODESET)) != 0) {
    std::cerr
      << "We appear to be running in the non-unicode locale "
      << nl_langinfo(CODESET)
      << ".\n"
      << "The BitC compiler will only operate correctly "
      << "in a unicode locale.\n";
    exit(1);
  }
#endif

  /// Note the "-" at the start of the getopt_long option string. In
  /// order to generate behavior that is compatible with other
  /// compilers (and linkers), we need to process library archives in
  /// strict left-to-right form, without regard to whether they appear
  /// in .../libfoo.a or -lbar format. That is: -lbar must be treated
  /// as if macro-expanded
  ///
  /// But it is possible to see things like:
  ///
  ///    bitc a.bito b.bito libfoo.a -L path -lbar -o out
  ///
  /// This  means that there is left-context sensitivity to
  /// consider, because the inputs are:
  ///
  ///    a.bito b.bito libfoo.a ResolveLibPath("bar")
  ///
  /// and the behavior of ResolveLibPath relies on having processed
  /// the -L options appearing to its left and NOT any -L options
  /// appearing to its right. To make matters even more fun, we have
  /// to selectively accumulate some of these options to be passed
  /// along to gcc, and given the interspersal we need to do so in an
  /// order-preserving way.

  while ((c = getopt_long(argc, argv, 
			  "-e:o:O::l:VvchI:L:",
			  longopts, 0
		     )) != -1) {
    switch(c) {
    case 1:
      {
	Path p(optarg);
	std::string sfx = p.suffix();

	if (
	    // Interface files. Probably should not appear on the
	    // command line, but this may be a case of obsolete
	    // usage. Allow it for now.
	    (sfx == ".bitc")
	    // BitC source file.
	    || (sfx == ".bits")
	    // BitC "object" file.
	    || (sfx == ".bito")
	    // BitC archive file.
	    || (sfx == ".bita")) {

	  SawFirstBitcInput = true;
	  Options::inputs->append(optarg);
	}
	else
	  // Else it is something to be passed through to GCC:
	  AddLinkArgumentForGCC(optarg);

	break;
      }

    case 'V':
      cerr << "Bitc Version: " << BITC_VERSION << endl;
      exit(0);
      break;

    case LOPT_NOSTDINC:
      Options::useStdInc = false;
      AddCompileArgumentForGCC("--nostdinc");
      AddLinkArgumentForGCC("--nostdinc");
      break;

    case LOPT_NOSTDLIB:
      Options::useStdLib = false;
      AddLinkArgumentForGCC("--nostdlib");
      break;

    case LOPT_SHOWPARSE:
      Options::showParse = true;
      break;

    case LOPT_SHOWLEX:
      Options::showLex = true;
      break;

    case LOPT_SHOWPASSES:
      Options::showPasses = true;
      break;

    case LOPT_PPFQNS:
      Options::ppFQNS = true;
      break;

    case LOPT_PPDECORATE:
      Options::ppDecorate = true;
      break;

    case LOPT_NOGC:
      Options::noGC = true;
      break;

    case LOPT_NOALLOC:
      Options::noAlloc = true;
      break;

    case LOPT_NOPRELUDE:
      Options::noPrelude = true;
      break;

    case LOPT_HEURISTIC:
      Options::heuristicInference=true;
      break;

    case LOPT_EQ_INFER:
      //Options::inferenceAlgorithm = inf_eq;
      // FIX: TEMPORARY
      //UocInfo::passInfo[pn_typeCheck].stopAfter = true;
      break;

    case LOPT_SHOWPASSNMS:
      {
	std::cerr.width(15);
	std::cerr << left 
		  << "PASS"
		  << "PURPOSE" << std::endl << std::endl;

	for (size_t i = (size_t)pn_none+1; i < (size_t) pn_npass; i++) {
	  std::cerr.width(15);
	  std::cerr << left 
		    << UocInfo::passInfo[i].name
		    << UocInfo::passInfo[i].descrip << std::endl;
	}

	for (size_t i = (size_t)op_none+1; i < (size_t) op_npass; i++) {
	  std::cerr.width(15);
	  std::cerr << left 
		    << UocInfo::onePassInfo[i].name
		    << UocInfo::onePassInfo[i].descrip << std::endl;
	}

	// FIX: What about the onepass passes? 
	exit(0);
      }

    case LOPT_DUMPAFTER:
      {
	for (size_t i = (size_t)pn_none+1; i < (size_t) pn_npass; i++) {
	  if (strcmp(UocInfo::passInfo[i].name, optarg) == 0 ||
	      strcmp("ALL", optarg) == 0)
	    UocInfo::passInfo[i].printAfter = true;
	}

	for (size_t i = (size_t)op_none+1; i < (size_t) op_npass; i++) {
	  if (strcmp(UocInfo::onePassInfo[i].name, optarg) == 0 ||
	      strcmp("ALL", optarg) == 0)
	    UocInfo::onePassInfo[i].printAfter = true;
	}
	
	if (strcmp("midend", optarg) == 0 ||
	    strcmp("ALL", optarg) == 0) 
	  Options::dumpAfterMidEnd = true;
 
	break;
      }

    case LOPT_DUMPTYPES:
      {
	for (size_t i = (size_t)pn_none+1; i < (size_t) pn_npass; i++) {
	  if (strcmp(UocInfo::passInfo[i].name, optarg) == 0 ||
	      strcmp("ALL", optarg) == 0)
	    UocInfo::passInfo[i].typesAfter = true;
	}

	for (size_t i = (size_t)op_none+1; i < (size_t) op_npass; i++) {
	  if (strcmp(UocInfo::onePassInfo[i].name, optarg) == 0 ||
	      strcmp("ALL", optarg) == 0)
	    UocInfo::onePassInfo[i].typesAfter = true;
	}
	
	if (strcmp("midend", optarg) == 0 ||
	    strcmp("ALL", optarg) == 0) 
	  Options::dumpTypesAfterMidEnd = true;

	break;
      }

    case LOPT_STOPAFTER:
      {
	for (size_t i = (size_t)pn_none+1; i < (size_t) pn_npass; i++) {
	  if (strcmp(UocInfo::passInfo[i].name, optarg) == 0 ||
	      strcmp("ALL", optarg) == 0)
	    UocInfo::passInfo[i].stopAfter = true;
	}

	for (size_t i = (size_t)op_none+1; i < (size_t) op_npass; i++) {
	  if (strcmp(UocInfo::onePassInfo[i].name, optarg) == 0 ||
	      strcmp("ALL", optarg) == 0)
	    UocInfo::onePassInfo[i].stopAfter = true;
	}

	break;
      }

    case LOPT_ADVISORY:
      {
	Options::advisory = true;
      }      

    case LOPT_RAW_TVARS:
      {
	Options::rawTvars = true;
	break;
      }

    case LOPT_SHOW_MAYBES:
      {
	Options::showMaybes = true;
	break;
      }
      
    case LOPT_FQ_TYPES:
      {
	Options::FQtypes = true;
	break;
      }

    case LOPT_SA_TCC:
      {
	Options::showAllTccs = true;
	break;
      }

    case LOPT_SHOW_TYPES:
      {
	if(!Options::showTypesUocs->contains(optarg))
	  Options::showTypesUocs->append(optarg);
	break;
      }

    case LOPT_XML_TYPES:
      {
	if(!Options::xmlTypesUocs->contains(optarg))
	  Options::xmlTypesUocs->append(optarg);
	break;
      }

    case 'v':
      Options::verbose++;
      if (Options::verbose > 1) {
	AddCompileArgumentForGCC("-v");
	AddLinkArgumentForGCC("-v");
      }
      break;

    case 'c':
      {
	if (Options::backEnd) {
	  std::cerr << "Can only specify one output language.\n";
	  exit(1);
	}

	// Issue: if we are passed a .c file, shouldn't we pass that
	// along to GCC in this case? Problem: what if there aren't
	// any inputs exclusively for GCC?
	//
	// AddArgumentForGCC("-c");

	Options::backEnd = FindBackEnd("bito");
	break;
      }

    case 'h':
      {
	if (Options::backEnd) {
	  std::cerr << "Can only specify one output language.\n";
	  exit(1);
	}

	Options::backEnd = FindBackEnd("h");
	break;
      }

    case LOPT_EMIT:
      {
	if (Options::backEnd) {
	  std::cerr << "Can only specify one output language.\n";
	  exit(1);
	}

	Options::backEnd = FindBackEnd(optarg);
	if (!Options::backEnd) {
	  std::cerr << "Unknown target language.\n";
	  exit(1);
	}

	break;
      }

    case 'e':
      {	
	userAddedEntryPts = true;
	Options::entryPts->append(optarg);
	break;
      }

    case LOPT_HELP: 
      {
	help();
	exit(0);
      }

    case 'o':
      Options::outputFileName = optarg;

      AddLinkArgumentForGCC("-o");
      AddLinkArgumentForGCC(optarg);

      break;

    case 'I':
      AddCompileArgumentForGCC("-I");
      AddCompileArgumentForGCC(optarg);
      AddLinkArgumentForGCC("-I");
      AddLinkArgumentForGCC(optarg);
      UocInfo::searchPath->append(new Path(optarg));
      break;

    case LOPT_SYSTEM:
      Options::SystemDirs->append(optarg);
      break;

    case 'l':
      {
	AddLinkArgumentForGCC("-l");
	AddLinkArgumentForGCC(optarg);

	std::string resolvedName;
	if (ResolveLibPath(optarg, resolvedName))
	    Options::inputs->append(resolvedName);
	break;
      }

    case 'L':
      AddLinkArgumentForGCC("-L");
      AddLinkArgumentForGCC(optarg);

      Options::libDirs->append(new Path(optarg));
      break;

    case 'O':			// a.k.a. -O2
      {
	std::string optlevel = "-O";
	if (optarg)
	  optlevel += optarg;

	AddCompileArgumentForGCC(optlevel);
	AddLinkArgumentForGCC(optlevel);
      }

      break;

    default:
      opterr++;
      break;
    }
  }
  
  for (size_t i = 0; i < Options::SystemDirs->size(); i++) {
    stringstream incpath;
    incpath << Options::SystemDirs->elem(i) << "/include";
    
    UocInfo::searchPath->append(new Path(incpath.str()));
    Options::CompilePreOptionsGCC->append("-I");
    Options::CompilePreOptionsGCC->append(incpath.str());

    stringstream libpath;
    libpath << Options::SystemDirs->elem(i) << "/lib";
    Options::libDirs->append(new Path(libpath.str()));
    Options::LinkPostOptionsGCC->append("-L");
    Options::LinkPostOptionsGCC->append(libpath.str());
  }

  if (Options::useStdLib) {
    std::string resolvedName;
    if (ResolveLibPath("bitc", resolvedName))
      Options::inputs->append(resolvedName);
  }

#if 0
  const char *root_dir = getenv("COYOTOS_ROOT");
  const char *xenv_dir = getenv("COYOTOS_XENV");

  if (root_dir) {
    stringstream incpath;
    stringstream libpath;
    incpath << root_dir << "/host/include";
    libpath << root_dir << "/host/lib";

    if (Options::useStdInc)
      UocInfo::searchPath->append(new Path(incpath.str()));

    // Thankfully, this is not actually what --nostdlib means. What it
    // means is that we should not automatically add -lbitc to the
    // link line.
    if (Options::useStdLib) {
      AddLinkArgumentForGCC("-L");
      AddLinkArgumentForGCC(libpath.str());
      Options::libDirs->append(new Path(libpath.str()));
    }
  }

  if (xenv_dir) {
    stringstream incpath;
    stringstream libpath;
    incpath << xenv_dir << "/host/include";
    libpath << xenv_dir << "/host/lib";

    if (Options::useStdInc)
      UocInfo::searchPath->append(new Path(incpath.str()));

    if (Options::useStdLib) {
      AddLinkArgumentForGCC("-L");
      AddLinkArgumentForGCC(libpath.str());
      Options::libDirs->append(new Path(libpath.str()));
    }
  }
#endif

  /* From this point on, argc and argv should no longer be consulted. */

  if (Options::inputs->size() == 0)
    opterr++;

  if (opterr) {
    std::cerr << "Usage: Try bitcc --help" << std::endl;
    exit(0);
  }

  if(Options::backEnd == 0) {
    Options::backEnd = &BackEnd::backends[0];
  }

  if (Options::outputFileName.size() == 0)
    Options::outputFileName = "bitc.out";
  
  /************************************************************/
  /*                UOC Parse and Validate                    */
  /************************************************************/
  // Process the Prelude:

  // FIX: TEMPORARY
  if(!Options::noPrelude) {
    sherpa::LexLoc loc = LexLoc();
    (void) UocInfo::importInterface(std::cerr, loc, "bitc.prelude");
  }
  
  // Compile everything
  for(size_t i = 0; i < Options::inputs->size(); i++)
    UocInfo::CompileFromFile(Options::inputs->elem(i), true);

  /* Per-file backend output after processing frontend, if any */
  bool doFinal = true;

  /* Output for interfaces */
  for(size_t i = 0; i < UocInfo::ifList->size(); i++) {
    GCPtr<UocInfo> puoci = UocInfo::ifList->elem(i);
    
    if (puoci->lastCompletedPass >= Options::backEnd->needPass) {
      if (Options::backEnd->fn)
	Options::backEnd->fn(std::cout, std::cerr, puoci);
    }
    else
      doFinal = false;
  } 

  /* Output for Source modules */
  for(size_t i = 0; i < UocInfo::srcList->size(); i++) {
    GCPtr<UocInfo> puoci = UocInfo::srcList->elem(i);

    if (puoci->lastCompletedPass >= Options::backEnd->needPass){
      if (Options::backEnd->fn)
	Options::backEnd->fn(std::cout, std::cerr, puoci);
    }
    else
      doFinal = false;
  } 

  /* We have completed all of the per-UOC passes. Assuming that we did
   * so successfully, run any required mid-end function:
   */
  if (doFinal) {
    if (Options::backEnd->midfn) {
      Options::backEnd->midfn(std::cout, std::cerr);
    }
  }

  if (!doFinal)
    exit(1);

  if (Options::backEnd->flags & BK_UOC_MODE)
    exit(0);

  /************************************************************/
  /*   The mid zone, build the polyinstantiate + big-ast      */
  /************************************************************/
#if (BITC_CURRENT_MODE == BITC_COMPILER_MODE)
  /* Build the list of things to be instantiated */

  /** The following symbols are introduced by code that is added in the
   * SSA pass. This code is added @em after polyinstantiation, and
   * consequently cannot be discovered by the demand-driven incremental
   * instantiation process. Add them to the emission list by hand here
   * to ensure that they get emitted.
   */
  Options::entryPts->append("bitc.prelude.__index_lt");
  Options::entryPts->append("bitc.prelude.IndexBoundsError");  

  /** If we are not compiling in header mode, we also need to emit
   * code for the primary entry point, which is generally
   * bitc.main.main. All else will follow from that as
   * polyinstantiation proceeds.
   *
   * Header mode is a bit different. In that mode we are converting
   * interfaces into header files, and there is no particular entry
   * point to emit.
   */
  if(!userAddedEntryPts &&
     ((Options::backEnd->flags & BK_HDR_MODE) == 0))
    Options::entryPts->append("bitc.main.main");

  /** Add all other top level forms that might cause
   * side-effects. These are the top-level initializers. This is a
   * temporary expedient. We need to re-examine the rules for legal
   * top-level initialization.
   */
  UocInfo::addAllCandidateEPs(); 
#endif

  /* Create a new unit of compilation that will become the grand,
     unified UoC */
  GCPtr<UocInfo> unifiedUOC = UocInfo::CreateUnifiedUoC();

  // Update all of the defForm pointers so that we can find things:
  UocInfo::findAllDefForms();
    
  // Build the master back-end AST. This is done in a way that can be
  // extended incrementally.
  // The batch version is similar to the unbatched instantiator as
  // it calls the real instantiator one entry point at a time over
  // the list of entry points it recieves. The only difference is
  // that the megaENVs is not updated after each entry point if we
  // use batch mode. In an interpreter, if we are instantiating only
  // one entry point, one might as well use instantiate instead of
  // instantiateBatch call.
  bool midPassOK = unifiedUOC->instantiateBatch(std::cerr, Options::entryPts);
    
  if (Options::dumpAfterMidEnd) {
    std::cerr << "==== DUMPING *unified UOC*"
	      << " AFTER mid-end"
	      << " ====" << std::endl;
    unifiedUOC->PrettyPrint(std::cerr, Options::ppDecorate);
  }
  if (Options::dumpTypesAfterMidEnd) {
    std::cerr << "==== TYPES for *unified UOC*"
	      << " AFTER mid-end"
	      << " ====" << std::endl;
    unifiedUOC->ShowTypes(std::cerr);
    std::cerr <<std::endl << std::endl;
  }
  
  if(!midPassOK) {
    std::cerr << "Exiting due to errors during Instantiation."
	      << std::endl;
    exit(1);    
  }
  
  /************************************************************/
  /*                        The backend                       */
  /************************************************************/

  // Finally perform one-pass backend Functions.
  unifiedUOC->DoBackend();
  
  // If there is any post-gather output, it should be done now.
  if(Options::backEnd->plfn) {
    bool done = Options::backEnd->plfn(std::cout, std::cerr, 
				       unifiedUOC);
    if(!done)
      exit(1);
  }
  
  exit(0);
}
