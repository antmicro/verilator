#include <vector>
#include <stack>
#include <functional>
#include <algorithm>
#include <regex>

#include "headers/uhdm.h"

#include "V3Ast.h"
#include "V3ParseSym.h"
#include "V3Global.h"
#include "UhdmAst.h"

namespace UhdmAst {

// Walks through one-to-many relationships from given parent
// node through the VPI interface, visiting child nodes belonging to
// ChildrenNodeTypes that are present in the given object.
void visit_one_to_many(const std::vector<int> childrenNodeTypes, vpiHandle parentHandle,
                       UhdmShared& shared, const std::function<void(AstNode*)>& f) {
    for (auto child : childrenNodeTypes) {
        vpiHandle itr = vpi_iterate(child, parentHandle);
        while (vpiHandle vpi_child_obj = vpi_scan(itr)) {
            auto* childNode = visit_object(vpi_child_obj, shared);
            f(childNode);
            vpi_free_object(vpi_child_obj);
        }
        vpi_free_object(itr);
    }
}

// Walks through one-to-one relationships from given parent
// node through the VPI interface, visiting child nodes belonging to
// ChildrenNodeTypes that are present in the given object.
void visit_one_to_one(const std::vector<int> childrenNodeTypes, vpiHandle parentHandle,
                      UhdmShared& shared, const std::function<void(AstNode*)>& f) {
    for (auto child : childrenNodeTypes) {
        vpiHandle itr = vpi_handle(child, parentHandle);
        if (itr) {
            auto* childNode = visit_object(itr, shared);
            f(childNode);
        }
        vpi_free_object(itr);
    }
}

void sanitize_str(std::string& s) {
    if (!s.empty()) {
        auto pos = s.rfind("@");
        s = s.substr(pos + 1);
        // Replace [ and ], seen in GenScope names
        s = std::regex_replace(s, std::regex("\\["), "__BRA__");
        s = std::regex_replace(s, std::regex("\\]"), "__KET__");
    }
}

std::string get_object_name(vpiHandle obj_h, const std::vector<int>& name_fields = {vpiName}) {
    std::string objectName;
    for (auto name : name_fields) {
        if (auto s = vpi_get_str(name, obj_h)) {
            objectName = s;
            sanitize_str(objectName);
            break;
        }
    }
    return objectName;
}

bool is_imported(vpiHandle obj_h) {
    if (auto s = vpi_get_str(vpiImported, obj_h)) {
        return true;
    } else {
        return false;
    }
}

void remove_scope(std::string& s) {
    auto pos = s.rfind("::");
    if (pos != std::string::npos) s = s.substr(pos + 2);
}

AstPackage* get_package(UhdmShared& shared, const std::string& objectName) {
    std::size_t delimiter_pos = objectName.rfind("::");
    std::size_t prefix_pos = objectName.find("::");
    AstPackage* classpackagep = nullptr;
    if (delimiter_pos != std::string::npos) {
        std::string classpackageName = "";
        if (prefix_pos < delimiter_pos) {
            // "Nested" packages - package importing package
            // Last one is where definition is located
            classpackageName = objectName.substr(prefix_pos + 2, delimiter_pos - prefix_pos - 2);
        } else {
            // Simple package reference
            classpackageName = objectName.substr(0, delimiter_pos);
        }
        // Nested or not, func is named after last package
        auto object_name = objectName.substr(delimiter_pos + 2, objectName.length());
        UINFO(7, "Found package prefix: " << classpackageName << std::endl);
        remove_scope(classpackageName);
        auto it = shared.package_map.find(classpackageName);
        if (it != shared.package_map.end()) { classpackagep = it->second; }
    }
    return classpackagep;
}

bool is_expr_context(vpiHandle obj_h) {
    auto parent_h = vpi_handle(vpiParent, obj_h);
    if (parent_h) {
        auto parent_type = vpi_get(vpiType, parent_h);
        switch (parent_type) {
        case vpiOperation:
        case vpiIf:
        case vpiIfElse:
        case vpiAssignStmt:
        case vpiAssignment:
        case vpiContAssign: {
            return true;
        }
        case vpiBegin:
        case vpiFuncCall:
        case vpiTaskCall:
        case vpiCaseItem: {
            return false;
        }
        default: {
            v3info("Encountered unhandled parent type in " << __FUNCTION__ << std::endl);
            return false;
        }
        }
    } else {
        v3info("Missing parent handle in " << __FUNCTION__ << std::endl);
        // TODO: it seems that this happens only in expr context?
        return true;
    }
}

string deQuote(FileLine* fileline, string text) {
    // Fix up the quoted strings the user put in, for example "\"" becomes "
    // Reverse is V3OutFormatter::quoteNameControls(...)
    bool quoted = false;
    string newtext;
    unsigned char octal_val = 0;
    int octal_digits = 0;
    for (string::const_iterator cp = text.begin(); cp != text.end(); ++cp) {
        if (quoted) {
            if (isdigit(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                if (++octal_digits == 3) {
                    octal_digits = 0;
                    quoted = false;
                    newtext += octal_val;
                }
            } else {
                if (octal_digits) {
                    // Spec allows 1-3 digits
                    octal_digits = 0;
                    quoted = false;
                    newtext += octal_val;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    continue;
                }
                quoted = false;
                if (*cp == 'n')
                    newtext += '\n';
                else if (*cp == 'a')
                    newtext += '\a';  // SystemVerilog 3.1
                else if (*cp == 'f')
                    newtext += '\f';  // SystemVerilog 3.1
                else if (*cp == 'r')
                    newtext += '\r';
                else if (*cp == 't')
                    newtext += '\t';
                else if (*cp == 'v')
                    newtext += '\v';  // SystemVerilog 3.1
                else if (*cp == 'x' && isxdigit(cp[1]) && isxdigit(cp[2])) {  // SystemVerilog 3.1
#define vl_decodexdigit(c) ((isdigit(c) ? ((c) - '0') : (tolower((c)) - 'a' + 10)))
                    newtext += (char)(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } else if (isalnum(*cp)) {
                    fileline->v3error("Unknown escape sequence: \\" << *cp);
                    break;
                } else
                    newtext += *cp;
            }
        } else if (*cp == '\\') {
            quoted = true;
            octal_digits = 0;
        } else if (*cp != '"') {
            newtext += *cp;
        }
    }
    return newtext;
}

FileLine* make_fileline(vpiHandle obj_h) {
    std::string filename = "uhdm";
    if (auto* s = vpi_get_str(vpiFile, obj_h)) { filename = s; }
    const int lineNo = vpi_get(vpiLineNo, obj_h);
    const int endLineNo = vpi_get(vpiEndLineNo, obj_h);
    const int columnNo = vpi_get(vpiColumnNo, obj_h);
    const int endColumnNo = vpi_get(vpiEndColumnNo, obj_h);
    auto* fl = new FileLine(filename);
    fl->firstLineno(lineNo);
    fl->lastLineno(endLineNo);
    fl->firstColumn(columnNo);
    fl->lastColumn(endColumnNo);
    return fl;
}

AstNode* get_referenceNode(FileLine* fl, const string& name) {
    size_t dot_pos = name.find('.');
    if (dot_pos != std::string::npos) {
        std::string lhs = name.substr(0, dot_pos);
        std::string rhs = name.substr(dot_pos + 1, name.length());
        AstParseRef* lhsNode
            = new AstParseRef(fl, VParseRefExp::en::PX_TEXT, lhs, nullptr, nullptr);
        AstNode* rhsNode = get_referenceNode(fl, rhs);
        return new AstDot(fl, false, lhsNode, rhsNode);
    } else {
        return new AstParseRef(fl, VParseRefExp::en::PX_TEXT, name, nullptr, nullptr);
    }
}

AstNode* get_class_package_ref_node(FileLine* fl, std::string objectName, UhdmShared& shared) {
    AstNode* refp = nullptr;
    size_t colon_pos = objectName.find("::");
    while (colon_pos != std::string::npos) {
        std::string classPkgName = objectName.substr(0, colon_pos);
        objectName = objectName.substr(colon_pos + 2, objectName.length());

        UINFO(7, "Creating ClassOrPackageRef" << std::endl);
        AstPackage* classpackagep = nullptr;
        auto it = shared.package_map.find(classPkgName);
        if (it != shared.package_map.end()) { classpackagep = it->second; }
        AstNode* classpackageref
            = new AstClassOrPackageRef(fl, classPkgName, classpackagep, nullptr);
        shared.m_symp->nextId(classpackagep);
        if (refp == nullptr)
            refp = classpackageref;
        else
            refp = new AstDot(fl, true, refp, classpackageref);

        colon_pos = objectName.find("::");
    }
    return refp;
}

AstNode* get_value_as_node(vpiHandle obj_h, bool need_decompile = false) {
    AstNode* valueNodep = nullptr;
    std::string valStr;

    // Most nodes will have raw value in vpiDecompile, leave deducing the type to Verilator
    if (need_decompile) {
        if (auto s = vpi_get_str(vpiDecompile, obj_h)) {
            valStr = s;
            auto type = vpi_get(vpiConstType, obj_h);
            if (type == vpiStringConst) {
                valueNodep = new AstConst(make_fileline(obj_h), AstConst::VerilogStringLiteral(),
                                          deQuote(make_fileline(obj_h), valStr));
            } else if (type == vpiRealConst) {
                bool parseSuccess;
                double value = VString::parseDouble(valStr, &parseSuccess);
                UASSERT(parseSuccess, "Unable to parse real value: " + valStr);

                valueNodep = new AstConst(make_fileline(obj_h), AstConst::RealDouble(), value);
            } else {
                valStr = s;
                if (valStr.find('\'') == std::string::npos) {
                    if (int size = vpi_get(vpiSize, obj_h)) {
                        if (type == vpiBinaryConst) valStr = "'b" + valStr;
                        else if (type == vpiOctConst) valStr = "'o" + valStr;
                        else if (type == vpiHexConst) valStr = "'h" + valStr;
                        else valStr = "'d" + valStr;
                        valStr = std::to_string(size) + valStr;
                    }
                    auto* constp = new AstConst(make_fileline(obj_h), AstConst::StringToParse(), valStr.c_str());
                    auto& num = constp->num();
                    if (num.width() > 32 && num.widthMin() <= 32) {
                        num.width(32, false);
                        num.isSigned(true);
                        constp = new AstConst(make_fileline(obj_h), num);
                    }
                    valueNodep = constp;
                } else {
                    auto* constp = new AstConst(make_fileline(obj_h), AstConst::StringToParse(), valStr.c_str());
                    valueNodep = constp;
                }
            }
            return valueNodep;
        }
    } else {
        UINFO(7, "Requested vpiDecompile value not found in UHDM" << std::endl);
    }
    s_vpi_value val;
    vpi_get_value(obj_h, &val);
    switch (val.format) {
    case vpiIntVal:
    case vpiScalarVal:
    case vpiUIntVal: {
        if (val.format == vpiIntVal)
            valStr = std::to_string(val.value.integer);
        else if (val.format == vpiScalarVal)
            valStr = std::to_string(val.value.scalar);
        else if (val.format == vpiUIntVal)
            valStr = std::to_string(val.value.uint);

        if (valStr[0] == '-') {
            valStr = valStr.substr(1);
            auto* inner = new AstConst(make_fileline(obj_h), AstConst::StringToParse(), valStr.c_str());
            valueNodep = new AstNegate(make_fileline(obj_h), inner);
            break;
        }

        if (int size = vpi_get(vpiSize, obj_h))
            valStr = std::to_string(size) + "'d" + valStr;
        auto* constp = new AstConst(make_fileline(obj_h), AstConst::StringToParse(), valStr.c_str());
        auto& num = constp->num();
        if (num.width() > 32 && num.widthMin() <= 32) {
            num.width(32, false);
            num.isSigned(true);
            constp = new AstConst(make_fileline(obj_h), num);
        }
        valueNodep = constp;
        break;
    }
    case vpiRealVal: {
        valStr = std::to_string(val.value.real);

        bool parseSuccess;
        double value = VString::parseDouble(valStr, &parseSuccess);
        UASSERT(parseSuccess, "Unable to parse real value: " + valStr);

        valueNodep = new AstConst(make_fileline(obj_h), AstConst::RealDouble(), value);
        break;
    }
    case vpiBinStrVal:
    case vpiOctStrVal:
    case vpiDecStrVal:
    case vpiHexStrVal: {
        // if vpiDecompile is unavailable i.e. in EnumConst, cast the string
        // size is stored in enum typespec
        if (val.format == vpiBinStrVal)
            valStr = "'b" + std::string(val.value.str);
        else if (val.format == vpiOctStrVal)
            valStr = "'o" + std::string(val.value.str);
        else if (val.format == vpiDecStrVal)
            valStr = "'d" + std::string(val.value.str);
        else if (val.format == vpiHexStrVal)
            valStr = "'h" + std::string(val.value.str);
        if (int size = vpi_get(vpiSize, obj_h))
            valStr = std::to_string(size) + valStr;
        auto* constp = new AstConst(make_fileline(obj_h), AstConst::StringToParse(), valStr.c_str());
        valueNodep = constp;
        break;
    }
    case vpiStringVal: {
        if (auto* s = val.value.str) valStr = std::to_string(*s);
        valStr.assign(val.value.str);
        valueNodep = new AstConst(make_fileline(obj_h), AstConst::VerilogStringLiteral(),
                                  deQuote(make_fileline(obj_h), valStr));
        break;
    }
    default: {
        v3info("Encountered unknown value format " << val.format);
        break;
    }
    }
    return valueNodep;
}

AstBasicDTypeKwd get_kwd_for_type(int vpi_var_type) {
    switch (vpi_var_type) {
    case vpiLogicTypespec:
    case vpiLogicNet:
    case vpiLogicVar: {
        return AstBasicDTypeKwd::LOGIC;
    }
    case vpiIntTypespec:
    case vpiIntVar: {
        return AstBasicDTypeKwd::INT;
    }
    case vpiLongIntTypespec:
    case vpiLongIntVar: {
        return AstBasicDTypeKwd::LONGINT;
    }
    case vpiIntegerTypespec:
    case vpiIntegerNet:
    case vpiIntegerVar: {
        return AstBasicDTypeKwd::INTEGER;
    }
    case vpiBitTypespec:
    case vpiBitVar: {
        return AstBasicDTypeKwd::BIT;
    }
    case vpiByteTypespec:
    case vpiByteVar: {
        return AstBasicDTypeKwd::BYTE;
    }
    case vpiRealTypespec:
    case vpiRealVar: {
        return AstBasicDTypeKwd::DOUBLE;
    }
    case vpiStringTypespec:
    case vpiStringVar: {
        return AstBasicDTypeKwd::STRING;
    }
    case vpiTimeTypespec:
    case vpiTimeNet:
    case vpiTimeVar: {
        return AstBasicDTypeKwd::TIME;
    }
    case vpiChandleTypespec:
    case vpiChandleVar: {
        return AstBasicDTypeKwd::CHANDLE;
    }
    case vpiEnumTypespec:
    case vpiEnumVar:
    case vpiEnumNet:
    case vpiStructTypespec:
    case vpiStructNet:
    case vpiStructVar:
    case vpiArrayTypespec:
    case vpiPackedArrayTypespec:
    case vpiUnionTypespec: {
        // Not a basic dtype, needs further handling
        return AstBasicDTypeKwd::UNKNOWN;
    }
    default: v3error("Unknown object type " << vpi_var_type);
    }
    return AstBasicDTypeKwd::UNKNOWN;
}

AstNodeDType* getDType(FileLine* fl, vpiHandle obj_h, UhdmShared& shared) {
    if (vpiHandle alias_h = vpi_handle(vpiTypedefAlias, obj_h)) {
        return getDType(fl, alias_h, shared);
    }

    AstNodeDType* dtypep = nullptr;
    auto type = vpi_get(vpiType, obj_h);
    if (type == vpiPort) {
        auto ref_h = vpi_handle(vpiLowConn, obj_h);
        if (!ref_h) {
            v3error("Could not get lowconn handle for port, aborting");
            return nullptr;
        }
        auto actual_h = vpi_handle(vpiActual, ref_h);
        if (!actual_h) {
            v3error("Could not get actual handle for port, aborting");
            return nullptr;
        }
        auto ref_type = vpi_get(vpiType, actual_h);
        if (ref_type == vpiLogicNet || ref_type == vpiIntegerNet || ref_type == vpiTimeNet
            || ref_type == vpiArrayNet || ref_type == vpiPackedArrayNet) {
            type = ref_type;
            obj_h = actual_h;
        } else if (ref_type == vpiEnumNet || ref_type == vpiStructNet || ref_type == vpiStructVar
                   || ref_type == vpiEnumVar) {
            auto typespec_h = vpi_handle(vpiTypedef, obj_h);
            if (typespec_h) {
                type = vpi_get(vpiType, typespec_h);
                obj_h = typespec_h;
            }
        } else if (ref_type == vpiModport || ref_type == vpiInterface) {
            // Special handling, generate only reference
            vpiHandle iface_h = nullptr;
            if (ref_type == vpiModport) {
                iface_h = vpi_handle(vpiInterface, actual_h);
            } else if (ref_type == vpiInterface) {
                iface_h = actual_h;
            }
            std::string cellName, ifaceName;
            if (auto s = vpi_get_str(vpiName, ref_h)) {
                cellName = s;
                sanitize_str(cellName);
            }
            if (auto s = vpi_get_str(vpiDefName, iface_h)) {
                ifaceName = s;
                sanitize_str(ifaceName);
            }
            return new AstIfaceRefDType(fl, cellName, ifaceName);
        }
    }
    if (type == vpiEnumNet || type == vpiStructNet || type == vpiStructVar || type == vpiEnumVar) {
        auto typespec_h = vpi_handle(vpiTypespec, obj_h);
        if (typespec_h) {
            type = vpi_get(vpiType, typespec_h);
            obj_h = typespec_h;
        }
    }
    AstBasicDTypeKwd keyword = AstBasicDTypeKwd::UNKNOWN;
    switch (type) {
    case vpiLogicNet:
    case vpiLogicTypespec:
    case vpiLogicVar:
    case vpiBitVar:
    case vpiBitTypespec: {
        AstBasicDTypeKwd keyword = get_kwd_for_type(type);
        auto basic = new AstBasicDType(fl, keyword);
        AstRange* rangeNode = nullptr;
        std::stack<AstRange*> range_stack;
        visit_one_to_many({vpiRange}, obj_h, shared, [&](AstNode* node) {
            rangeNode = reinterpret_cast<AstRange*>(node);
            range_stack.push(rangeNode);
        });
        rangeNode = nullptr;
        while (!range_stack.empty()) {
            if (rangeNode == nullptr) {
                // Last node serves for basic type
                rangeNode = range_stack.top();
                basic->rangep(rangeNode);
                dtypep = basic;
            } else {
                // For other levels create a PackedArray
                rangeNode = range_stack.top();
                dtypep = new AstPackArrayDType(fl, VFlagChildDType(), dtypep, rangeNode);
            }
            range_stack.pop();
        }
        if (dtypep == nullptr) { dtypep = basic; }
        break;
    }
    case vpiArrayTypespec: {
        AstRange* rangeNodep = nullptr;
        std::stack<AstRange*> range_stack;
        visit_one_to_many({vpiRange}, obj_h, shared, [&](AstNode* nodep) {
            rangeNodep = reinterpret_cast<AstRange*>(nodep);
            range_stack.push(rangeNodep);
        });

        auto elem_typespec_h = vpi_handle(vpiElemTypespec, obj_h);
        dtypep = getDType(fl, elem_typespec_h, shared);

        while (!range_stack.empty()) {
            rangeNodep = range_stack.top();
            dtypep = new AstUnpackArrayDType(fl, VFlagChildDType(), dtypep, rangeNodep);
            range_stack.pop();
        }
        break;
    }
    case vpiPackedArrayTypespec: {
        AstRange* rangeNodep = nullptr;
        std::stack<AstRange*> range_stack;
        visit_one_to_many({vpiRange}, obj_h, shared, [&](AstNode* nodep) {
            rangeNodep = reinterpret_cast<AstRange*>(nodep);
            range_stack.push(rangeNodep);
        });

        auto elem_typespec_h = vpi_handle(vpiElemTypespec, obj_h);
        if (elem_typespec_h) {
            dtypep = getDType(fl, elem_typespec_h, shared);
        } else {
            UINFO(7, "No elem_typespec found in vpiPackedArrayTypespec, trying IndexTypespec"
                         << std::endl);
            auto index_typespec_h = vpi_handle(vpiIndexTypespec, obj_h);
            if (index_typespec_h) {
                dtypep = getDType(fl, index_typespec_h, shared);

                // Workaround for implicit function argument types
                // They are stored in UHDM as PackedArrays, but parsed by Verilator
                // like the simple types above
                AstBasicDType* basicDTypep = VN_CAST(dtypep, BasicDType);
                if (range_stack.size() == 1 && basicDTypep) {
                    basicDTypep->rangep(range_stack.top());
                    range_stack.pop();
                }
            } else {
                v3error("\t! Failed to get typespec handle for PackedArrayTypespec");
            }
        }

        while (!range_stack.empty()) {
            rangeNodep = range_stack.top();
            dtypep = new AstPackArrayDType(fl, VFlagChildDType(), dtypep, rangeNodep);
            range_stack.pop();
        }
        break;
    }
    case vpiIntegerNet:
    case vpiTimeNet:
    case vpiIntVar:
    case vpiLongIntVar:
    case vpiIntegerVar:
    case vpiByteVar:
    case vpiRealVar:
    case vpiStringVar:
    case vpiTimeVar:
    case vpiChandleVar:
    case vpiIntTypespec:
    case vpiLongIntTypespec:
    case vpiIntegerTypespec:
    case vpiByteTypespec:
    case vpiRealTypespec:
    case vpiStringTypespec:
    case vpiChandleTypespec:
    case vpiTimeTypespec: {
        AstBasicDTypeKwd keyword = get_kwd_for_type(type);
        dtypep = new AstBasicDType(fl, keyword);
        break;
    }
    case vpiEnumNet:
    case vpiStructNet:
    case vpiEnumVar:
    case vpiEnumTypespec:
    case vpiStructTypespec:
    case vpiStructVar:
    case vpiUnionTypespec: {
        std::string type_string;
        const uhdm_handle* const handle = (const uhdm_handle*)obj_h;
        const UHDM::BaseClass* const object = (const UHDM::BaseClass*)handle->object;

        std::string type_name = get_object_name(obj_h);
        auto pos = type_name.rfind("::");
        if (pos != std::string::npos) type_name = type_name.substr(pos + 2);

        if (shared.visited_types.find(object) != shared.visited_types.end()) {
            type_string = shared.visited_types[object];
            size_t delimiter_pos = type_string.rfind("::");
            size_t prefix_pos = type_string.find("::");
            if (delimiter_pos == string::npos) {
                UINFO(7, "No package prefix found, creating ref" << std::endl);
                dtypep = new AstRefDType(fl, type_string);
            } else {
                std::string classpackageName = "";
                if (prefix_pos < delimiter_pos) {
                    // "Nested" packages - package importing package
                    // Last one is where definition is located
                    classpackageName
                        = type_string.substr(prefix_pos + 2, delimiter_pos - prefix_pos - 2);
                } else {
                    // Simple package reference
                    classpackageName = type_string.substr(0, delimiter_pos);
                }
                // Nested or not, type is named after last package
                auto type_name = type_string.substr(delimiter_pos + 2, type_string.length());
                UINFO(7, "Found package prefix: " << classpackageName << std::endl);
                // If we are in the same package - do not create reference,
                // as it will confuse Verilator
                if (classpackageName
                    == shared.package_prefix.substr(0, shared.package_prefix.length() - 2)) {
                    UINFO(7, "In the same package, creating simple ref" << std::endl);
                    dtypep = new AstRefDType(fl, type_name);
                } else {
                    UINFO(7, "Creating ClassOrPackageRef" << std::endl);
                    AstPackage* classpackagep = nullptr;
                    auto it = shared.package_map.find(classpackageName);
                    if (it != shared.package_map.end()) { classpackagep = it->second; }
                    AstNode* classpackageref
                        = new AstClassOrPackageRef(fl, classpackageName, classpackagep, nullptr);
                    shared.m_symp->nextId(classpackagep);
                    dtypep = new AstRefDType(fl, type_name, classpackageref, nullptr);
                }
            }
        } else if (type_name != "") {
            // Type not found or object pointer mismatch, but let's try to create a reference
            // to be resolved later
            // Simple reference only, prefix is not stored in name
            UINFO(7, "No match found, creating ref to name" << type_name << std::endl);
            dtypep = new AstRefDType(fl, type_name);
        } else {
            // Typedefed types were visited earlier, probably anonymous struct
            // Get the typespec here
            UINFO(7, "Encountered anonymous struct");
            AstNode* typespec_p = visit_object(obj_h, shared);
            dtypep = typespec_p->getChildDTypep()->cloneTree(false);
        }
        break;
    }
    case vpiPackedArrayNet:
    case vpiPackedArrayVar: {
        auto typespec_h = vpi_handle(vpiTypespec, obj_h);
        if (typespec_h == 0) {  // Try to get the type from one of the elements
            auto itr = vpi_iterate(vpiElement, obj_h);
            auto element_h = vpi_scan(itr);
            typespec_h = vpi_handle(vpiTypespec, element_h);
        }

        if (typespec_h) {
            dtypep = getDType(fl, typespec_h, shared);
        } else {
            v3error("Missing typespec for packed_array_var");
        }

        AstRange* rangeNodep = nullptr;
        std::stack<AstRange*> range_stack;
        visit_one_to_many({vpiRange}, obj_h, shared, [&](AstNode* nodep) {
            rangeNodep = reinterpret_cast<AstRange*>(nodep);
            range_stack.push(rangeNodep);
        });
        while (!range_stack.empty()) {
            rangeNodep = range_stack.top();
            dtypep = new AstPackArrayDType(fl, VFlagChildDType(), dtypep, rangeNodep);
            range_stack.pop();
        }
        break;
    }
    case vpiArrayVar: {
        auto typespec_h = vpi_handle(vpiTypespec, obj_h);
        if (typespec_h) {
            dtypep = getDType(fl, typespec_h, shared);
        } else {  // Try to get the type from one of the elements
            auto itr = vpi_iterate(vpiReg, obj_h);
            auto member_h = vpi_scan(itr);
            typespec_h = vpi_handle(vpiTypespec, member_h);
            if (typespec_h) {
                dtypep = getDType(fl, typespec_h, shared);
            } else {
                auto element_type = vpi_get(vpiType, member_h);
                if (element_type) {
                    AstBasicDTypeKwd keyword = get_kwd_for_type(element_type);
                    dtypep = new AstBasicDType(fl, keyword);
                } else {
                    v3error("Missing typespec for array_var");
                }
            }
        }

        std::vector<AstRange*> ranges;
        visit_one_to_many({vpiRange}, obj_h, shared, [&](AstNode* itemp) {
            if (itemp != nullptr) { ranges.push_back(reinterpret_cast<AstRange*>(itemp)); }
        });

        for (auto rangep_it = ranges.rbegin(); rangep_it != ranges.rend(); rangep_it++) {
            dtypep = new AstUnpackArrayDType(fl, VFlagChildDType(), dtypep, *rangep_it);
        }
        break;
    }
    case vpiArrayNet: {
        vpiHandle itr = vpi_iterate(vpiNet, obj_h);
        if (vpiHandle vpi_child_obj = vpi_scan(itr)) {
            dtypep = getDType(fl, vpi_child_obj, shared);
            vpi_free_object(vpi_child_obj);
        }
        vpi_free_object(itr);

        std::vector<AstRange*> ranges;
        visit_one_to_many({vpiRange}, obj_h, shared, [&](AstNode* itemp) {
            if (itemp != nullptr) { ranges.push_back(reinterpret_cast<AstRange*>(itemp)); }
        });

        for (auto rangep_it = ranges.rbegin(); rangep_it != ranges.rend(); rangep_it++) {
            dtypep = new AstUnpackArrayDType(fl, VFlagChildDType(), dtypep, *rangep_it);
        }
        break;
    }
    default: v3error("Unknown object type: " << UHDM::VpiTypeName(obj_h));
    }
    return dtypep;
}

AstNode* process_operation(vpiHandle obj_h, UhdmShared& shared,
                           const std::vector<AstNode*>& operands) {
    auto operation = vpi_get(vpiOpType, obj_h);
    switch (operation) {
    case vpiBitNegOp: {
        return new AstNot(make_fileline(obj_h), operands[0]);
    }
    case vpiNotOp: {
        return new AstLogNot(make_fileline(obj_h), operands[0]);
    }
    case vpiBitAndOp: {
        return new AstAnd(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiListOp: {
        AstNode* elementp = operands[0];
        // First operand is assigned above, start from second
        for (auto it = ++operands.begin(); it != operands.end(); it++) {
            elementp->addNextNull(*it);
        }
        return elementp;
    }
    case vpiBitOrOp: {
        return new AstOr(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiBitXorOp: {
        return new AstXor(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiBitXnorOp: {
        return new AstXnor(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiPostIncOp:
    case vpiPostDecOp: {
        auto* onep = new AstConst(make_fileline(obj_h), 1);
        AstNode* op = nullptr;
        if (operation == vpiPostIncOp) {
            op = new AstAdd(make_fileline(obj_h), operands[0], onep);
        } else if (operation == vpiPostDecOp) {
            op = new AstSub(make_fileline(obj_h), operands[0], onep);
        }
        auto* varp = get_referenceNode(make_fileline(obj_h), operands[0]->name());
        return new AstAssign(make_fileline(obj_h), varp, op);
    }
    case vpiAssignmentOp: {
        return new AstAssign(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiUnaryAndOp: {
        return new AstRedAnd(make_fileline(obj_h), operands[0]);
    }
    case vpiUnaryNandOp: {
        auto* op = new AstRedAnd(make_fileline(obj_h), operands[0]);
        return new AstNot(make_fileline(obj_h), op);
    }
    case vpiUnaryNorOp: {
        auto* op = new AstRedOr(make_fileline(obj_h), operands[0]);
        return new AstNot(make_fileline(obj_h), op);
    }
    case vpiUnaryOrOp: {
        return new AstRedOr(make_fileline(obj_h), operands[0]);
    }
    case vpiUnaryXorOp: {
        return new AstRedXor(make_fileline(obj_h), operands[0]);
    }
    case vpiUnaryXNorOp: {
        return new AstRedXnor(make_fileline(obj_h), operands[0]);
    }
    case vpiEventOrOp: {
        // Do not create a separate node
        // Chain operand nodes instead
        AstNode* eventOrNodep = nullptr;
        for (auto op : operands) {
            if (op) {
                if (op->type() == AstType::en::atSenItem) {
                    // This is a Posedge/Negedge operation, keep this op
                    if (eventOrNodep == nullptr) {
                        eventOrNodep = op;
                    } else {
                        eventOrNodep->addNextNull(op);
                    }
                } else {
                    // Edge not specified -> use ANY
                    auto* wrapperp
                        = new AstSenItem(make_fileline(obj_h), VEdgeType::ET_ANYEDGE, op);
                    if (eventOrNodep == nullptr) {
                        eventOrNodep = wrapperp;
                    } else {
                        eventOrNodep->addNextNull(wrapperp);
                    }
                }
            }
        }
        return eventOrNodep;
    }
    case vpiLogAndOp: {
        return new AstLogAnd(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiLogOrOp: {
        return new AstLogOr(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiPosedgeOp: {
        return new AstSenItem(make_fileline(obj_h), VEdgeType::ET_POSEDGE, operands[0]);
    }
    case vpiNegedgeOp: {
        return new AstSenItem(make_fileline(obj_h), VEdgeType::ET_NEGEDGE, operands[0]);
    }
    case vpiEqOp: {
        return new AstEq(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiCaseEqOp: {
        return new AstEqCase(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiNeqOp: {
        return new AstNeq(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiCaseNeqOp: {
        return new AstNeqCase(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiGtOp: {
        return new AstGt(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiGeOp: {
        return new AstGte(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiLtOp: {
        return new AstLt(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiLeOp: {
        return new AstLte(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiPlusOp: {
        return operands[0];
    }
    case vpiSubOp: {
        return new AstSub(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiMinusOp: {
        return new AstNegate(make_fileline(obj_h), operands[0]);
    }
    case vpiAddOp: {
        return new AstAdd(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiMultOp: {
        return new AstMul(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiDivOp: {
        return new AstDiv(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiModOp: {
        return new AstModDiv(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiConditionOp: {
        return new AstCond(make_fileline(obj_h), operands[0], operands[1], operands[2]);
    }
    case vpiConcatOp: {
        AstNode* op1p = nullptr;
        AstNode* op2p = nullptr;
        for (auto op : operands) {
            if (op != nullptr) {
                if (op1p == nullptr) {
                    op1p = op;
                } else if (op2p == nullptr) {
                    op2p = op;
                } else {
                    // Add one more level
                    op1p = new AstConcat(make_fileline(obj_h), op1p, op2p);
                    op2p = op;
                }
            }
        }
        // Wrap in a Replicate node
        if (op2p != nullptr) {
            op1p = new AstConcat(make_fileline(obj_h), op1p, op2p);
            op2p = new AstConst(make_fileline(obj_h), 1);
        } else {
            op2p = new AstConst(make_fileline(obj_h), 1);
        }
        return new AstReplicate(make_fileline(obj_h), op1p, op2p);
    }
    case vpiMultiConcatOp: {
        // Sides in AST are switched: first value, then count
        return new AstReplicate(make_fileline(obj_h), operands[1], operands[0]);
    }
    case vpiArithLShiftOp:  // This behaves the same as normal shift
    case vpiLShiftOp: {
        return new AstShiftL(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiRShiftOp: {
        return new AstShiftR(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiArithRShiftOp: {
        return new AstShiftRS(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiInsideOp: {
        AstNode* exprp = operands[0];
        AstNode* itemsp = operands[1];
        for (auto it = operands.begin() + 2; it != operands.end(); it++) {
            itemsp->addNextNull(*it);
        }
        return new AstInside(make_fileline(obj_h), exprp, itemsp);
    }
    case vpiCastOp: {
        auto typespec_h = vpi_handle(vpiTypespec, obj_h);
        auto type = vpi_get(vpiType, typespec_h);
        AstNode* sizeNodep = nullptr;
        if (type == vpiIntTypespec) {
            auto* namep = vpi_get_str(vpiName, typespec_h);
            if (namep != nullptr)
                sizeNodep = new AstParseRef(make_fileline(typespec_h), VParseRefExp::en::PX_TEXT,
                                            namep, nullptr, nullptr);
        } else if (type == vpiIntegerTypespec) {
            sizeNodep = get_value_as_node(typespec_h);
        }

        if (sizeNodep != nullptr) {
            return new AstCastParse(make_fileline(obj_h), operands[0], sizeNodep);
        } else {
            AstNodeDType* dtypep = getDType(make_fileline(obj_h), typespec_h, shared);
            return new AstCast(make_fileline(obj_h), operands[0], dtypep);
        }
    }
    case vpiStreamRLOp: {
        // Verilog {op1{op0}} - Note op1 is the slice size, not the op0
        // IEEE 11.4.14.2: If a slice_size is not specified, the default is 1.
        if (operands.size() == 1) {
            return new AstStreamL(make_fileline(obj_h), operands[0],
                                  new AstConst(make_fileline(obj_h), 1));
        } else {
            return new AstStreamL(make_fileline(obj_h), operands[0], operands[1]);
        }
    }
    case vpiStreamLROp: {
        // See comments above - default slice size is 1
        if (operands.size() == 1) {
            return new AstStreamR(make_fileline(obj_h), operands[0],
                                  new AstConst(make_fileline(obj_h), 1));
        } else {
            return new AstStreamR(make_fileline(obj_h), operands[0], operands[1]);
        }
    }
    case vpiPowerOp: {
        return new AstPow(make_fileline(obj_h), operands[0], operands[1]);
    }
    case vpiAssignmentPatternOp: {
        AstNode* itemsp = nullptr;
        for (auto op : operands) {
            // Wrap only if this is a positional pattern
            // Tagged patterns will return member nodes
            if (op && !VN_IS(op, PatMember)) {
                op = new AstPatMember(make_fileline(obj_h), op, nullptr, nullptr);
            }
            if (itemsp == nullptr) {
                itemsp = op;
            } else {
                itemsp->addNextNull(op);
            }
        }
        return new AstPattern(make_fileline(obj_h), itemsp);
    }
    case vpiNullOp: {
        return nullptr;
    }
    default: {
        v3error("\t! Encountered unhandled operation: " << operation);
        break;
    }
    }
    return nullptr;
}

AstNode* process_assignment(vpiHandle obj_h, UhdmShared& shared) {
    AstNode* lvaluep = nullptr;
    AstNode* rvaluep = nullptr;
    const unsigned int objectType = vpi_get(vpiType, obj_h);
    const unsigned int operationType = vpi_get(vpiOpType, obj_h);

    // Right
    visit_one_to_one({vpiRhs}, obj_h, shared, [&](AstNode* childp) { rvaluep = childp; });

    // Left
    visit_one_to_one({vpiLhs}, obj_h, shared, [&](AstNode* childp) { lvaluep = childp; });

    const unsigned int arithmeticAssignmentOpTypes[]
        = {vpiBitAndOp, vpiBitOrOp,       vpiBitXorOp, vpiBitXnorOp, vpiLogAndOp,
           vpiLogOrOp,  vpiSubOp,         vpiAddOp,    vpiMultOp,    vpiDivOp,
           vpiModOp,    vpiArithLShiftOp, vpiLShiftOp, vpiRShiftOp,  vpiArithRShiftOp};
    if (std::find(std::begin(arithmeticAssignmentOpTypes), std::end(arithmeticAssignmentOpTypes),
                  operationType)
        != std::end(arithmeticAssignmentOpTypes)) {
        rvaluep = process_operation(obj_h, shared, {lvaluep, rvaluep});
        lvaluep = lvaluep->cloneTree(true);
    }

    if (rvaluep != nullptr && rvaluep->type() == AstType::en::atFOpen) {
        // Not really an assignment, AstFOpen node takes care of the lhs
        return rvaluep;
    }
    if (lvaluep && rvaluep) {
        if (objectType == vpiAssignment) {
            auto blocking = vpi_get(vpiBlocking, obj_h);
            if (blocking) {
                return new AstAssign(make_fileline(obj_h), lvaluep, rvaluep);
            } else {
                return new AstAssignDly(make_fileline(obj_h), lvaluep, rvaluep);
            }
        } else {
            AstNode* assignp;
            if (lvaluep->type() == AstType::en::atVar) {
                // This is not a true assignment
                // Set initial value to a variable and return it
                AstVar* varp = static_cast<AstVar*>(lvaluep);
                varp->valuep(rvaluep);
                return varp;
            } else {
                if (objectType == vpiContAssign)
                    assignp = new AstAssignW(make_fileline(obj_h), lvaluep, rvaluep);
                else
                    assignp = new AstAssign(make_fileline(obj_h), lvaluep, rvaluep);
                return assignp;
            }
        }
    }
    v3error("Failed to handle assignment");
    return nullptr;
}

AstNode* process_function(vpiHandle obj_h, UhdmShared& shared) {
    AstNode* statementsp = nullptr;
    AstNode* functionVarsp = nullptr;
    AstNodeFTask* taskFuncp = nullptr;
    shared.isFunction = false;

    std::string objectName = get_object_name(obj_h);

    auto return_h = vpi_handle(vpiReturn, obj_h);
    if (return_h) {
        AstNodeDType* dtypep = getDType(make_fileline(obj_h), return_h, shared);
        // Implicit return type is always logic.
        // If we see another type here, it must be a function.
        // If not, we will check for vpiReturn when visiting statements below.
        if (auto basicp = dtypep->basicp()) {
            if (basicp->keyword() != AstBasicDTypeKwd::LOGIC || basicp->rangep() != nullptr) {
                shared.isFunction = true;
            }
        }
        functionVarsp = dtypep;
    }

    visit_one_to_many({vpiIODecl, vpiVariables}, obj_h, shared, [&](AstNode* itemp) {
        if (itemp) {
            if (statementsp)
                statementsp->addNextNull(itemp);
            else
                statementsp = itemp;
        }
    });
    visit_one_to_one({vpiStmt}, obj_h, shared, [&](AstNode* itemp) {
        if (itemp) {
            if (statementsp)
                statementsp->addNextNull(itemp);
            else
                statementsp = itemp;
        }
    });

    if (shared.isFunction) {
        taskFuncp = new AstFunc(make_fileline(obj_h), objectName, statementsp, functionVarsp);
    } else {
        taskFuncp = new AstTask(make_fileline(obj_h), objectName, statementsp);
    }
    auto accessType = vpi_get(vpiAccessType, obj_h);
    if (accessType == vpiDPIExportAcc) {
        AstDpiExport* exportp = new AstDpiExport(make_fileline(obj_h), objectName, objectName);
        exportp->addNext(taskFuncp);
        v3Global.dpi(true);
        return exportp;
    } else if (accessType == vpiDPIImportAcc) {
        taskFuncp->dpiImport(true);
        v3Global.dpi(true);
        if (taskFuncp->prettyName()[0] == '$')
            shared.m_symp->reinsert(taskFuncp, nullptr, taskFuncp->prettyName());
        shared.m_symp->reinsert(taskFuncp);
    }
    return taskFuncp;
}

AstNode* process_genScopeArray(vpiHandle obj_h, UhdmShared& shared) {
    AstNode* statementsp = nullptr;
    std::string objectName = get_object_name(obj_h);

    visit_one_to_many({vpiGenScope}, obj_h, shared, [&](AstNode* itemp) {
        if (statementsp == nullptr) {
            statementsp = itemp;
        } else {
            statementsp->addNextNull(itemp);
        }
    });
    // Cheat here a little:
    // UHDM already provides us with a correct, elaborated branch only, but then we lose
    // hierarchy for named scopes. Create here an always-true scope that will be expanded
    // by Verilator later, preserving naming hierarchy.
    if (objectName != "") {
        auto* truep = new AstConst(make_fileline(obj_h), AstConst::Unsized32(), 1);
        auto* blockp = new AstBegin(make_fileline(obj_h), objectName, statementsp, true, false);
        auto* scopep = new AstGenIf(make_fileline(obj_h), truep, blockp, nullptr);
        return scopep;
    } else {
        return new AstBegin(make_fileline(obj_h), "", statementsp);
    }
}

AstNode* process_hierPath(vpiHandle obj_h, UhdmShared& shared) {
    AstNode* lhsp = nullptr;
    AstNode* rhsp = nullptr;

    visit_one_to_many({vpiActual}, obj_h, shared, [&](AstNode* childp) {
        if (lhsp == nullptr) {
            lhsp = childp;
        } else if (rhsp == nullptr) {
            rhsp = childp;
        } else {
            lhsp = new AstDot(make_fileline(obj_h), false, lhsp, rhsp);
            rhsp = childp;
        }
    });

    return new AstDot(make_fileline(obj_h), false, lhsp, rhsp);
}

AstNode* process_ioDecl(vpiHandle obj_h, UhdmShared& shared) {
    std::string objectName = get_object_name(obj_h);

    VDirection dir;
    if (const int n = vpi_get(vpiDirection, obj_h)) {
        if (n == vpiInput) {
            dir = VDirection::INPUT;
        } else if (n == vpiOutput) {
            dir = VDirection::OUTPUT;
        } else if (n == vpiInout) {
            dir = VDirection::INOUT;
        }
        // TODO: vpiMixedIO, vpiNoDirection - not encountered yet
    }
    vpiHandle typedef_h = vpi_handle(vpiTypedef, obj_h);
    AstNodeDType* dtypep = nullptr;
    if (typedef_h) {
        dtypep = getDType(make_fileline(obj_h), typedef_h, shared);
    } else {
        UINFO(7, "No typedef handle found in vpiIODecl" << std::endl);
    }
    if (dtypep == nullptr) {
        UINFO(7, "No dtype found in vpiIODecl, falling back to logic" << std::endl);
        dtypep = new AstBasicDType(make_fileline(obj_h), AstBasicDTypeKwd::LOGIC);
    }
    auto* varp = new AstVar(make_fileline(obj_h), AstVarType::PORT, objectName, VFlagChildDType(),
                            dtypep);
    varp->declDirection(dir);
    varp->direction(dir);
    return varp;
}

AstNode* process_parameter(vpiHandle obj_h, UhdmShared& shared, bool get_value) {
    AstVar* parameterp = nullptr;
    AstNode* parameterValuep = nullptr;

    std::string objectName = get_object_name(obj_h);

    if (get_value) {
        std::string fullName = get_object_name(obj_h, {vpiFullName});
        size_t colon_pos = fullName.rfind("::");
        if (colon_pos != std::string::npos) {
            AstNode* class_pkg_refp
                = get_class_package_ref_node(make_fileline(obj_h), fullName, shared);

            AstNode* var_refp = new AstParseRef(make_fileline(obj_h), VParseRefExp::en::PX_TEXT,
                                                objectName, nullptr, nullptr);
            return AstDot::newIfPkg(make_fileline(obj_h), class_pkg_refp, var_refp);
        }
    }
    if (is_imported(obj_h)) {
        // Skip imported parameters, they will still be visible in their packages
        UINFO(3, "Skipping imported parameter " << objectName << std::endl);
        return nullptr;
    }

    AstRange* rangeNodep = nullptr;
    visit_one_to_many({vpiRange}, obj_h, shared, [&](AstNode* nodep) {
        if (nodep) rangeNodep = reinterpret_cast<AstRange*>(nodep);
    });

    AstNodeDType* dtypep = nullptr;
    auto typespec_h = vpi_handle(vpiTypespec, obj_h);
    if (typespec_h) {
        dtypep = getDType(make_fileline(obj_h), typespec_h, shared);
    } else {
        UINFO(7, "No typespec found in vpiParameter " << objectName << std::endl);
    }

    // If no typespec provided assume default
    if (dtypep == nullptr) {
        dtypep = new AstBasicDType(make_fileline(obj_h), AstBasicDTypeKwd::LOGIC_IMPLICIT);
    }
    if (rangeNodep) {
        dtypep
            = new AstUnpackArrayDType(make_fileline(obj_h), VFlagChildDType(), dtypep, rangeNodep);
    }

    if (get_value) { parameterValuep = get_value_as_node(obj_h); }

    AstVarType parameterType;
    int is_local = vpi_get(vpiLocalParam, obj_h);

    if (is_local)
        parameterType = AstVarType::LPARAM;
    else
        parameterType = AstVarType::GPARAM;

    parameterp
        = new AstVar(make_fileline(obj_h), parameterType, objectName, VFlagChildDType(), dtypep);
    if (parameterValuep != nullptr) parameterp->valuep(parameterValuep);
    return parameterp;
}

AstVar* process_param_assign(vpiHandle obj_h, UhdmShared& shared) {
    AstVar* parameterp = nullptr;
    AstNode* parameterValuep = nullptr;

    visit_one_to_one({vpiRhs}, obj_h, shared, [&](AstNode* nodep) { parameterValuep = nodep; });

    vpiHandle parameter_h = vpi_handle(vpiLhs, obj_h);

    if (parameterValuep == nullptr) {
        parameterp = reinterpret_cast<AstVar*>(process_parameter(parameter_h, shared, true));
    } else {
        parameterp = reinterpret_cast<AstVar*>(process_parameter(parameter_h, shared, false));
        if (parameterp != nullptr) { parameterp->valuep(parameterValuep); }
    }

    return parameterp;
}

AstPackageImport* process_uhdm_import(vpiHandle obj_h, UhdmShared& shared) {
    std::string objectName = get_object_name(obj_h);

    AstPackage* packagep = nullptr;
    auto it = shared.package_map.find(objectName);
    if (it != shared.package_map.end()) { packagep = it->second; }
    if (packagep != nullptr) {
        std::string symbol_name;
        vpiHandle imported_name = vpi_handle(vpiImport, obj_h);
        if (imported_name) {
            s_vpi_value val;
            vpi_get_value(imported_name, &val);
            symbol_name = val.value.str;
        }
        auto* package_importp = new AstPackageImport(make_fileline(obj_h), packagep, symbol_name);
        shared.m_symp->importItem(packagep, symbol_name);
        return package_importp;
    }
    return nullptr;
}

AstMemberDType* process_typespec_member(vpiHandle obj_h, UhdmShared& shared) {
    std::string objectName = get_object_name(obj_h);
    AstNodeDType* typespecp = nullptr;
    auto typespec_h = vpi_handle(vpiTypespec, obj_h);
    typespecp = getDType(make_fileline(typespec_h), typespec_h, shared);
    if (typespecp != nullptr) {
        auto* memberp
            = new AstMemberDType(make_fileline(obj_h), objectName, VFlagChildDType(), typespecp);
        return memberp;
    }
    return nullptr;
}

AstNode* process_typespec(vpiHandle obj_h, UhdmShared& shared) {
    std::string objectName = get_object_name(obj_h);
    const unsigned int objectType = vpi_get(vpiType, obj_h);

    auto file_name = vpi_get_str(vpiFile, obj_h);
    const unsigned int currentLine = vpi_get(vpiLineNo, obj_h);
    UINFO(6, __func__ << ": Object: " << objectName << " of type " << objectType << " ("
                      << UHDM::VpiTypeName(obj_h) << ")"
                      << " @ " << currentLine << " : " << (file_name != 0 ? file_name : "?")
                      << std::endl);
    if (file_name) {
        shared.coverage_set.insert({file_name, currentLine, UHDM::VpiTypeName(obj_h)});
    }

    FileLine* fl = make_fileline(obj_h);

    switch (objectType) {
    case vpiBitTypespec:
    case vpiLogicTypespec: {
        return getDType(fl, obj_h, shared);
    }
    case vpiIntTypespec: {
        auto* name = vpi_get_str(vpiName, obj_h);
        if (name == nullptr) {
            auto* dtypep = new AstBasicDType(fl, AstBasicDTypeKwd::INT);
            return dtypep;
        }
        return new AstParseRef(fl, VParseRefExp::en::PX_TEXT, name, nullptr, nullptr);
    }
    case vpiStringTypespec: {
        auto* dtypep = new AstBasicDType(fl, AstBasicDTypeKwd::STRING);
        return dtypep;
    }
    case vpiChandleTypespec: {
        auto* dtypep = new AstBasicDType(fl, AstBasicDTypeKwd::CHANDLE);
        return dtypep;
    }
    case vpiIntegerTypespec: {
        AstNode* constNode = get_value_as_node(obj_h);
        if (constNode == nullptr) {
            v3info("Valueless typepec, returning dtype");
            auto* dtypep = new AstBasicDType(fl, AstBasicDTypeKwd::INTEGER);
            return dtypep;
        }
        return constNode;
    }
    case vpiVoidTypespec: {
        return new AstVoidDType(fl);
    }
    case vpiEnumTypespec: {
        const uhdm_handle* const handle = (const uhdm_handle*)obj_h;
        const UHDM::BaseClass* const object = (const UHDM::BaseClass*)handle->object;
        if (shared.visited_types.find(object) != shared.visited_types.end()) {
            // Already seen this, do not create a duplicate
            // References are handled using getDType, not in visit_object
            return nullptr;
        }

        shared.visited_types[object] = objectName;

        // Use bare name for typespec itself, hierarchy was stored above
        auto pos = objectName.rfind("::");
        if (pos != std::string::npos) objectName = objectName.substr(pos + 2);

        AstNode* enum_members = nullptr;
        AstNodeDType* enum_member_dtype = nullptr;

        vpiHandle itr = vpi_iterate(vpiEnumConst, obj_h);
        while (vpiHandle item_h = vpi_scan(itr)) {
            std::string item_name = get_object_name(item_h);

            auto* value = get_value_as_node(item_h, false);
            auto* wrapped_item = new AstEnumItem(make_fileline(item_h), item_name, nullptr, value);
            if (enum_members == nullptr) {
                enum_members = wrapped_item;
            } else {
                enum_members->addNextNull(wrapped_item);
            }
        }
        vpi_free_object(itr);

        visit_one_to_one({vpiBaseTypespec}, obj_h, shared, [&](AstNode* item) {
            if (item != nullptr) { enum_member_dtype = reinterpret_cast<AstNodeDType*>(item); }
        });
        if (enum_member_dtype == nullptr) {
            // No data type specified, use default
            enum_member_dtype = new AstBasicDType(fl, AstBasicDTypeKwd::INT);
        }
        auto* enum_dtype
            = new AstEnumDType(fl, VFlagChildDType(), enum_member_dtype, enum_members);
        auto* dtype
            = new AstDefImplicitDType(fl, objectName, nullptr, VFlagChildDType(), enum_dtype);
        return dtype;
    }
    case vpiStructTypespec: {
        const uhdm_handle* const handle = (const uhdm_handle*)obj_h;
        const UHDM::BaseClass* const object = (const UHDM::BaseClass*)handle->object;
        if (shared.visited_types.find(object) != shared.visited_types.end()) {
            UINFO(6, "Object " << objectName << " was already visited" << std::endl);
            return nullptr;
        }

        shared.visited_types[object] = objectName;

        // Use bare name for typespec itself, hierarchy was stored above
        auto pos = objectName.rfind("::");
        if (pos != std::string::npos) objectName = objectName.substr(pos + 2);

        // VSigning below is used in AstStructDtype to indicate
        // if packed or not
        VSigning packed;
        if (vpi_get(vpiPacked, obj_h)) {
            packed = VSigning::SIGNED;
        } else {
            packed = VSigning::UNSIGNED;
        }
        auto* struct_dtype = new AstStructDType(fl, packed);

        vpiHandle member_itr = vpi_iterate(vpiTypespecMember, obj_h);
        while (vpiHandle member_obj = vpi_scan(member_itr)) {
            AstMemberDType* memberp = process_typespec_member(member_obj, shared);
            if (memberp != nullptr) struct_dtype->addMembersp(memberp);
        }

        auto* dtype
            = new AstDefImplicitDType(fl, objectName, nullptr, VFlagChildDType(), struct_dtype);
        return dtype;
    }
    case vpiPackedArrayTypespec:
    case vpiArrayTypespec: {
        return getDType(fl, obj_h, shared);
    }
    case vpiUnsupportedTypespec: {
        v3info("\t! This typespec is unsupported in UHDM: " << file_name << ":" << currentLine);
        // Create a reference and try to resolve later
        return get_referenceNode(make_fileline(obj_h), objectName);
    }
    }
    return nullptr;
}

AstNode* process_typedef(vpiHandle obj_h, UhdmShared& shared) {
    const unsigned int type = vpi_get(vpiType, obj_h);
    if (type == UHDM::uhdmimport) {
        // imports are under vpiTypedef nodes, but they are not defined types
        return process_uhdm_import(obj_h, shared);
    }

    std::string objectName = get_object_name(obj_h);

    auto pos = objectName.rfind("::");
    if (pos != std::string::npos) objectName = objectName.substr(pos + 2);

    AstNodeDType* refp = nullptr;
    if (vpiHandle alias_h = vpi_handle(vpiTypedefAlias, obj_h)) {
        refp = getDType(make_fileline(alias_h), alias_h, shared);
    } else {
        refp = reinterpret_cast<AstNodeDType*>(process_typespec(obj_h, shared));
        if (refp == nullptr) {
            UINFO(7, "Typedef revisited: " << objectName << ", skipping" << std::endl);
            return nullptr;
        }
    }

    AstTypedef* typedefp
        = new AstTypedef(make_fileline(obj_h), objectName, nullptr, VFlagChildDType(), refp);

    shared.m_symp->reinsert(typedefp);
    return typedefp;
}

AstNode* visit_object(vpiHandle obj_h, UhdmShared& shared) {
    // Will keep current node
    AstNode* node = nullptr;

    // Current object data
    int lineNo = 0;

    // For iterating over child objects
    vpiHandle itr;

    std::string objectName = get_object_name(obj_h, {vpiName, vpiFullName, vpiDefName});
    std::string fullObjectName = get_object_name(obj_h, {vpiFullName});

    auto file_name = vpi_get_str(vpiFile, obj_h);

    if (unsigned int l = vpi_get(vpiLineNo, obj_h)) { lineNo = l; }

    const unsigned int currentLine = vpi_get(vpiLineNo, obj_h);
    const unsigned int objectType = vpi_get(vpiType, obj_h);
    UINFO(6, "Object: " << objectName << " of type " << objectType << " ("
                        << UHDM::VpiTypeName(obj_h) << ")"
                        << " @ " << currentLine << " : " << (file_name != 0 ? file_name : "?")
                        << std::endl);
    if (file_name) {
        shared.coverage_set.insert({file_name, currentLine, UHDM::VpiTypeName(obj_h)});
    }

    switch (objectType) {
    case vpiDesign: {

        visit_one_to_many({UHDM::uhdmallInterfaces, UHDM::uhdmtopModules}, obj_h, shared,
                          [&](AstNode* module) {
                              if (module != nullptr) { node = module; }
                          });
        return node;
    }
    case vpiPackage: {
        auto* package = new AstPackage(new FileLine(objectName), objectName);
        package->inLibrary(true);
        shared.package_prefix = objectName + "::";
        shared.m_symp->pushNew(package);

        vpiHandle typedef_itr = vpi_iterate(vpiTypedef, obj_h);
        while (vpiHandle typedef_obj = vpi_scan(typedef_itr)) {
            AstNode* typedefp = process_typedef(typedef_obj, shared);
            if (typedefp != nullptr) package->addStmtp(typedefp);
        }

        visit_one_to_many(
            {
                vpiParamAssign,
                vpiProgram,
                vpiProgramArray,
                vpiTaskFunc,
                vpiSpecParam,
                vpiAssertion,
            },
            obj_h, shared, [&](AstNode* item) {
                if (item != nullptr) { package->addStmtp(item); }
            });
        shared.m_symp->popScope(package);
        shared.package_prefix = shared.package_prefix.substr(0, shared.package_prefix.length()
                                                                    - (objectName.length() + 2));

        shared.package_map[objectName] = package;
        return package;
    }
    case vpiPort: {
        static unsigned numPorts;
        AstPort* port = nullptr;
        AstVar* var = nullptr;

        AstNodeDType* dtype = nullptr;
        auto parent_h = vpi_handle(vpiParent, obj_h);
        std::string netName = "";
        for (auto net : {vpiNet, vpiNetArray, vpiArrayVar, vpiArrayNet, vpiVariables}) {
            vpiHandle itr = vpi_iterate(net, parent_h);
            while (vpiHandle vpi_child_obj = vpi_scan(itr)) {
                netName = get_object_name(vpi_child_obj);
                UINFO(7, "Net name is " << netName << std::endl);
                if (netName == objectName) {
                    UINFO(7, "Found matching net for " << objectName << std::endl);
                    dtype = getDType(make_fileline(obj_h), vpi_child_obj, shared);
                    break;
                }
                vpi_free_object(vpi_child_obj);
            }
            vpi_free_object(itr);
        }
        if (dtype == nullptr) {
            // If no matching net was found, get info from port node connections
            // This is the case for interface ports
            dtype = getDType(make_fileline(obj_h), obj_h, shared);
        }
        if (VN_IS(dtype, IfaceRefDType)) {
            var = new AstVar(make_fileline(obj_h), AstVarType::IFACEREF, objectName,
                             VFlagChildDType(), dtype);
        } else {
            var = new AstVar(make_fileline(obj_h), AstVarType::PORT, objectName, VFlagChildDType(),
                             dtype);

            if (const int n = vpi_get(vpiDirection, obj_h)) {
                v3info("Got direction for " << objectName);
                if (n == vpiInput) {
                    var->declDirection(VDirection::INPUT);
                    var->direction(VDirection::INPUT);
                    var->varType(AstVarType::PORT);
                } else if (n == vpiOutput) {
                    var->declDirection(VDirection::OUTPUT);
                    var->direction(VDirection::OUTPUT);
                    var->varType(AstVarType::PORT);
                } else if (n == vpiInout) {
                    var->declDirection(VDirection::INOUT);
                    var->direction(VDirection::INOUT);
                }
            } else {
                v3info("Got no direction for " << objectName << ", skipping");
                return nullptr;
            }
        }

        port = new AstPort(make_fileline(obj_h), ++numPorts, objectName);
        port->addNextNull(var);

        if (v3Global.opt.trace()) { var->trace(true); }

        return port;
    }
    case UHDM::uhdmimport: {
        return process_uhdm_import(obj_h, shared);
    }
    case vpiModule: {
        std::string modType = get_object_name(obj_h, {vpiDefName});
        std::string name = objectName;
        AstModule* module;

        // Check if we have encountered this object before
        auto it = shared.partial_modules.find(modType);
        auto param_it = shared.top_param_map.find(modType);
        if (it != shared.partial_modules.end()) {
            // Was created before, fill missing
            module = reinterpret_cast<AstModule*>(it->second);
            // If available, check vpiFullName instead of vpiName, as vpiName can equal vpiDefName
            std::string fullName = name;
            if (auto* s = vpi_get_str(vpiFullName, obj_h)) {
                fullName = s;
                sanitize_str(fullName);
            }
            if (fullName != modType) {
                static int module_counter;
                // Not a top module, create separate node with proper params
                module = module->cloneTree(false);
                module->user4p(nullptr);  // Clear SymEnt
                // Use more specific name
                name = modType + "_" + objectName + std::to_string(module_counter++);
            }
            module->name(name);
            shared.m_symp->pushNew(module);
            visit_one_to_many(
                {
                    vpiPort,
                    vpiInterface,
                    vpiInterfaceArray,
                    vpiProcess,
                    vpiContAssign,
                    vpiModule,
                    vpiModuleArray,
                    vpiPrimitive,
                    vpiPrimitiveArray,
                    vpiModPath,
                    vpiTchk,
                    vpiDefParam,
                    vpiIODecl,
                    vpiAliasStmt,
                    vpiClockingBlock,
                    vpiNet,
                    vpiGenScopeArray,
                    vpiArrayNet,

                    // from vpiInstance
                    vpiProgram,
                    vpiProgramArray,
                    vpiTaskFunc,
                    vpiSpecParam,
                    vpiAssertion,
                    // vpiClassDefn,

                    // from vpiScope
                    vpiPropertyDecl,
                    vpiSequenceDecl,
                    vpiConcurrentAssertions,
                    vpiNamedEvent,
                    vpiNamedEventArray,
                    vpiVariables,
                    vpiVirtualInterfaceVar,
                    vpiReg,
                    vpiRegArray,
                    vpiMemory,
                    vpiInternalScope,
                    vpiAttribute,
                },
                obj_h, shared, [&](AstNode* node) {
                    if (node != nullptr) module->addStmtp(node);
                });
            // Update parameter values using TopModules tree
            if (param_it != shared.top_param_map.end()) {
                auto param_map = param_it->second;
                visit_one_to_many({vpiParamAssign}, obj_h, shared, [&](AstNode* node) {
                    if (VN_IS(node, Var)) {
                        AstVar* param_node = VN_CAST(node, Var);
                        // Global parameters are added as pins, skip them here
                        if (param_node->varType() == AstVarType::LPARAM)
                            param_map[node->name()] = node;
                    }
                });
                // Add final values of parameters
                for (auto& param_p : param_map) {
                    if (param_p.second != nullptr)
                        module->addStmtp(param_p.second->cloneTree(false));
                }
            }
            (shared.top_nodes)[name] = module;
            shared.m_symp->popScope(module);
        } else {
            // Encountered for the first time
            module = new AstModule(new FileLine(modType), modType);

            vpiHandle typedef_itr = vpi_iterate(vpiTypedef, obj_h);
            while (vpiHandle typedef_obj = vpi_scan(typedef_itr)) {
                AstNode* typedefp = process_typedef(typedef_obj, shared);
                if (typedefp != nullptr) module->addStmtp(typedefp);
            }

            visit_one_to_many(
                {
                    vpiModule,
                    vpiContAssign,
                    vpiProcess,
                    vpiTaskFunc,
                },
                obj_h, shared, [&](AstNode* node) {
                    if (node != nullptr) module->addStmtp(node);
                });

            NameNodeMap param_map;
            visit_one_to_many({vpiParamAssign}, obj_h, shared, [&](AstNode* node) {
                if (node != nullptr) param_map[node->name()] = node;
            });
            (shared.partial_modules)[module->name()] = module;
            if (v3Global.opt.trace()) { module->modTrace(true); }
            shared.top_param_map[module->name()] = param_map;
        }

        // If available, check vpiFullName instead of vpiName, as vpiName can equal vpiDefName
        std::string fullName = name;
        if (auto* s = vpi_get_str(vpiFullName, obj_h)) {
            fullName = s;
            sanitize_str(fullName);
        }
        if (fullName != modType) {
            // Not a top module, create instance
            AstPin* modPins = nullptr;
            AstPin* modParams = nullptr;

            // Get port assignments
            vpiHandle itr = vpi_iterate(vpiPort, obj_h);
            int np = 0;
            while (vpiHandle vpi_child_obj = vpi_scan(itr)) {
                vpiHandle highConn = vpi_handle(vpiHighConn, vpi_child_obj);
                std::string portName = vpi_get_str(vpiName, vpi_child_obj);
                sanitize_str(portName);
                AstNode* ref = nullptr;
                if (highConn) { ref = visit_object(highConn, shared); }
                AstPin* pin = new AstPin(make_fileline(obj_h), ++np, portName, ref);
                if (!modPins)
                    modPins = pin;
                else
                    modPins->addNextNull(pin);

                vpi_free_object(vpi_child_obj);
            }
            vpi_free_object(itr);

            // Get parameter assignments
            itr = vpi_iterate(vpiParamAssign, obj_h);
            std::set<std::string> parameter_set;
            while (vpiHandle vpi_child_obj = vpi_scan(itr)) {
                vpiHandle param_handle = vpi_handle(vpiLhs, vpi_child_obj);
                std::string param_name = get_object_name(param_handle);
                UINFO(3, "Got parameter (pin) " << param_name << std::endl);
                auto is_local = vpi_get(vpiLocalParam, param_handle);
                AstNode* value = nullptr;
                // Try to construct complex expression
                visit_one_to_one({vpiRhs}, vpi_child_obj, shared, [&](AstNode* node) {
                    if (node != nullptr)
                        value = node;
                    else
                        v3error("No value for parameter: " << param_name);
                });
                if (is_local) {
                    // Skip local parameters
                    UINFO(3, "Skipping local parameter (pin) " << param_name << std::endl);
                    continue;
                }
                if (is_imported(param_handle)) {
                    // Skip imported parameters when creating cells
                    UINFO(3, "Skipping imported parameter (pin) " << param_name << std::endl);
                    continue;
                }
                if (parameter_set.find(param_name) == parameter_set.end()) {
                    // Although those are parameters, they are stored as pins
                    AstPin* pin = new AstPin(make_fileline(obj_h), ++np, param_name, value);
                    if (!modParams)
                        modParams = pin;
                    else
                        modParams->addNextNull(pin);
                    parameter_set.insert(param_name);
                } else {
                    // Duplicate
                    UINFO(3, "Duplicate parameter assignment: " << param_name << " in "
                                                                << objectName << std::endl);
                    continue;
                }
                vpi_free_object(vpi_child_obj);
            }
            vpi_free_object(itr);
            std::string fullname = get_object_name(obj_h, {vpiFullName});
            UINFO(8, "Adding cell " << fullname << std::endl);
            AstCell* cell = new AstCell(make_fileline(obj_h), make_fileline(obj_h), objectName,
                                        name, modPins, modParams, nullptr);
            if (v3Global.opt.trace()) { cell->trace(true); }
            return cell;
        }
        break;
    }
    case vpiAssignment:
    case vpiAssignStmt:
    case vpiContAssign: {
        return process_assignment(obj_h, shared);
    }
    case vpiHierPath: {
        return process_hierPath(obj_h, shared);
    }
    case vpiRefObj: {
        return get_referenceNode(make_fileline(obj_h), objectName);
    }
    case vpiNetArray: {  // also defined as vpiArrayNet
        // vpiNetArray is used for unpacked arrays
        AstVar* vpi_net = nullptr;
        visit_one_to_many({vpiNet}, obj_h, shared, [&](AstNode* node) {
            if ((node != nullptr) && (vpi_net == nullptr)) {
                vpi_net = reinterpret_cast<AstVar*>(node);
            }
        });

        auto dtypep = getDType(make_fileline(obj_h), obj_h, shared);
        AstVar* v = new AstVar(make_fileline(obj_h), vpi_net->varType(), objectName,
                               VFlagChildDType(), dtypep);
        if (v3Global.opt.trace()) { v->trace(true); }
        return v;
    }

    case vpiEnumNet:
    case vpiStructNet:
    case vpiIntegerNet:
    case vpiTimeNet:
    case vpiPackedArrayNet:
    case vpiLogicNet: {  // also defined as vpiNet
        AstNodeDType* dtype = nullptr;
        AstVarType net_type = AstVarType::VAR;
        AstBasicDTypeKwd dtypeKwd = AstBasicDTypeKwd::LOGIC_IMPLICIT;
        vpiHandle obj_net;
        dtype = getDType(make_fileline(obj_h), obj_h, shared);

        if (net_type == AstVarType::UNKNOWN && dtype == nullptr) {
            // Not set in case above, most likely a "false" port net
            return nullptr;  // Skip this net
        }

        auto* v = new AstVar(make_fileline(obj_h), net_type, objectName, VFlagChildDType(), dtype);
        if (v3Global.opt.trace()) { v->trace(true); }
        return v;
    }
    case vpiStructVar: {
        AstNodeDType* dtype = getDType(make_fileline(obj_h), obj_h, shared);

        auto* v = new AstVar(make_fileline(obj_h), AstVarType::VAR, objectName, VFlagChildDType(),
                             dtype);
        if (v3Global.opt.trace()) { v->trace(true); }
        return v;
    }
    case vpiParameter: {
        return process_parameter(obj_h, shared, true);
    }
    case vpiParamAssign: {
        return process_param_assign(obj_h, shared);
    }
    case vpiInterface: {
        // Interface definition is represented by a module node
        AstIface* elaboratedInterface = new AstIface(make_fileline(obj_h), objectName);
        bool hasModports = false;
        visit_one_to_many(
            {
                vpiPort,
                vpiParamAssign,
                vpiInterfaceTfDecl,
                vpiModPath,
                vpiContAssign,
                vpiInterface,
                vpiInterfaceArray,
                vpiProcess,
                vpiGenScopeArray,

                // from vpiInstance
                vpiProgram,
                vpiProgramArray,
                vpiTaskFunc,
                vpiArrayNet,
                vpiSpecParam,
                vpiAssertion,
                vpiNet,
            },
            obj_h, shared, [&](AstNode* port) {
                if (port) { elaboratedInterface->addStmtp(port); }
            });
        visit_one_to_many({vpiModport}, obj_h, shared, [&](AstNode* port) {
            if (port) {
                hasModports = true;
                elaboratedInterface->addStmtp(port);
            }
        });
        if (hasModports) {
            // Only then create the nets, as they won't be connected otherwise
            visit_one_to_many({vpiNet}, obj_h, shared, [&](AstNode* port) {
                if (port) { elaboratedInterface->addStmtp(port); }
            });
        }

        elaboratedInterface->name(objectName);
        std::string modType = vpi_get_str(vpiDefName, obj_h);
        sanitize_str(modType);
        if (objectName != modType) {
            AstPin* modPins = nullptr;
            vpiHandle itr = vpi_iterate(vpiPort, obj_h);
            int np = 0;
            while (vpiHandle vpi_child_obj = vpi_scan(itr)) {
                vpiHandle highConn = vpi_handle(vpiHighConn, vpi_child_obj);
                if (highConn) {
                    std::string portName = get_object_name(vpi_child_obj);
                    AstParseRef* ref
                        = reinterpret_cast<AstParseRef*>(visit_object(highConn, shared));
                    AstPin* pin = new AstPin(make_fileline(vpi_child_obj), ++np, portName, ref);
                    if (!modPins)
                        modPins = pin;
                    else
                        modPins->addNextNull(pin);
                }

                vpi_free_object(vpi_child_obj);
            }
            vpi_free_object(itr);

            AstCell* cell = new AstCell(make_fileline(obj_h), new FileLine("uhdm"), objectName,
                                        modType, modPins, nullptr, nullptr);
            return cell;
        } else {
            // is top level
            return elaboratedInterface;
        }
        break;
    }
    case vpiModport: {
        AstNode* modport_vars = nullptr;

        // We construct a reference here, the net is created in the interface
        // No full visit, just grab name and direction
        auto io_itr = vpi_iterate(vpiIODecl, obj_h);
        while (vpiHandle io_h = vpi_scan(io_itr)) {
            std::string io_name = get_object_name(io_h);
            VDirection dir;
            if (const int n = vpi_get(vpiDirection, io_h)) {
                if (n == vpiInput) {
                    dir = VDirection::INPUT;
                } else if (n == vpiOutput) {
                    dir = VDirection::OUTPUT;
                } else if (n == vpiInout) {
                    dir = VDirection::INOUT;
                }
            }
            auto* io_node = new AstModportVarRef(make_fileline(io_h), io_name, dir);
            if (modport_vars)
                modport_vars->addNextNull(io_node);
            else
                modport_vars = io_node;
            vpi_free_object(io_h);
        }
        vpi_free_object(io_itr);

        return new AstModport(make_fileline(obj_h), objectName, modport_vars);
    }
    case vpiIODecl: {
        return process_ioDecl(obj_h, shared);
    }
    case vpiAlways: {
        VAlwaysKwd alwaysType;
        AstSenTree* senTree = nullptr;
        AstNode* body = nullptr;

        // Which always type is it?
        switch (vpi_get(vpiAlwaysType, obj_h)) {
        case vpiAlways: {
            alwaysType = VAlwaysKwd::ALWAYS;
            break;
        }
        case vpiAlwaysFF: {
            alwaysType = VAlwaysKwd::ALWAYS_FF;
            break;
        }
        case vpiAlwaysLatch: {
            alwaysType = VAlwaysKwd::ALWAYS_LATCH;
            break;
        }
        case vpiAlwaysComb: {
            alwaysType = VAlwaysKwd::ALWAYS_COMB;
            break;
        }
        default: {
            v3error("Unhandled always type");
            break;
        }
        }

        if (alwaysType != VAlwaysKwd::ALWAYS_COMB && alwaysType != VAlwaysKwd::ALWAYS_LATCH) {
            // Handled in vpiEventControl
            AstNode* always;

            visit_one_to_one({vpiStmt}, obj_h, shared, [&](AstNode* node) { always = node; });
            return always;
        } else {
            // Body of statements
            visit_one_to_one({vpiStmt}, obj_h, shared, [&](AstNode* node) { body = node; });
        }

        return new AstAlways(make_fileline(obj_h), alwaysType, senTree, body);
    }
    case vpiEventControl: {
        AstSenItem* senItemRoot = nullptr;
        AstNode* body = nullptr;
        AstSenTree* senTree = nullptr;
        visit_one_to_one({vpiCondition}, obj_h, shared, [&](AstNode* node) {
            if (node->type() == AstType::en::atSenItem) {
                senItemRoot = reinterpret_cast<AstSenItem*>(node);
            } else {  // wrap this in a AstSenItem
                senItemRoot = new AstSenItem(node->fileline(), VEdgeType::ET_ANYEDGE, node);
            }
        });
        senTree = new AstSenTree(make_fileline(obj_h), senItemRoot);
        // Body of statements
        visit_one_to_one({vpiStmt}, obj_h, shared, [&](AstNode* node) { body = node; });
        auto* tctrl = new AstTimingControl(make_fileline(obj_h), senTree, body);
        return new AstAlways(make_fileline(obj_h), VAlwaysKwd::ALWAYS_FF, nullptr, tctrl);
    }
    case vpiInitial: {
        AstNode* body = nullptr;
        visit_one_to_one({vpiStmt}, obj_h, shared, [&](AstNode* node) { body = node; });
        return new AstInitial(make_fileline(obj_h), body);
    }
    case vpiFinal: {
        AstNode* body = nullptr;
        visit_one_to_one({vpiStmt}, obj_h, shared, [&](AstNode* node) { body = node; });
        return new AstFinal(make_fileline(obj_h), body);
    }
    case vpiNamedBegin:
    case vpiBegin: {
        AstNode* body = nullptr;

        vpiHandle typedef_itr = vpi_iterate(vpiTypedef, obj_h);
        while (vpiHandle typedef_obj = vpi_scan(typedef_itr)) {
            AstNode* typedefp = process_typedef(typedef_obj, shared);
            if (body == nullptr)
                body = typedefp;
            else
                body->addNextNull(typedefp);
        }

        visit_one_to_many(
            {
                vpiStmt,
                vpiPropertyDecl,
                vpiSequenceDecl,
                vpiConcurrentAssertions,
                vpiNamedEvent,
                vpiNamedEventArray,
                vpiVariables,
                vpiVirtualInterfaceVar,
                vpiReg,
                vpiRegArray,
                vpiMemory,
                vpiParamAssign,
                vpiInternalScope,
                vpiAttribute,
            },
            obj_h, shared, [&](AstNode* node) {
                if (body == nullptr) {
                    body = node;
                } else {
                    body->addNextNull(node);
                }
            });
        if (objectType == vpiBegin) {
            objectName = "";  // avoid storing parent name
        }
        return new AstBegin(make_fileline(obj_h), "", body);
    }
    case vpiIf:
    case vpiIfElse: {
        AstNode* condition = nullptr;
        AstNode* statement = nullptr;
        AstNode* elseStatement = nullptr;

        visit_one_to_one({vpiCondition}, obj_h, shared, [&](AstNode* node) { condition = node; });
        visit_one_to_one({vpiStmt}, obj_h, shared, [&](AstNode* node) { statement = node; });
        if (objectType == vpiIfElse) {
            visit_one_to_one({vpiElseStmt}, obj_h, shared,
                             [&](AstNode* node) { elseStatement = node; });
        }
        return new AstIf(make_fileline(obj_h), condition, statement, elseStatement);
    }
    case vpiCase: {
        VCaseType case_type;
        switch (vpi_get(vpiCaseType, obj_h)) {
        case vpiCaseExact: {
            case_type = VCaseType::en::CT_CASE;
            break;
        }
        case vpiCaseX: {
            case_type = VCaseType::en::CT_CASEX;
            break;
        }
        case vpiCaseZ: {
            case_type = VCaseType::en::CT_CASEZ;
            break;
        }
        default: {
            // Should never be reached
            break;
        }
        }
        AstNode* conditionNode = nullptr;
        visit_one_to_one({vpiCondition}, obj_h, shared,
                         [&](AstNode* node) { conditionNode = node; });
        AstNode* itemNodes = nullptr;
        visit_one_to_many({vpiCaseItem}, obj_h, shared, [&](AstNode* item) {
            if (item) {
                if (itemNodes == nullptr) {
                    itemNodes = item;
                } else {
                    itemNodes->addNextNull(item);
                }
            }
        });
        return new AstCase(make_fileline(obj_h), case_type, conditionNode, itemNodes);
    }
    case vpiCaseItem: {
        AstNode* expressionNode = nullptr;
        visit_one_to_many({vpiExpr}, obj_h, shared, [&](AstNode* item) {
            if (item) {
                if (expressionNode == nullptr) {
                    expressionNode = item;
                } else {
                    expressionNode->addNextNull(item);
                }
            }
        });
        AstNode* bodyNode = nullptr;
        visit_one_to_one({vpiStmt}, obj_h, shared, [&](AstNode* node) { bodyNode = node; });
        return new AstCaseItem(make_fileline(obj_h), expressionNode, bodyNode);
    }
    case vpiOperation: {
        std::vector<AstNode*> operands;
        visit_one_to_many({vpiOperand}, obj_h, shared, [&](AstNode* itemp) {
            if (itemp) { operands.push_back(itemp); }
        });
        return process_operation(obj_h, shared, operands);
    }
    case vpiTaggedPattern: {
        AstNode* typespec = nullptr;
        AstNode* pattern = nullptr;
        auto typespec_h = vpi_handle(vpiTypespec, obj_h);
        std::string pattern_name;
        if (auto s = vpi_get_str(vpiName, typespec_h)) { pattern_name = s; }
        sanitize_str(pattern_name);
        typespec = new AstText(make_fileline(obj_h), pattern_name);

        visit_one_to_one({vpiPattern}, obj_h, shared, [&](AstNode* node) { pattern = node; });
        if (pattern_name == "default") {
            auto* patm = new AstPatMember(make_fileline(obj_h), pattern, nullptr, nullptr);
            patm->isDefault(true);
            return patm;
        } else {
            return new AstPatMember(make_fileline(obj_h), pattern, typespec, nullptr);
        }
    }
    case vpiEnumConst: {
        return get_value_as_node(obj_h, false);
    }
    case vpiConstant: {
        return get_value_as_node(obj_h, true);
    }
    case vpiBitSelect: {
        AstNode* refp = get_class_package_ref_node(make_fileline(obj_h), objectName, shared);
        size_t colon_pos = objectName.rfind("::");
        if (colon_pos != std::string::npos) objectName = objectName.substr(colon_pos + 2);

        auto* fromp = get_referenceNode(make_fileline(obj_h), objectName);
        AstNode* bitp = nullptr;
        visit_one_to_one({vpiIndex}, obj_h, shared, [&](AstNode* item) {
            if (item) { bitp = item; }
        });

        AstNode* selbitp = new AstSelBit(make_fileline(obj_h), fromp, bitp);

        return AstDot::newIfPkg(make_fileline(obj_h), refp, selbitp);
    }
    case vpiVarSelect: {
        AstNode* refp = get_class_package_ref_node(make_fileline(obj_h), objectName, shared);
        size_t colon_pos = objectName.rfind("::");
        if (colon_pos != std::string::npos) objectName = objectName.substr(colon_pos + 2);

        auto* fromp = get_referenceNode(make_fileline(obj_h), objectName);
        AstNode* bitp = nullptr;
        AstNode* select = nullptr;
        visit_one_to_many({vpiIndex}, obj_h, shared, [&](AstNode* item) {
            bitp = item;
            if (item->type() == AstType::en::atSelExtract) {
                select = new AstSelExtract(make_fileline(obj_h), fromp,
                                           ((AstSelExtract*)item)->msbp()->cloneTree(true),
                                           ((AstSelExtract*)item)->lsbp()->cloneTree(true));
            } else if (item->type() == AstType::en::atConst) {
                select = new AstSelBit(make_fileline(obj_h), fromp, bitp);
            } else if (item->type() == AstType::atSelPlus) {
                AstSelPlus* selplusp = VN_CAST(item, SelPlus);
                select = new AstSelPlus(make_fileline(obj_h), fromp,
                                        selplusp->bitp()->cloneTree(true),
                                        selplusp->widthp()->cloneTree(true));
            } else if (item->type() == AstType::atSelMinus) {
                AstSelMinus* selminusp = VN_CAST(item, SelMinus);
                select = new AstSelMinus(make_fileline(obj_h), fromp,
                                         selminusp->bitp()->cloneTree(true),
                                         selminusp->widthp()->cloneTree(true));

            } else {
                select = new AstSelBit(make_fileline(obj_h), fromp, bitp);
            }
            fromp = select;
        });
        return AstDot::newIfPkg(make_fileline(obj_h), refp, select);
    }
    case vpiTask: {
        AstNode* statements = nullptr;
        visit_one_to_many({vpiIODecl}, obj_h, shared, [&](AstNode* item) {
            if (item) {
                // Overwrite direction for arguments
                auto* io = reinterpret_cast<AstVar*>(item);
                io->direction(VDirection::INPUT);
                if (statements)
                    statements->addNextNull(item);
                else
                    statements = item;
            }
        });
        visit_one_to_one({vpiStmt}, obj_h, shared, [&](AstNode* item) {
            if (item) {
                if (statements)
                    statements->addNextNull(item);
                else
                    statements = item;
            }
        });
        return new AstTask(make_fileline(obj_h), objectName, statements);
    }
    case vpiTaskCall: {
        return new AstTaskRef(make_fileline(obj_h), objectName, nullptr);
    }
    case vpiFunction: {
        return process_function(obj_h, shared);
    }
    case vpiReturn:
    case vpiReturnStmt: {
        AstNode* condition = nullptr;
        visit_one_to_one({vpiCondition}, obj_h, shared, [&](AstNode* item) {
            if (item) { condition = item; }
        });
        if (condition) shared.isFunction = true;
        return new AstReturn(make_fileline(obj_h), condition);
    }
    case vpiFuncCall: {
        AstNode* func_refp = nullptr;
        AstNode* arguments = nullptr;
        visit_one_to_many({vpiArgument}, obj_h, shared, [&](AstNode* item) {
            if (item) {
                if (arguments == nullptr) {
                    arguments = new AstArg(make_fileline(obj_h), "", item);
                } else {
                    arguments->addNextNull(new AstArg(make_fileline(obj_h), "", item));
                }
            }
        });
        AstNode* refp = get_class_package_ref_node(make_fileline(obj_h), objectName, shared);
        size_t colon_pos = objectName.rfind("::");
        if (colon_pos != std::string::npos) objectName = objectName.substr(colon_pos + 2);

        size_t dot_pos = objectName.rfind('.');
        if (dot_pos != std::string::npos) {
            // Split object name into prefix.method
            // TODO: Handle >1 dot, currently all goes into prefix
            std::string lhs = objectName.substr(0, dot_pos);
            std::string rhs = objectName.substr(dot_pos + 1, objectName.length());
            AstNode* from = get_referenceNode(make_fileline(obj_h), lhs);
            func_refp = new AstMethodCall(make_fileline(obj_h), from, rhs, arguments);
        } else {
            // Functions can be called as tasks, depending on context
            bool inExpression = is_expr_context(obj_h);

            if (inExpression)
                func_refp = new AstFuncRef(make_fileline(obj_h), objectName, arguments);
            else
                func_refp = new AstTaskRef(make_fileline(obj_h), objectName, arguments);
        }

        return AstDot::newIfPkg(make_fileline(obj_h), refp, func_refp);
    }
    case vpiSysFuncCall: {
        std::vector<AstNode*> arguments;
        visit_one_to_many({vpiArgument}, obj_h, shared, [&](AstNode* item) {
            if (item) { arguments.push_back(item); }
        });

        if (objectName == "$signed") {
            return new AstSigned(make_fileline(obj_h), arguments[0]);
        } else if (objectName == "$unsigned") {
            return new AstUnsigned(make_fileline(obj_h), arguments[0]);
        } else if (objectName == "$cast") {
            return new AstCastParse(make_fileline(obj_h), arguments[0], arguments[1]);
        } else if (objectName == "$isunknown") {
            return new AstIsUnknown(make_fileline(obj_h), arguments[0]);
        } else if (objectName == "$time") {
            return new AstTime(make_fileline(obj_h),
                               VTimescale::TS_1PS);  // TODO: revisit once we have it in UHDM
        } else if (objectName == "$display") {
            AstNode* args = nullptr;
            for (auto a : arguments) {
                if (args == nullptr)
                    args = a;
                else
                    args->addNextNull(a);
            }
            return new AstDisplay(make_fileline(obj_h), AstDisplayType(), nullptr, args);
        } else if (objectName == "$value$plusargs") {
            node = new AstValuePlusArgs(make_fileline(obj_h), arguments[0], arguments[1]);
        } else if (objectName == "$sformat" || objectName == "$swrite") {
            AstNode* args = nullptr;
            // Start from second argument
            for (auto it = ++arguments.begin(); it != arguments.end(); it++) {
                if (args == nullptr)
                    args = *it;
                else
                    args->addNextNull(*it);
            }
            return new AstSFormat(make_fileline(obj_h), arguments[0], args);
        } else if (objectName == "$sformatf") {
            AstNode* args = nullptr;
            // Start from second argument
            for (auto it = arguments.begin(); it != arguments.end(); it++) {
                if (args == nullptr)
                    args = *it;
                else
                    args->addNextNull(*it);
            }
            return new AstSFormatF(make_fileline(obj_h), "", false, args);
        } else if (objectName == "$finish") {
            return new AstFinish(make_fileline(obj_h));
        } else if (objectName == "$fopen") {
            // We need to obtain the variable in which the descriptor will be stored
            // This usually will be LHS of an assignment fd = $fopen(...)
            auto parent_h = vpi_handle({vpiParent}, obj_h);
            auto lhs_h = vpi_handle({vpiLhs}, parent_h);
            AstNode* fd = nullptr;
            if (lhs_h) { fd = visit_object(lhs_h, shared); }
            return new AstFOpen(make_fileline(obj_h), fd, arguments[0], arguments[1]);
        } else if (objectName == "$fclose") {
            return new AstFClose(make_fileline(obj_h), arguments[0]);
        } else if (objectName == "$fwrite") {
            AstNode* filep = arguments[0];
            arguments.erase(arguments.begin());
            AstNode* args = nullptr;
            for (auto a : arguments) {
                if (args == nullptr)
                    args = a;
                else
                    args->addNextNull(a);
            }
            return new AstDisplay(make_fileline(obj_h),
                                  AstDisplayType(AstDisplayType::en::DT_WRITE), filep, args);
        } else if (objectName == "$fflush") {
            return new AstFFlush(make_fileline(obj_h), arguments[0]);
        } else if (objectName == "$clog2") {
            return new AstCLog2(make_fileline(obj_h), arguments[0]);
        } else if (objectName == "$left") {
            if (arguments.size() == 1)
                arguments.push_back(nullptr);  // provide default for optional parameter
            return new AstAttrOf(make_fileline(obj_h), AstAttrType::DIM_LEFT, arguments[0],
                                 arguments[1]);
        } else if (objectName == "$right") {
            if (arguments.size() == 1)
                arguments.push_back(nullptr);  // provide default for optional parameter
            return new AstAttrOf(make_fileline(obj_h), AstAttrType::DIM_RIGHT, arguments[0],
                                 arguments[1]);
        } else if (objectName == "$low") {
            if (arguments.size() == 1)
                arguments.push_back(nullptr);  // provide default for optional parameter
            return new AstAttrOf(make_fileline(obj_h), AstAttrType::DIM_LOW, arguments[0],
                                 arguments[1]);
        } else if (objectName == "$high") {
            if (arguments.size() == 1)
                arguments.push_back(nullptr);  // provide default for optional parameter
            return new AstAttrOf(make_fileline(obj_h), AstAttrType::DIM_HIGH, arguments[0],
                                 arguments[1]);
        } else if (objectName == "$increment") {
            if (arguments.size() == 1)
                arguments.push_back(nullptr);  // provide default for optional parameter
            return new AstAttrOf(make_fileline(obj_h), AstAttrType::DIM_RIGHT, arguments[0],
                                 arguments[1]);
        } else if (objectName == "$size") {
            if (arguments.size() == 1)
                arguments.push_back(nullptr);  // provide default for optional parameter
            return new AstAttrOf(make_fileline(obj_h), AstAttrType::DIM_RIGHT, arguments[0],
                                 arguments[1]);
        } else if (objectName == "$dimensions") {
            return new AstAttrOf(make_fileline(obj_h), AstAttrType::DIM_DIMENSIONS, arguments[0]);
        } else if (objectName == "$unpacked_dimensions") {
            return new AstAttrOf(make_fileline(obj_h), AstAttrType::DIM_UNPK_DIMENSIONS,
                                 arguments[0]);
        } else if (objectName == "$bits") {
            // If this is not an expression, explicitly mark it as data type ref.
            // See exprOrDataType in verilog.y
            AstNode* expr_datatype_p = arguments[0];
            if (VN_IS(expr_datatype_p, ParseRef)) {
                expr_datatype_p = new AstRefDType(make_fileline(obj_h), expr_datatype_p->name());
            }
            return new AstAttrOf(make_fileline(obj_h), AstAttrType::DIM_BITS, expr_datatype_p);
        } else if (objectName == "$realtobits") {
            return new AstRealToBits(make_fileline(obj_h), arguments[0]);
        } else if (objectName == "$bitstoreal") {
            return new AstBitsToRealD(make_fileline(obj_h), arguments[0]);
        } else if (objectName == "$readmemh") {
            if (arguments.size() == 2) {
                return new AstReadMem(make_fileline(obj_h),
                                      true,  // isHex
                                      arguments[0], arguments[1], nullptr, nullptr);
            } else if (arguments.size() == 4) {
                return new AstReadMem(make_fileline(obj_h),
                                      true,  // isHex
                                      arguments[0], arguments[1], arguments[2], arguments[3]);
            }
        } else if (objectName == "$readmemb") {
            if (arguments.size() == 2) {
                return new AstReadMem(make_fileline(obj_h),
                                      false,  // isHex
                                      arguments[0], arguments[1], nullptr, nullptr);
            } else if (arguments.size() == 4) {
                return new AstReadMem(make_fileline(obj_h),
                                      false,  // isHex
                                      arguments[0], arguments[1], arguments[2], arguments[3]);
            }
        } else if (objectName == "$info" || objectName == "$warning" || objectName == "$error"
                   || objectName == "$fatal") {

            AstDisplayType type;
            if (objectName == "$info") {
                type = AstDisplayType::DT_INFO;
            } else if (objectName == "$warning") {
                type = AstDisplayType::DT_WARNING;
            } else if (objectName == "$error") {
                type = AstDisplayType::DT_ERROR;
            } else if (objectName == "$fatal") {
                type = AstDisplayType::DT_FATAL;
                // Verilator discards the finish number - first argument
                if (arguments.size()) arguments.erase(arguments.begin());
            }

            AstNode* args = nullptr;
            for (auto a : arguments) {
                if (args == nullptr)
                    args = a;
                else
                    args->addNextNull(a);
            }
            node = new AstDisplay(make_fileline(obj_h), type, nullptr, args);
            if (type == AstDisplayType::DT_ERROR || type == AstDisplayType::DT_FATAL) {
                auto* stop = new AstStop(make_fileline(obj_h), (type == AstDisplayType::DT_ERROR));
                node->addNext(stop);
            }
            return node;
        } else if (objectName == "$__BAD_SYMBOL__") {
            v3info("\t! Bad symbol encountered @ " << file_name << ":" << currentLine);
            // Dummy statement to keep parsing
            return new AstTime(make_fileline(obj_h),
                               VTimescale::TS_1PS);  // TODO: revisit once we have it in UHDM
        } else {
            v3error("\t! Encountered unhandled SysFuncCall: " << objectName);
        }
        auto parent_h = vpi_handle(vpiParent, obj_h);
        int parent_type = 0;
        if (parent_h) { parent_type = vpi_get(vpiType, parent_h); }
        if (parent_type == vpiBegin) {  // TODO: Are other contexts missing here?
            // In task-like context return values are discarded
            // This is indicated by wrapping the node
            return new AstSysFuncAsTask(make_fileline(obj_h), node);
        } else {
            return node;
        }
    }
    case vpiRange: {
        AstNode* msbNode = nullptr;
        AstNode* lsbNode = nullptr;
        AstRange* rangeNode = nullptr;
        auto leftRange_h = vpi_handle(vpiLeftRange, obj_h);
        if (leftRange_h) { msbNode = visit_object(leftRange_h, shared); }
        auto rightRange_h = vpi_handle(vpiRightRange, obj_h);
        if (rightRange_h) { lsbNode = visit_object(rightRange_h, shared); }
        if (msbNode && lsbNode) {
            rangeNode = new AstRange(make_fileline(obj_h), msbNode, lsbNode);
        }
        return rangeNode;
    }
    case vpiPartSelect: {
        AstNode* msbNode = nullptr;
        AstNode* lsbNode = nullptr;
        AstNode* fromNode = nullptr;

        visit_one_to_one(
            {
                vpiLeftRange,
                vpiRightRange,
            },
            obj_h, shared, [&](AstNode* item) {
                if (item) {
                    if (msbNode == nullptr) {
                        msbNode = item;
                    } else if (lsbNode == nullptr) {
                        lsbNode = item;
                    }
                }
            });
        auto parent_h = vpi_handle(vpiParent, obj_h);
        if (parent_h != 0) {
            std::string parent_name = get_object_name(parent_h, {vpiName, vpiFullName});

            AstNode* refp = get_class_package_ref_node(make_fileline(obj_h), parent_name, shared);
            size_t colon_pos = parent_name.rfind("::");
            if (colon_pos != std::string::npos) parent_name = parent_name.substr(colon_pos + 2);

            fromNode = get_referenceNode(make_fileline(obj_h), parent_name);
            if (refp != nullptr) fromNode = new AstDot(make_fileline(obj_h), true, refp, fromNode);
        }
        return new AstSelExtract(make_fileline(obj_h), fromNode, msbNode, lsbNode);
    }
    case vpiIndexedPartSelect: {
        AstNode* bit = nullptr;
        AstNode* width = nullptr;
        AstNode* fromNode = nullptr;

        visit_one_to_one(
            {
                vpiBaseExpr,
                vpiWidthExpr,
            },
            obj_h, shared, [&](AstNode* item) {
                if (item) {
                    if (bit == nullptr) {
                        bit = item;
                    } else if (width == nullptr) {
                        width = item;
                    }
                }
            });
        auto parent_h = vpi_handle(vpiParent, obj_h);
        std::string parent_name = get_object_name(parent_h, {vpiName, vpiFullName});

        fromNode = get_referenceNode(make_fileline(obj_h), parent_name);

        auto type = vpi_get(vpiIndexedPartSelectType, obj_h);
        if (type == vpiPosIndexed) {
            return new AstSelPlus(make_fileline(obj_h), fromNode, bit, width);
        } else if (type == vpiNegIndexed) {
            return new AstSelMinus(make_fileline(obj_h), fromNode, bit, width);
        } else {
            return nullptr;
        }
    }
    case vpiFor: {
        AstNode* initsp = nullptr;
        AstNode* condp = nullptr;
        AstNode* incsp = nullptr;
        AstNode* bodysp = nullptr;
        visit_one_to_one(
            {
                vpiForInitStmt,
            },
            obj_h, shared, [&](AstNode* item) { initsp = item; });
        visit_one_to_many(
            {
                vpiForInitStmt,
            },
            obj_h, shared, [&](AstNode* item) {
                if (initsp == nullptr) {
                    initsp = item;
                } else {
                    initsp->addNextNull(item);
                }
            });
        visit_one_to_one(
            {
                vpiCondition,
            },
            obj_h, shared, [&](AstNode* item) {
                if (condp == nullptr) {
                    condp = item;
                } else {
                    condp->addNextNull(item);
                }
            });
        visit_one_to_one(
            {
                vpiForIncStmt,
            },
            obj_h, shared, [&](AstNode* item) { incsp = item; });
        visit_one_to_many(
            {
                vpiForIncStmt,
            },
            obj_h, shared, [&](AstNode* item) {
                if (incsp == nullptr) {
                    incsp = item;
                } else {
                    incsp->addNextNull(item);
                }
            });
        visit_one_to_one(
            {
                vpiStmt,
            },
            obj_h, shared, [&](AstNode* item) {
                if (bodysp == nullptr) {
                    bodysp = item;
                } else {
                    bodysp->addNextNull(item);
                }
            });
        AstNode* loop = new AstWhile(make_fileline(obj_h), condp, bodysp, incsp);
        initsp->addNextNull(loop);
        AstNode* stmt = new AstBegin(make_fileline(obj_h), "", initsp);
        return stmt;
    }
    case vpiDoWhile:
    case vpiWhile: {
        AstNode* condp = nullptr;
        AstNode* bodysp = nullptr;
        visit_one_to_one(
            {
                vpiCondition,
            },
            obj_h, shared, [&](AstNode* item) {
                if (condp == nullptr) {
                    condp = item;
                } else {
                    condp->addNextNull(item);
                }
            });
        visit_one_to_one(
            {
                vpiStmt,
            },
            obj_h, shared, [&](AstNode* item) {
                if (bodysp == nullptr) {
                    bodysp = item;
                } else {
                    bodysp->addNextNull(item);
                }
            });
        AstNode* loop = new AstWhile(make_fileline(obj_h), condp, bodysp);
        if (objectType == vpiWhile) {
            return loop;
        } else if (objectType == vpiDoWhile) {
            auto* first_iter = bodysp->cloneTree(true);
            first_iter->addNextNull(loop);
            return first_iter;
        }
        break;
    }

    case vpiBitTypespec:
    case vpiLogicTypespec:
    case vpiIntTypespec:
    case vpiStringTypespec:
    case vpiChandleTypespec:
    case vpiIntegerTypespec:
    case vpiVoidTypespec:
    case vpiEnumTypespec:
    case vpiStructTypespec:
    case vpiPackedArrayTypespec:
    case vpiUnsupportedTypespec: {
        return process_typespec(obj_h, shared);
    }
    case vpiTypespecMember: {
        return process_typespec_member(obj_h, shared);
    }
    case vpiTypeParameter: {
        vpiHandle typespec_h = vpi_handle(vpiTypespec, obj_h);
        return process_typespec(typespec_h, shared);
    }
    case vpiLogicVar:
    case vpiStringVar:
    case vpiTimeVar:
    case vpiRealVar:
    case vpiIntVar:
    case vpiLongIntVar:
    case vpiIntegerVar:
    case vpiEnumVar:
    case vpiBitVar:
    case vpiByteVar: {
        AstNodeDType* dtype = getDType(make_fileline(obj_h), obj_h, shared);
        auto* var = new AstVar(make_fileline(obj_h), AstVarType::VAR, objectName,
                               VFlagChildDType(), dtype);
        if (v3Global.opt.trace()) { var->trace(true); }
        visit_one_to_one({vpiExpr}, obj_h, shared, [&](AstNode* item) { var->valuep(item); });
        return var;
    }
    case vpiPackedArrayVar:
    case vpiArrayVar: {
        auto dtype = getDType(make_fileline(obj_h), obj_h, shared);

        auto* var = new AstVar(make_fileline(obj_h), AstVarType::VAR, objectName,
                               VFlagChildDType(), dtype);
        visit_one_to_one({vpiExpr}, obj_h, shared, [&](AstNode* item) { var->valuep(item); });
        if (v3Global.opt.trace()) { var->trace(true); }
        return var;
    }
    case vpiChandleVar: {
        auto* dtype = new AstBasicDType(make_fileline(obj_h), AstBasicDTypeKwd::CHANDLE);
        auto* var = new AstVar(make_fileline(obj_h), AstVarType::VAR, objectName,
                               VFlagChildDType(), dtype);
        if (v3Global.opt.trace()) { var->trace(true); }
        return var;
    }
    case vpiGenScopeArray: {
        return process_genScopeArray(obj_h, shared);
    }
    case vpiGenScope: {
        AstNode* statements = nullptr;

        vpiHandle typedef_itr = vpi_iterate(vpiTypedef, obj_h);
        while (vpiHandle typedef_obj = vpi_scan(typedef_itr)) {
            AstNode* typedefp = process_typedef(typedef_obj, shared);
            if (statements == nullptr)
                statements = typedefp;
            else
                statements->addNextNull(typedefp);
        }

        visit_one_to_many(
            {
                vpiInternalScope,
                vpiArrayNet,
                // vpiLogicVar,
                // vpiArrayVar,
                vpiMemory,
                vpiVariables,
                vpiNet,
                vpiNamedEvent,
                vpiNamedEventArray,
                vpiProcess,
                vpiContAssign,
                vpiModule,
                vpiModuleArray,
                vpiPrimitive,
                vpiPrimitiveArray,
                vpiDefParam,
                vpiParameter,
                vpiParamAssign,
                vpiGenScopeArray,
                vpiProgram,
                vpiProgramArray,
                // vpiAssertion,
                vpiInterface,
                vpiInterfaceArray,
                vpiAliasStmt,
                vpiClockingBlock,
            },
            obj_h, shared, [&](AstNode* item) {
                if (statements == nullptr) {
                    statements = item;
                } else {
                    statements->addNextNull(item);
                }
            });
        return statements;
    }
    case vpiDelayControl: {
        s_vpi_delay delay;
        vpi_get_delays(obj_h, &delay);

        // Verilator ignores delay statements, just grab the first one for simplicity
        if (delay.da != nullptr) {
            auto* delay_c = new AstConst(make_fileline(obj_h), delay.da[0].real);
            return new AstDelay(make_fileline(obj_h), delay_c);
        }
    }
    case vpiBreak: {
        return new AstBreak(make_fileline(obj_h));
    }
    case vpiForeachStmt: {
        AstNode* arrayp = nullptr;  // Array, then index variables
        AstNode* bodyp = nullptr;
        visit_one_to_one({vpiVariables}, obj_h, shared, [&](AstNode* item) {
            if (arrayp == nullptr) {
                arrayp = item;
            } else {
                arrayp->addNextNull(item);
            }
        });
        visit_one_to_many({vpiLoopVars}, obj_h, shared, [&](AstNode* item) {
            if (arrayp == nullptr) {
                arrayp = item;
            } else {
                arrayp->addNextNull(item);
            }
        });
        visit_one_to_many({vpiStmt}, obj_h, shared, [&](AstNode* item) {
            if (bodyp == nullptr) {
                bodyp = item;
            } else {
                bodyp->addNextNull(item);
            }
        });
        return new AstForeach(make_fileline(obj_h), arrayp, bodyp);
    }
    case vpiMethodFuncCall: {
        AstNode* from = nullptr;
        AstNode* args = nullptr;
        visit_one_to_one({vpiPrefix}, obj_h, shared, [&](AstNode* item) { from = item; });
        if (from == nullptr) {
            from = new AstParseRef(make_fileline(obj_h), VParseRefExp::en::PX_TEXT, "this",
                                   nullptr, nullptr);
        }
        visit_one_to_many({vpiArgument}, obj_h, shared, [&](AstNode* item) {
            AstNode* argp = new AstArg(make_fileline(obj_h), "", item);
            if (args == nullptr) {
                args = argp;
            } else {
                args->addNextNull(argp);
            }
        });
        return new AstMethodCall(make_fileline(obj_h), from, objectName, args);
    }
    // What we can see (but don't support yet)
    case vpiClassObj: break;
    case vpiClassDefn: {
        auto* definition = new AstClass(make_fileline(obj_h), objectName);
        visit_one_to_many(
            {
                vpiVariables,
                // vpiMethods,  // Not supported yet in UHDM
                vpiConstraint,
                vpiParamAssign,
                vpiNamedEvent,
                vpiNamedEventArray,
                vpiInternalScope,
            },
            obj_h, shared, [&](AstNode* item) {
                if (item != nullptr) { definition->addMembersp(item); }
            });
        return definition;
    }
    case vpiImmediateAssert: {
        AstAssert* assert = nullptr;
        visit_one_to_one({vpiExpr}, obj_h, shared, [&](AstNode* item) {
            if (item != nullptr) {
                assert = new AstAssert(make_fileline(obj_h), item, nullptr, nullptr, true);
            }
        });
        return assert;
    }
    case vpiUnsupportedStmt:
        v3info("\t! This statement is unsupported in UHDM: " << file_name << ":" << currentLine);
        // Dummy statement to keep parsing
        return new AstTime(make_fileline(obj_h),
                           VTimescale::TS_1PS);  // TODO: revisit once we have it in UHDM
        break;
    case vpiUnsupportedExpr:
        v3info("\t! This expression is unsupported in UHDM: " << file_name << ":" << currentLine);
        // Dummy expression to keep parsing
        return new AstConst(make_fileline(obj_h), 1);
        break;
    default: {
        // Notify we have something unhandled
        v3error("\t! Unhandled type: " << objectType << ":" << UHDM::VpiTypeName(obj_h));
        break;
    }
    }

    return nullptr;
}

std::vector<AstNodeModule*> visit_designs(const std::vector<vpiHandle>& designs,
                                          std::ostream& coverage_report_stream, V3ParseSym* symp) {

    UhdmShared shared;
    shared.m_symp = symp;
    // Package for top-level class definitions
    // Created and added only if there are classes in the design
    AstPackage* class_package = nullptr;
    for (auto design : designs) {
        visit_one_to_many({UHDM::uhdmallPackages,  // Keep this first, packages need to be defined
                                                   // before any imports
                           // UHDM::uhdmallClasses,
                           UHDM::uhdmallInterfaces, UHDM::uhdmallModules, UHDM::uhdmtopModules},
                          design, shared, [&](AstNode* module) {
                              if (module != nullptr) {
                                  // Top level nodes need to be NodeModules (created from design)
                                  // This is true as we visit only top modules and interfaces (with
                                  // the same AST node) above
                                  shared.top_nodes[module->name()]
                                      = (reinterpret_cast<AstNodeModule*>(module));
                              }
                              for (auto entry : shared.coverage_set) {
                                  coverage_report_stream << std::get<0>(entry) << ":"
                                                         << std::get<1>(entry) << ":"
                                                         << std::get<2>(entry) << std::endl;
                              }
                              shared.coverage_set.clear();
                          });
        // Top level class definitions
        visit_one_to_many(
            {
                UHDM::uhdmallClasses,
            },
            design, shared, [&](AstNode* class_def) {
                if (class_def != nullptr) {
                    if (class_package == nullptr) {
                        class_package = new AstPackage(new FileLine("uhdm"), "AllClasses");
                    }
                    UINFO(6, "Adding class " << class_def->name() << std::endl);
                    class_package->addStmtp(class_def);
                }
                for (auto entry : shared.coverage_set) {
                    coverage_report_stream << std::get<0>(entry) << ":" << std::get<1>(entry)
                                           << ":" << std::get<2>(entry) << std::endl;
                }
                shared.coverage_set.clear();
            });
    }
    std::vector<AstNodeModule*> nodes;
    for (auto node : shared.top_nodes) nodes.push_back(node.second);
    if (class_package != nullptr) { nodes.push_back(class_package); }
    return nodes;
}

}  // namespace UhdmAst
