// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/Core/Replacement.h"
#include "clang/Tooling/Refactoring.h"
#include <iostream>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

std::string getTemplateParameters(const std::string& functionCall) {
  std::string res;
  bool opened = false;
  int level = 0;
  for (auto&& c: functionCall) {
    if (c == '<') {
      ++level;
      if (!opened) {
        opened = true;
        continue;
      }
    }
    
    if (c == '>') {
      --level;
    }

    if (level == 0 && opened)
      break;

    if (opened)
      res.push_back(c);

  }
  return res;
}

std::string removeLineBreaks(const std::string& s) {
	std::string res;
  res.reserve(s.size());
  for (auto&& c: s)
    if (c != '\n')
      res.push_back(c);
  return res;
}

std::string trimHeadSpaces(const std::string& s) {
  auto iter = s.begin();
  for (; iter != s.end(); ++iter)
    if (*iter != ' ') break;	  
  return std::string(iter, s.end());
}

bool functionOrConstructor(const std::string& s) {
	bool fl1 = (s.find("(") != std::string::npos);
	bool fl2 = (s.find(")") != std::string::npos);
	bool fl3 = (s.find("{") != std::string::npos);
	bool fl4 = (s.find("}") != std::string::npos);
  return (fl1 && fl2) || (fl3 && fl4); 
}

std::string convertDimensionsArg(const std::string& dimArgs) {
  std::string emptyInit = "Dims(), Shape()";
  if (dimArgs.find("Dimensions{}") != std::string::npos ||
      dimArgs.find("Dimensions()") != std::string::npos)
    return emptyInit;
  bool isInitializerList = (*dimArgs.rbegin() == '}');
  std::string dims = "Dims{";
  std::string shape = "Shape{";
  if (isInitializerList) {
    bool hasCommas = (dimArgs.find(",") != std::string::npos);
    if (!hasCommas) {
		  std::string actualArg(dimArgs.begin() + dimArgs.find("{"), dimArgs.end());
			actualArg = trimHeadSpaces(std::string(actualArg.begin(), 
												         std::remove_if(actualArg.begin(), actualArg.end(),
																 [](char& c) { return c == '}' || c == '{'; })));
			if (actualArg.empty())
			  return "Dims(), Shape()";
			else
				return "Dimensions(" + dimArgs + ")"; 
		}

    std::string args;
    auto pos = dimArgs.find('{');
    if (dimArgs[pos] == '{' && dimArgs[pos+1] == '{')
      args = std::string(dimArgs, pos + 1, dimArgs.size() - pos - 2);
    else
      args = std::string(dimArgs, pos, dimArgs.size() - pos);

    std::vector<std::string> tokens;
    std::string curToken;
    bool start = false;
    for(auto&& ch: args) {
      if (!start) {
        if (ch == '{') {
          start = true;
          curToken.push_back('{');
        }
        continue;
      }

      if (ch == '}') {
        tokens.emplace_back(curToken + "}");
        curToken.clear();
        continue;
      }
      if ((ch == ',' || ch == ' ') && curToken.empty())
        continue;
      curToken.push_back(ch);
    }

    for (auto&& tok: tokens) {
      args = tok;
      args.erase(std::remove(args.begin(), args.end(), '{'), args.end());
      args.erase(std::remove(args.begin(), args.end(), '}'), args.end());
      auto comma = args.find(',');
      dims += std::string(args, 0, comma) + ", ";
      shape += trimHeadSpaces(std::string(args.begin() + comma + 1, args.end())) + ", ";
    }
    dims.pop_back();
    shape.pop_back();
  } else {
    if (dimArgs.find("Dimensions") != std::string::npos) {	  
			int p1{0}, p2{0}, p3{0};
			int level = 0;
			for (std::size_t i = 0; i < dimArgs.size(); ++i) {
				auto ch = dimArgs[i];
				if (ch == '(' || ch == '{')
					++level;

				if (ch == ')' || ch == '}')
					--level;

				if (level == 1 && ch == ',')
					p2 = i;
				if (level == 1 && ch == '(')
					p1 = i;
				if(level == 0 && ch == ')')
					p3 = i;
			}
			dims += std::string(dimArgs, p1 + 1, p2 - 1 - p1);
			shape += trimHeadSpaces(std::string(dimArgs,p2 + 1, p3 - 1 - p2));
			if (*dims.rbegin() == '{' && *(dims.rbegin()++) == '{') {
				dims.pop_back();
				dims.erase(dims.begin());
			}

			if (*shape.rbegin() == '{' && *(shape.rbegin()++) == '{') {
				shape.pop_back();
				shape.erase(dims.begin());
			}
    } else {
			return "Dimensions(" + dimArgs + ")"; 
		}
  }

  *dims.rbegin() = '}';
  *shape.rbegin() = '}';
  return dims + ", " + shape;
}

LangOptions langOpts;
std::set<Replacement> replacements;

class MakeVariableWithDimsCallBack: public MatchFinder::MatchCallback {
public :

static StatementMatcher MakeVariableMatcher;	
MakeVariableWithDimsCallBack(std::map<std::string, Replacements> *rm) : replaceMap(rm) {}
  virtual void run(const MatchFinder::MatchResult &Result) {
    PrintingPolicy policy(langOpts);
    SourceManager *SM = Result.SourceManager;
    if (const CallExpr *FS = Result.Nodes.getNodeAs<clang::CallExpr>("makeVariable")) {     
       std::string exprStr;
       raw_string_ostream exprS(exprStr);
       FS->printPretty(exprS, 0, policy);            
       
       outs() << "expression: " << exprS.str() << "\n";
       SourceRange range = FS->getSourceRange();
			 exprStr = std::string(Lexer::getSourceText(CharSourceRange::getTokenRange(range), *SM, langOpts));
       outs() << "native expression: " << exprS.str() << "\n";
       auto startLoc = SM->getSpellingLoc(range.getBegin());
       auto endLoc = SM->getSpellingLoc(range.getEnd());
       SourceRange toReplace(startLoc, endLoc);
       outs() << "location: " << startLoc.printToString(*SM)  << "\t" << endLoc.printToString(*SM) << "\n";
       std::string substitute = "createVariable<" + getTemplateParameters(exprStr) + ">(";
       bool values = true;
       for (auto&& iter: FS->arguments()) {
           std::string argStr;
	         raw_string_ostream s(argStr);
           iter->printPretty(s, 0, policy);
           std::string type = iter->getType().getAsString();
           outs() << "arg: " << type << " " << s.str() << "\n";

           SourceRange argRange = iter->getSourceRange();
           argStr = removeLineBreaks(std::string(Lexer::getSourceText(CharSourceRange::getTokenRange(argRange), *SM, langOpts)));
           outs() << "native arg: " << type << " " << s.str() << "\n";
           if (type.find("Dimensions") != std::string::npos) {
	           substitute += convertDimensionsArg(argStr) + ", ";	   
           } else if(type.find("Unit") != std::string::npos 
                  || type.find("unit") != std::string::npos
                  || argStr.find("units::") != std::string::npos) {
             if (argStr.find("units::Unit") == std::string::npos)
               substitute += "units::Unit(" + argStr + "), ";
             else 
               substitute += argStr + ", "; 
           } else if(type.find("initializer_list") != std::string::npos) {
             std::string pref = "(";
             std::string suf = ")";
             if (argStr.find("{") != std::string::npos && argStr.find("}") != std::string::npos) {
             	 pref.clear();
               suf.clear(); 
             }
                
						 if (values) {
							 substitute += "Values" + pref + argStr + suf + ", ";
							 values =false; 
						 } else {
							 if (!argStr.empty())
								 substitute += "Variances" + pref + argStr + suf + ", ";
						 }
           } else if(type.find("vector") != std::string::npos) {
             if (functionOrConstructor(argStr)) {
               substitute = exprStr + "/*LABEL_1*/";
               break;
             }
             if (values) {
               substitute += "Values(" + argStr + ".begin(), " + argStr + ".end()" + "), ";
               values =false; 
             } else {
               if (!argStr.empty())
                 substitute += "Variances(" + argStr + ".begin(), " + argStr + ".end()" + "), ";
             }
           } else {
             if (values) {
               substitute += "Values(" + argStr + "), ";
               values =false; 
             } else {
               if (!argStr.empty())
                 substitute += "Variances(" + argStr + "), ";
             } 
           }
       }
       if (substitute.find("LABEL") == std::string::npos) {
         substitute.pop_back(); 
         *substitute.rbegin() = ')';
       }
       outs() << "substitution: " << substitute << "\n";
       outs() << "\n\n";
			 std::string fileName = SM->getFilename(startLoc);
			 auto iter = replaceMap->find(fileName);
			 if (iter == replaceMap->end())
				 iter = replaceMap->emplace(fileName, Replacements()).first;
       auto repl = Replacement(*SM,  CharSourceRange(toReplace, true), substitute, langOpts);
			 if (replacements.find(repl) == replacements.end() && fileName.find("variable.h") == std::string::npos) {
         replacements.emplace(repl);
         auto error = iter->second.add(repl);
			   if (error)
				   errs() << "Error: " << error << "\n";
       }
    }
  }
private:
	std::map<std::string, Replacements> *replaceMap;
};

StatementMatcher MakeVariableWithDimsCallBack::MakeVariableMatcher = callExpr(callee(functionDecl(hasName("makeVariable"))), hasArgument(0, hasType(recordDecl(hasName("Dimensions"))))).bind("makeVariable");

int main(int argc, const char **argv) {
  langOpts.CPlusPlus = true;
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  RefactoringTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
  MakeVariableWithDimsCallBack withDimensions(&Tool.getReplacements());
  MatchFinder Finder;
  Finder.addMatcher(MakeVariableWithDimsCallBack::MakeVariableMatcher, &withDimensions);
  auto Result = Tool.runAndSave(newFrontendActionFactory(&Finder).get());
	return Result;
}

