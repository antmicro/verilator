#ifndef _UHDM_AST_H_
#define _UHDM_AST_H_ 1

#include <vector>

#include "headers/uhdm.h"

#include "V3Ast.h"
#include "V3ParseSym.h"

namespace UhdmAst {

typedef std::map<std::string, AstNode*> NameNodeMap;

// Variables that need to be accessed by multiple nodes in the design
struct UhdmShared {
    std::map<std::string, AstPackage*> package_map;
    std::string package_prefix;
    NameNodeMap partial_modules;
    std::unordered_map<const UHDM::BaseClass*, std::string> visited_types;
    // Store parameters here (values can be updated for each instance)
    // Final values will be added in respective module/package
    std::map<std::string, NameNodeMap> top_param_map;
    std::set<std::tuple<std::string, int, std::string>> coverage_set;
    V3ParseSym* m_symp;
    // Used to distinguish between task/function calls inside statement hierarchy
    bool isFunction;

    std::map<std::string, AstNodeModule*> top_nodes;
};

// Parse vpi*Typespec nodes
AstNode* process_typespec(vpiHandle obj_h, UhdmShared& shared);

// Visits single VPI object and creates proper AST node
AstNode* visit_object(vpiHandle obj_h, UhdmShared& shared);

// Visits all VPI design objects and returns created ASTs
std::vector<AstNodeModule*> visit_designs(const std::vector<vpiHandle>& designs,
                                          std::ostream& coverage_report_stream, V3ParseSym* symp);
}  // namespace UhdmAst
#endif  // Guard
