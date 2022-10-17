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
int V3Error::s_warnCount = 0;
int V3Error::s_debugDefault = 0;
int V3Error::s_errorLimit = V3Error::MAX_ERRORS;
bool V3Error::s_warnFatal = true;
int V3Error::s_tellManual = 0;
std::ostringstream V3Error::s_errorStr;  // Error string being formed
V3ErrorCode V3Error::s_errorCode = V3ErrorCode::EC_FATAL;
bool V3Error::s_errorContexted = false;
bool V3Error::s_errorSuppressed = false;
std::array<bool, V3ErrorCode::_ENUM_MAX> V3Error::s_describedEachWarn;
std::array<bool, V3ErrorCode::_ENUM_MAX> V3Error::s_pretendError;
bool V3Error::s_describedWarnings = false;
bool V3Error::s_describedWeb = false;
V3Error::MessagesSet V3Error::s_messages;
V3Error::ErrorExitCb V3Error::s_errorExitCb = nullptr;
VerilatedMutex V3Error::s_mutex;

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

void V3Error::init() VL_MT_SAFE {
    const VerilatedLockGuard lg{s_mutex};
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

bool V3Error::isError(V3ErrorCode code, bool supp) VL_MT_SAFE {
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
    } else if (code < V3ErrorCode::EC_FIRST_WARN || s_pretendError[code]) {
        return true;
    } else {
        return false;
    }
}

string V3Error::msgPrefix() VL_MT_SAFE {
    const VerilatedLockGuard lg{s_mutex};
    return msgPrefixNoLock();
}

string V3Error::msgPrefixNoLock() VL_REQUIRES(s_mutex) VL_MT_SAFE {
    const V3ErrorCode code = s_errorCode;
    const bool supp = s_errorSuppressed;
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

void V3Error::vlAbortOrExit() VL_REQUIRES(s_mutex) VL_MT_SAFE {
    if (V3Error::debugDefault()) {
        std::cerr << msgPrefixNoLock() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    } else {
        std::exit(1);
    }
}

void V3Error::vlAbort() {
    VL_GCOV_DUMP();
    std::abort();
}

//======================================================================
// Global Functions

void V3Error::suppressThisWarning() VL_MT_SAFE {
    const VerilatedLockGuard lg{s_mutex};
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum(std::string{"Warnings, Suppressed "} + s_errorCode.ascii(), 1);
#endif
    s_errorSuppressed = true;
}

string V3Error::warnMore() VL_MT_SAFE {
    const VerilatedLockGuard lg{s_mutex};
    return warnMoreNoLock();
}

string V3Error::warnMoreNoLock() VL_REQUIRES(s_mutex) VL_MT_SAFE {
    return string(msgPrefixNoLock().size(), ' ');
}

void V3Error::v3errorEnd(std::ostringstream& sstr, const string& extra) VL_MT_SAFE {
    VerilatedLockGuard lg{s_mutex};
#if defined(__COVERITY__) || defined(__cppcheck__)
    if (s_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif
    // Skip suppressed messages
    if (s_errorSuppressed
        // On debug, show only non default-off warning to prevent pages of warnings
        && (!debug() || s_errorCode.defaultsOff()))
        return;
    string msg = msgPrefixNoLock() + sstr.str();
    if (s_errorSuppressed) {  // If suppressed print only first line to reduce verbosity
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
    if (s_messages.find(msg) != s_messages.end()) return;
    s_messages.insert(msg);
    if (!extra.empty()) {
        const string extraMsg = warnMoreNoLock() + extra + "\n";
        const size_t pos = msg.find('\n');
        msg.insert(pos + 1, extraMsg);
    }
    // Output
    if (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && s_errorCode == V3ErrorCode::EC_FATALEXIT)
#else
        true
#endif
    ) {
        std::cerr << msg;
    }
    if (!s_errorSuppressed
        && !(s_errorCode == V3ErrorCode::EC_INFO || s_errorCode == V3ErrorCode::USERINFO)) {
        const bool anError = isError(s_errorCode, s_errorSuppressed);
        if (s_errorCode >= V3ErrorCode::EC_FIRST_NAMED && !s_describedWeb) {
            s_describedWeb = true;
            std::cerr << warnMoreNoLock() << "... For " << (anError ? "error" : "warning")
                      << " description see https://verilator.org/warn/" << s_errorCode.ascii()
                      << "?v=" << PACKAGE_VERSION_NUMBER_STRING << endl;
        }
        if (!s_describedEachWarn[s_errorCode] && !s_pretendError[s_errorCode]) {
            s_describedEachWarn[s_errorCode] = true;
            if (s_errorCode >= V3ErrorCode::EC_FIRST_WARN && !s_describedWarnings) {
                s_describedWarnings = true;
                std::cerr << warnMoreNoLock() << "... Use \"/* verilator lint_off "
                          << s_errorCode.ascii()
                          << " */\" and lint_on around source to disable this message." << endl;
            }
            if (s_errorCode.dangerous()) {
                std::cerr << warnMoreNoLock() << "*** See https://verilator.org/warn/"
                          << s_errorCode.ascii() << " before disabling this,\n";
                std::cerr << warnMoreNoLock() << "else you may end up with different sim results."
                          << endl;
            }
        }
        // If first warning is not the user's fault (internal/unsupported) then give the website
        // Not later warnings, as a internal may be caused by an earlier problem
        if (s_tellManual == 0) {
            if (s_errorCode.mentionManual() || sstr.str().find("Unsupported") != string::npos) {
                s_tellManual = 1;
            } else {
                s_tellManual = 2;
            }
        }
        if (anError) {
            // We need to unlock lockguard here, as incErrors can call v3errorEnd recursively
            lg.unlock();
            incErrors();
            lg.lock();
        } else {
            incWarnings();
        }
        if (s_errorCode == V3ErrorCode::EC_FATAL || s_errorCode == V3ErrorCode::EC_FATALEXIT
            || s_errorCode == V3ErrorCode::EC_FATALSRC) {
            static bool inFatal = false;
            if (!inFatal) {
                inFatal = true;
                if (s_tellManual == 1) {
                    std::cerr << warnMoreNoLock()
                              << "... See the manual at https://verilator.org/verilator_doc.html "
                                 "for more assistance."
                              << endl;
                    s_tellManual = 2;
                }
#ifndef V3ERROR_NO_GLOBAL_
                if (dumpTree()) {
                    v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("final.tree", 990));
                }
                if (debug()) {
                    if (s_errorExitCb) s_errorExitCb();
                    V3Stats::statsFinalAll(v3Global.rootp());
                    V3Stats::statsReport();
                }
#endif
            }

            vlAbortOrExit();
        } else if (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            if (s_errorExitCb) s_errorExitCb();
        }
    }
}
