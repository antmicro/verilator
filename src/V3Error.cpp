// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Error handling
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2003-2022 by Wilson Snyder. This program is free software; you
// can redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

// clang-format off
#include "V3Error.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif
// clang-format on

//======================================================================
// Statics

int V3Error::s_errCount = 0;
int V3Error::s_debugDefault = 0;
int V3Error::s_errorLimit = V3Error::MAX_ERRORS;
bool V3Error::s_warnFatal = true;
std::ostringstream V3Error::s_errorStr;  // Error string being formed
bool V3Error::s_describedWeb = false;
std::array<bool, V3ErrorCode::_ENUM_MAX>
    V3Error::s_describedEachWarn;  // Told user specifics about this warning
std::array<bool, V3ErrorCode::_ENUM_MAX>
    V3Error::s_pretendError;  // Pretend this warning is an error

struct v3errorIniter {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3ErrorCode::V3ErrorCode(const char* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    for (int codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        const V3ErrorCode code{codei};
        if (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            return;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

//######################################################################
// V3Error class functions

void V3Error::init() {
    for (int i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        s_describedEachWarn[i] = false;
        s_pretendError[i] = V3ErrorCode{i}.pretendError();
    }
    if (VL_UNCOVERABLE(string(V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()) != " MAX")) {
        v3fatalSrc("Enum table in V3ErrorCode::EC_ascii() is munged");
    }
}

string V3Error::lineStr(const char* filename, int lineno) VL_MT_SAFE {
    std::ostringstream out;
    const char* const fnslashp = std::strrchr(filename, '/');
    if (fnslashp) filename = fnslashp + 1;
    out << filename << ":" << std::dec << lineno << ":";
    const char* const spaces = "                    ";
    size_t numsp = out.str().length();
    if (numsp > 20) numsp = 20;
    out << (spaces + numsp);
    return out.str();
}

void V3Error::incErrors() VL_MT_SAFE {
    s_errCount++;
    if (errorCount() == errorLimit()) {  // Not >= as would otherwise recurse
        v3fatalExit("Exiting due to too many errors encountered; --error-limit="  //
                    << errorCount() << endl);
    }
}

void V3Error::abortIfWarnings() {
    const bool exwarn = warnFatal() && warnCount();
    if (errorCount() && exwarn) {
        v3fatalExit("Exiting due to " << std::dec << errorCount() << " error(s), "  //
                                      << warnCount() << " warning(s)\n");
    } else if (errorCount()) {
        v3fatalExit("Exiting due to " << std::dec << errorCount() << " error(s)\n");
    } else if (exwarn) {
        v3fatalExit("Exiting due to " << std::dec << warnCount() << " warning(s)\n");
    }
}

bool V3ErrorGuarded::isError(V3ErrorCode code, bool supp) VL_MT_SAFE {
    if (supp) {
        return false;
    } else if (code == V3ErrorCode::USERINFO) {
        return false;
    } else if (code == V3ErrorCode::EC_INFO) {
        return false;
    } else if (code == V3ErrorCode::EC_FATAL) {
        return true;
    } else if (code == V3ErrorCode::EC_FATALEXIT) {
        return true;
    } else if (code == V3ErrorCode::EC_FATALSRC) {
        return true;
    } else if (code == V3ErrorCode::EC_ERROR) {
        return true;
    } else if (code < V3ErrorCode::EC_FIRST_WARN || V3Error::pretendError(code)) {
        return true;
    } else {
        return false;
    }
}

string V3Error::msgPrefix() VL_MT_SAFE_EXCLUDES(singleton().m_mutex) {
    const VerilatedLockGuard guard{singleton().m_mutex};
    return singleton().msgPrefixNoLock();
}

string V3ErrorGuarded::msgPrefixNoLock() VL_REQUIRES(m_mutex) VL_MT_SAFE {
    const V3ErrorCode code = m_errorCode;
    const bool supp = m_errorSuppressed;
    if (supp) {
        return "-arning-suppressed: ";
    } else if (code == V3ErrorCode::USERINFO) {
        return "-Info: ";
    } else if (code == V3ErrorCode::EC_INFO) {
        return "-Info: ";
    } else if (code == V3ErrorCode::EC_FATAL) {
        return "%Error: ";
    } else if (code == V3ErrorCode::EC_FATALEXIT) {
        return "%Error: ";
    } else if (code == V3ErrorCode::EC_FATALSRC) {
        return "%Error: Internal Error: ";
    } else if (code == V3ErrorCode::EC_ERROR) {
        return "%Error: ";
    } else if (isError(code, supp)) {
        return "%Error-" + std::string{code.ascii()} + ": ";
    } else {
        return "%Warning-" + std::string{code.ascii()} + ": ";
    }
}

//======================================================================
// Abort/exit

void V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    if (V3Error::debugDefault()) {
        std::cerr << msgPrefixNoLock() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    } else {
        std::exit(1);
    }
}

void V3Error::vlAbort() VL_MT_SAFE_EXCLUDES(singleton().m_mutex) {
    VL_GCOV_DUMP();
    std::abort();
}

//======================================================================
// Global Functions

void V3Error::suppressThisWarning() VL_MT_SAFE_EXCLUDES(singleton().m_mutex) {
    const VerilatedLockGuard guard{singleton().m_mutex};
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum(std::string{"Warnings, Suppressed "} + singleton().m_errorCode.ascii(), 1);
#endif
    singleton().m_errorSuppressed = true;
}

string V3Error::warnMore() VL_MT_SAFE_EXCLUDES(singleton().m_mutex) {
    const VerilatedLockGuard guard{singleton().m_mutex};
    return singleton().warnMoreNoLock();
}

string V3ErrorGuarded::warnMoreNoLock() VL_REQUIRES(m_mutex) VL_MT_SAFE {
    return string(msgPrefixNoLock().size(), ' ');
}

// cppcheck-has-bug-suppress constParameter
void V3Error::v3errorEnd(std::ostringstream& sstr, const string& extra)
    VL_MT_SAFE_EXCLUDES(singleton().m_mutex) {
    VerilatedLockGuard guard{singleton().m_mutex};
#if defined(__COVERITY__) || defined(__cppcheck__)
    if (s_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif
    // Skip suppressed messages
    if (singleton().m_errorSuppressed
        // On debug, show only non default-off warning to prevent pages of warnings
        && (!debug() || singleton().m_errorCode.defaultsOff()))
        return;
    string msg = singleton().msgPrefixNoLock() + sstr.str();
    if (singleton()
            .m_errorSuppressed) {  // If suppressed print only first line to reduce verbosity
        string::size_type pos;
        if ((pos = msg.find('\n')) != string::npos) {
            msg.erase(pos, msg.length() - pos);
            msg += "...";
        }
    }
    // Trailing newline (generally not on messages) & remove dup newlines
    {
        msg += '\n';  // Trailing newlines generally not put on messages so add
        string::size_type pos;
        while ((pos = msg.find("\n\n")) != string::npos) msg.erase(pos + 1, 1);
    }
    // Suppress duplicate messages
    if (singleton().m_messages.find(msg) != singleton().m_messages.end()) return;
    singleton().m_messages.insert(msg);
    if (!extra.empty()) {
        const string extraMsg = singleton().warnMoreNoLock() + extra + "\n";
        const size_t pos = msg.find('\n');
        msg.insert(pos + 1, extraMsg);
    }
    // Output
    if (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && singleton().m_errorCode == V3ErrorCode::EC_FATALEXIT)
#else
        true
#endif
    ) {
        std::cerr << msg;
    }
    if (!singleton().m_errorSuppressed
        && !(singleton().m_errorCode == V3ErrorCode::EC_INFO
             || singleton().m_errorCode == V3ErrorCode::USERINFO)) {
        const bool anError
            = singleton().isError(singleton().m_errorCode, singleton().m_errorSuppressed);
        if (singleton().m_errorCode >= V3ErrorCode::EC_FIRST_NAMED && !s_describedWeb) {
            s_describedWeb = true;
            std::cerr << singleton().warnMoreNoLock() << "... For "
                      << (anError ? "error" : "warning")
                      << " description see https://verilator.org/warn/"
                      << singleton().m_errorCode.ascii() << "?v=" << PACKAGE_VERSION_NUMBER_STRING
                      << endl;
        }
        if (!s_describedEachWarn[singleton().m_errorCode]
            && !s_pretendError[singleton().m_errorCode]) {
            s_describedEachWarn[singleton().m_errorCode] = true;
            if (singleton().m_errorCode >= V3ErrorCode::EC_FIRST_WARN
                && !singleton().m_describedWarnings) {
                singleton().m_describedWarnings = true;
                std::cerr << singleton().warnMoreNoLock() << "... Use \"/* verilator lint_off "
                          << singleton().m_errorCode.ascii()
                          << " */\" and lint_on around source to disable this message." << endl;
            }
            if (singleton().m_errorCode.dangerous()) {
                std::cerr << singleton().warnMoreNoLock() << "*** See https://verilator.org/warn/"
                          << singleton().m_errorCode.ascii() << " before disabling this,\n";
                std::cerr << singleton().warnMoreNoLock()
                          << "else you may end up with different sim results." << endl;
            }
        }
        // If first warning is not the user's fault (internal/unsupported) then give the website
        // Not later warnings, as a internal may be caused by an earlier problem
        if (singleton().m_tellManual == 0) {
            if (singleton().m_errorCode.mentionManual()
                || sstr.str().find("Unsupported") != string::npos) {
                singleton().m_tellManual = 1;
            } else {
                singleton().m_tellManual = 2;
            }
        }
        if (anError) {
            // We need to unlock lockguard here, as incErrors can call v3errorEnd recursively
            guard.unlock();
            incErrors();
            guard.lock();
        } else {
            singleton().incWarnings();
        }
        if (singleton().m_errorCode == V3ErrorCode::EC_FATAL
            || singleton().m_errorCode == V3ErrorCode::EC_FATALEXIT
            || singleton().m_errorCode == V3ErrorCode::EC_FATALSRC) {
            static bool inFatal = false;
            if (!inFatal) {
                inFatal = true;
                if (singleton().m_tellManual == 1) {
                    std::cerr << singleton().warnMoreNoLock()
                              << "... See the manual at https://verilator.org/verilator_doc.html "
                                 "for more assistance."
                              << endl;
                    singleton().m_tellManual = 2;
                }
#ifndef V3ERROR_NO_GLOBAL_
                if (dumpTree()) {
                    v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("final.tree", 990));
                }
                if (debug()) {
                    if (singleton().m_errorExitCb) singleton().m_errorExitCb();
                    V3Stats::statsFinalAll(v3Global.rootp());
                    V3Stats::statsReport();
                }
#endif
            }

            singleton().vlAbortOrExit();
        } else if (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            if (singleton().m_errorExitCb) singleton().m_errorExitCb();
        }
    }
}
