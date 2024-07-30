// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
//
// Code available from: https://verilator.org
//
// Copyright 2024 by Wilson Snyder.  This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU Lesser
// General Public License Version 3 or the Perl Artistic License Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//=========================================================================
///
/// \file
/// \brief Verilated randomization implementation code
///
/// This file must be compiled and linked against all Verilated objects
/// that use randomization features.
///
/// See the internals documentation docs/internals.rst for details.
///
//=========================================================================

#include "verilated_random.h"

#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>

#define _VL_SOLVER_HASH_LEN 1
#define _VL_SOLVER_HASH_LEN_TOTAL 4

// clang-format off
#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
# define _VL_SOLVER_PIPE  // Allow pipe SMT solving.  Needs fork()
#endif

#ifdef _VL_SOLVER_PIPE
# include <sys/wait.h>
# include <fcntl.h>
#endif

#if defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#endif
// clang-format on

class Process final : private std::streambuf, public std::iostream {
    static constexpr int BUFFER_SIZE = 4096;
    const char* const* m_cmd = nullptr;  // fork() process argv
#ifdef _VL_SOLVER_PIPE
    pid_t m_pid = 0;  // fork() process id
#else
    int m_pid = 0;  // fork() process id - always zero as disabled
#endif
    bool m_pidExited = true;  // If subprocess has exited and can be opened
    int m_pidStatus = 0;  // fork() process exit status, valid if m_pidExited
    int m_writeFd = -1;  // File descriptor TO subprocess
    int m_readFd = -1;  // File descriptor FROM subprocess
    char m_readBuf[BUFFER_SIZE];
    char m_writeBuf[BUFFER_SIZE];

public:
    typedef std::streambuf::traits_type traits_type;

protected:
    int overflow(int c = traits_type::eof()) override {
        char c2 = static_cast<char>(c);
        if (pbase() == pptr()) return 0;
        size_t size = pptr() - pbase();
#ifdef VL_DEBUG
//        std::stringstream ss;
//        ss << "SMTLib:\n";
//        ss.write(pbase(), size);
//        VL_WARN_MT(__FILE__, __LINE__, "randomize", ss.str().c_str());
#endif
        ssize_t n = ::write(m_writeFd, pbase(), size);
        if (n == -1) perror("write");
        if (n <= 0) {
            wait_report();
            return traits_type::eof();
        }
        if (n == size)
            setp(m_writeBuf, m_writeBuf + sizeof(m_writeBuf));
        else
            setp(m_writeBuf + n, m_writeBuf + sizeof(m_writeBuf));
        if (c != traits_type::eof()) sputc(c2);
        return 0;
    }
    int underflow() override {
        sync();
        ssize_t n = ::read(m_readFd, m_readBuf, sizeof(m_readBuf));
        if (n == -1) perror("read");
        if (n <= 0) {
            wait_report();
            return traits_type::eof();
        }
        setg(m_readBuf, m_readBuf, m_readBuf + n);
        return traits_type::to_int_type(m_readBuf[0]);
    }
    int sync() override {
        overflow();
        return 0;
    }

public:
    explicit Process(const char* const* const cmd = nullptr)
        : std::streambuf{}
        , std::iostream{this}
        , m_cmd{cmd} {
        open(cmd);
    }

    void wait_report() {
        if (m_pidExited) return;
#ifdef _VL_SOLVER_PIPE
        if (waitpid(m_pid, &m_pidStatus, 0) != m_pid) return;
        if (m_pidStatus) {
            std::stringstream msg;
            msg << "Subprocess command `" << m_cmd[0];
            for (const char* const* arg = m_cmd + 1; *arg; arg++) msg << ' ' << *arg;
            msg << "' failed: ";
            if (WIFSIGNALED(m_pidStatus))
                msg << strsignal(WTERMSIG(m_pidStatus))
                    << (WCOREDUMP(m_pidStatus) ? " (core dumped)" : "");
            else if (WIFEXITED(m_pidStatus))
                msg << "exit status " << WEXITSTATUS(m_pidStatus);
            const std::string str = msg.str();
            VL_WARN_MT("", 0, "Process", str.c_str());
        }
#endif
        m_pidExited = true;
        m_pid = 0;
        closeFds();
    }

    void closeFds() {
        if (m_writeFd != -1) {
            close(m_writeFd);
            m_writeFd = -1;
        }
        if (m_readFd != -1) {
            close(m_readFd);
            m_readFd = -1;
        }
    }

    bool open(const char* const* const cmd) {
        setp(std::begin(m_writeBuf), std::end(m_writeBuf));
        setg(m_readBuf, m_readBuf, m_readBuf);
#ifdef _VL_SOLVER_PIPE
        if (!cmd || !cmd[0]) return false;
        m_cmd = cmd;
        int fd_stdin[2];  // Can't use std::array
        int fd_stdout[2];  // Can't use std::array
        constexpr int P_RD = 0;
        constexpr int P_WR = 1;

        if (pipe(fd_stdin) != 0) {
            perror("Process::open: pipe");
            return false;
        }
        if (pipe(fd_stdout) != 0) {
            perror("Process::open: pipe");
            close(fd_stdin[P_RD]);
            close(fd_stdin[P_WR]);
            return false;
        }

        if (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            fprintf(stderr, "stdin/stdout closed before pipe opened\n");
            close(fd_stdin[P_RD]);
            close(fd_stdin[P_WR]);
            close(fd_stdout[P_RD]);
            close(fd_stdout[P_WR]);
            return false;
        }

        const pid_t pid = fork();
        if (pid < 0) {
            perror("Process::open: fork");
            close(fd_stdin[P_RD]);
            close(fd_stdin[P_WR]);
            close(fd_stdout[P_RD]);
            close(fd_stdout[P_WR]);
            return false;
        }
        if (pid == 0) {
            // Child
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], STDIN_FILENO);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], STDOUT_FILENO);
            execvp(cmd[0], const_cast<char* const*>(cmd));
            std::stringstream msg;
            msg << "Process::open: execvp(" << cmd[0] << ")";
            const std::string str = msg.str();
            perror(str.c_str());
            _exit(127);
        }
        // Parent
        m_pid = pid;
        m_pidExited = false;
        m_pidStatus = 0;
        m_readFd = fd_stdout[P_RD];
        m_writeFd = fd_stdin[P_WR];

        close(fd_stdin[P_RD]);
        close(fd_stdout[P_WR]);

        return true;
#else
        return false;
#endif
    }
};

static Process& getSolver() {
    static Process s_solver;
    static bool s_done = false;
    if (s_done) return s_solver;
    s_done = true;

    static std::vector<const char*> s_argv;
    static std::string s_program = Verilated::threadContextp()->solverProgram();
    s_argv.emplace_back(&s_program[0]);
    for (char* arg = &s_program[0]; *arg; arg++) {
        if (*arg == ' ') {
            *arg = '\0';
            s_argv.emplace_back(arg + 1);
        }
    }
    s_argv.emplace_back(nullptr);

    const char* const* const cmd = &s_argv[0];
    s_solver.open(cmd);
    s_solver << "(set-logic QF_BV)\n";
    s_solver << "(set-option :smt.phase-selection 5)\n";
    s_solver << "(check-sat)\n";
    s_solver << "(reset)\n";
    std::string s;
    getline(s_solver, s);
    if (s == "sat") return s_solver;

    std::stringstream msg;
    msg << "Unable to communicate with SAT solver, please check its installation or specify a "
           "different one in VERILATOR_SOLVER environment variable.\n";
    msg << " ... Tried: $";
    for (const char* const* arg = cmd; *arg; arg++) msg << ' ' << *arg;
    msg << '\n';
    const std::string str = msg.str();
    VL_WARN_MT("", 0, "randomize", str.c_str());

    while (getline(s_solver, s)) {}
    return s_solver;
}

//======================================================================
// VlRandomizer:: Methods

std::ostream& operator<<(std::ostream& os, const VlRandomExpr& dt) {
    dt.emit(os);
    return os;
}

void VlRandomSort::emit(std::ostream& s) const {
    switch (constructor()) {
    case Constructor::VL_RAND_BV: s << "(_ BitVec " << elemWidth() << ")"; break;
    case Constructor::VL_RAND_ABV:
        s << "(Array (_ BitVec " << idxWidth() << ") (_ BitVec " << elemWidth() << "))";
        break;
    case Constructor::VL_RAND_BOOL: s << "Bool"; break;
    default: s << "UNSUP_TYPE"; break;
    }
}
void VlRandomVarRef::emit(std::ostream& s) const { s << m_name; }
void VlRandomBVConst::emit(std::ostream& s) const {
    s << "#b";
    for (int i = 0; i < m_width; i++) s << (VL_BITISSET_Q(m_val, m_width - i - 1) ? '1' : '0');
}
void VlRandomBinOp::emit(std::ostream& s) const {
    s << '(' << m_op << ' ' << *m_lhs << ' ' << *m_rhs << ')';
}
void VlRandomExtract::emit(std::ostream& s) const {
    s << "((_ extract " << m_idx << ' ' << m_idx << ") " << *m_expr << ')';
}
void VlRandomSelectRef::emit(std::ostream& s) const {
    s << "(select " << *m_lhs << ' ' << m_idx << ')';
}
void VlRandomBVXorMany::emit(std::ostream& s) const {
    s << "(bvxor";
    for (auto& expr : m_exprs) s << " " << *expr;
    s << ")";
}
void VlRandomConstraint::emit(std::ostream& s) const {
    s << "(= (bvurem (bvmul (concat";
    for (auto& expr : m_bv_exprs) s << " " << *expr;
    s << ") " << *m_mult << ") " << *m_mod << ") " << *m_hash;
}
bool VlRandomVarRef::set(std::string&& val) const {
    VlWide<VL_WQ_WORDS_E> qowp;
    VL_SET_WQ(qowp, 0ULL);
    WDataOutP owp = qowp;
    int obits = sort().elemWidth();
    if (obits > VL_QUADSIZE) owp = reinterpret_cast<WDataOutP>(datap());
    int i;
    for (i = 0; val[i] && val[i] != '#'; i++) {}
    if (val[i++] != '#') return false;
    switch (val[i++]) {
    case 'b': _vl_vsss_based(owp, obits, 1, &val[i], 0, val.size() - i); break;
    case 'o': _vl_vsss_based(owp, obits, 3, &val[i], 0, val.size() - i); break;
    case 'h':  // FALLTHRU
    case 'x': _vl_vsss_based(owp, obits, 4, &val[i], 0, val.size() - i); break;
    default:
        VL_WARN_MT(__FILE__, __LINE__, "randomize",
                   "Internal: Unable to parse solver's randomized number");
        return false;
    }
    if (obits <= VL_BYTESIZE) {
        CData* const p = static_cast<CData*>(datap());
        *p = VL_CLEAN_II(obits, obits, owp[0]);
    } else if (obits <= VL_SHORTSIZE) {
        SData* const p = static_cast<SData*>(datap());
        *p = VL_CLEAN_II(obits, obits, owp[0]);
    } else if (obits <= VL_IDATASIZE) {
        IData* const p = static_cast<IData*>(datap());
        *p = VL_CLEAN_II(obits, obits, owp[0]);
    } else if (obits <= VL_QUADSIZE) {
        QData* const p = static_cast<QData*>(datap());
        *p = VL_CLEAN_QQ(obits, obits, VL_SET_QW(owp));
    } else {
        _vl_clean_inplace_w(obits, owp);
    }
    return true;
}
bool VlRandomSelectRef::set(std::string&& val) const {
    VL_WARN_MT(__FILE__, __LINE__, "randomize", "Internal: Array assignments unimplemented");
    return false;
}

std::unique_ptr<const VlRandomExpr> VlRandomizer::randomConstraint(VlRNG& rngr, int bits) {
    unsigned long long hash = VL_RANDOM_RNG_I(rngr) & ((1 << bits) - 1);
    std::unique_ptr<const VlRandomExpr> concat = nullptr;
    std::vector<std::unique_ptr<const VlRandomExpr>> varbits;
    for (const auto& var : m_vars) {
        switch (var.second->sort().constructor()) {
        case VlRandomSort::Constructor::VL_RAND_BV:
            for (int i = 0; i < var.second->sort().elemWidth(); i++)
                varbits.emplace_back(new VlRandomExtract(var.second->cloneRef(), i));
            break;
        case VlRandomSort::Constructor::VL_RAND_ABV:
            // TODO: Bypass unconstrained indices and use fast randomization method for them
            for (int idx = 0; idx < var.second->sort().len(); idx++) {
                for (int i = 0; i < var.second->sort().elemWidth(); i++)
                    varbits.emplace_back(std::make_shared<const VlRandomExtract>(
                        std::make_shared<const VlRandomSelectRef>(var.second, idx), i));
            }
            break;
        default:
            VL_WARN_MT(__FILE__, __LINE__, "randomize",
                       "Internal: Unsupported randomization type");
        }
    }
    for (int i = 0; i < bits; i++) {
        std::vector<std::shared_ptr<const VlRandomExpr>> xorbits;
        for (unsigned j = 0; j * 2 < varbits.size(); j++) {
            unsigned idx = j + VL_RANDOM_RNG_I(rngr) % (varbits.size() - j);
            std::unique_ptr<const VlRandomExpr>& sel = varbits[idx];
            std::swap(varbits[idx], varbits[j]);
            xorbits.emplace_back(sel->cloneExpr());
        }

        concat = concat == nullptr ? bit : std::make_shared<const VlRandomConcat>(concat, bit);
    }
    return std::make_unique<const VlRandomEq>(std::move(concat),
                                              std::make_unique<const VlRandomBVConst>(hash, bits));
}

bool VlRandomizer::checkSat(std::iostream& file) const {
    file << "(check-sat)\n";

    std::string sat;
    do { std::getline(file, sat); } while (sat == "");

    if (sat == "unsat") return false;
    if (sat != "sat") {
        std::stringstream msg;
        msg << "Internal: Solver error: " << sat;
        const std::string str = msg.str();
        VL_WARN_MT(__FILE__, __LINE__, "randomize", str.c_str());
        return false;
    }
    return true;
}

bool VlRandomizer::next(VlRNG& rngr) {
    if (m_vars.empty()) return true;
    std::iostream& f = getSolver();
    if (!f) return false;

    f << "(set-option :produce-models true)\n";
    f << "(set-logic QF_ABV)\n";
    f << "(set-option :smt.phase-selection 5)\n";
    unsigned long long seed = VL_RANDOM_RNG_I(rngr) & 0xffff;
    f << "(set-option :smt.random-seed " << seed << ")\n";
    f << "(set-option :sat.random-seed " << seed << ")\n";
    f << "(set-option :nlsat.seed " << seed << ")\n";
    f << "(set-option :sls.random-seed " << seed << ")\n";
    for (const auto& var : m_vars) {
        f << "(declare-fun " << *var.second << " () " << var.second->sort() << ")\n";
    }
    for (const std::string& constraint : m_constraints) { f << "(assert " << constraint << ")\n"; }

    unsigned long long times = (VL_RANDOM_RNG_I(rngr) % 10) + 1;
    for (int i = 0; i < times; i++) {
        if (!checkSat(f)) {
            f << "(reset)\n";
            return false;
        }
    }

    parseSolution(f);

    bool sat = true;
    for (int i = 0; i < _VL_SOLVER_HASH_LEN_TOTAL && sat; i++) {
        auto constr = randomConstraint(rngr, _VL_SOLVER_HASH_LEN);
        f << "(assert " << *constr << ")\n";
        sat = checkSat(f);
    }
    parseSolution(f);

    f << "(reset)\n";
    return true;
}

namespace smtparse {
using std::istream;
using std::string;

bool smtIsCharWhitespace(char c) { return (c == ' ') | (c == '\n') | (c == '\r'); }

struct SMTParseState {
    std::istream& s;
    char buf = '\0';
    bool err;
    const char* err_filename;
    int err_linenum;
#ifdef VL_DEBUG
    string d_read;
#endif

    SMTParseState(istream& s_)
        : s(s_) {}

    char peekChar() {
        if (buf) return buf;
        char c;
        s >> c;
        buf = c;
        return c;
    }

    char popChar() {
        char c;
        if (buf) {
            c = buf;
            buf = '\0';
        } else {
            s >> c;
        }
#ifdef VL_DEBUG
        d_read += c;
#endif
        return c;
    }

    bool popWhitespace(bool required = true) {
        bool any_whitespace = false;
        while (true) {
            char c = peekChar();
            bool whitespace = smtIsCharWhitespace(c);
            any_whitespace |= whitespace;
            if (!whitespace) break;
            popChar();
        }
        return !required || any_whitespace;
    }

    bool error(const char* filename, int linenum) {
        err = true;
        err_filename = filename;
        err_linenum = linenum;
        return false;
    }

    void clrErr() { err = false; }

    bool checkErr() {
        if (err) {
#ifdef VL_DEBUG
            string err_hl;
            int pos = -1;
            if (buf) {
                err_hl = d_read + "\033[41;371m" + popChar() + "\033[0m";
                pos = d_read.length();
            } else if (d_read.length() > 0) {
                err_hl = d_read.substr(0, d_read.length() - 1) + "\033[41;371m"
                         + d_read[d_read.length() - 1] + "\033[0m";
                pos = d_read.length() - 1;
            }
            // TODO: Fix this loop hanging
            // while (!s.fail() && !s.eof()) s >> d_read;

            string err_str = "Internal: Unable to parse solver's response: invalid "
                             "S-expression\nAn error occured at idx "
                             + std::to_string(pos) + ":\n" + err_hl;

            VL_WARN_MT(err_filename, err_linenum, "smt", &err_str[0]);
#else
            VL_WARN_MT(err_filename, err_linenum, "smt",
                       "Internal: Unable to parse solver's "
                       "response: invalid  S-expression");
#endif
            return false;
        }
        return true;
    }
};

template <typename ParseInner>
bool smtParseParen(SMTParseState& s, ParseInner parseInner) {
    if (s.popChar() != '(') return false;
    s.popWhitespace();
    if (!parseInner(s)) return false;
    s.popWhitespace();
    if (s.popChar() != ')') return s.error(__FILE__, __LINE__);
    return true;
}

bool smtParseIdent(SMTParseState& s, string& ident) {
    bool allowNum = false;
    ident.clear();
    while (true) {
        char c = s.peekChar();
        if (!allowNum && (c >= '0' && c <= '9')) return false;
        if (smtIsCharWhitespace(c) || c == '(' || c == ')' || c == '#') {
            if (ident.length() == 0) return false;
            return true;
        };
        ident += s.popChar();
        allowNum = true;
    }
}

bool smtParseValueRaw(SMTParseState& s, string& value) {
    bool allowNum = false;
    value.clear();
    while (true) {
        char c = s.peekChar();
        if (smtIsCharWhitespace(c) || c == '(' || c == ')') return value.length() > 0;
        value += s.popChar();
    }
}

template <typename LhsT, typename Assign, typename ParseLhs>
bool smtParseAssignList(SMTParseState& s, ParseLhs parseLhs, Assign assign) {
    string ident, value;
    s.popWhitespace();
    smtParseParen(s, [&](SMTParseState& s) {
        while (true) {
            bool inner_ok = false;
            if (!smtParseParen(s, [&](SMTParseState& s) {
                    LhsT lhs;
                    if (!parseLhs(s, lhs)) return s.error(__FILE__, __LINE__);
                    s.popWhitespace();
                    if (!smtParseValueRaw(s, value)) return s.error(__FILE__, __LINE__);
                    assign(lhs, std::move(value));
                    inner_ok = true;
                    return true;
                }))
                return inner_ok;
            s.popWhitespace();
        }
        return true;
    });
    return true;
}
}  //namespace smtparse

void VlRandomizer::parseSolution(std::iostream& f) {
    f << "(get-value (";
    for (const auto& var : m_vars) f << *var.second << ' ';
    f << "))\n";

    // Quasi-parse S-expression of the form ((x #xVALUE) (y #bVALUE) (z #xVALUE))
    {
        smtparse::SMTParseState s(f);
        smtparse::smtParseAssignList</*lhs*/ std::string>(
            s,
            /*parseLhs*/ smtparse::smtParseIdent,
            /*assign*/ [&](const std::string& ident, std::string&& value) {
                auto it = m_vars.find(ident);
                if (it == m_vars.end()) return false;

                auto& ref = it->second;
                switch (ref->sort().constructor()) {
                case VlRandomSort::Constructor::VL_RAND_BV:
                    const VlRandomVarRef& varref = static_cast<const VlRandomVarRef&>(*it->second);
                    if (m_randmode && !varref.randModeIdxNone()) {
                        if (!(m_randmode->at(varref.randModeIdx()))) return true;
                    }
                    varref.set(std::move(value));
                }
                return true;
            });
        (void)s.checkErr();
    }
}

void VlRandomizer::hard(std::string&& constraint) {
    m_constraints.emplace_back(std::move(constraint));
}

void VlRandomizer::clear() { m_constraints.clear(); }

#ifdef VL_DEBUG
void VlRandomizer::dump() const {
    for (const auto& var : m_vars) {
        std::stringstream ss;
        var.second->emit(ss);
        VL_PRINTF("Variable (%d/%d): %s\n", var.second->sort().elemWidth(),
                  var.second->sort().len(), ss.str().c_str());
    }
    for (const std::string& c : m_constraints) VL_PRINTF("Constraint: %s\n", c.c_str());
}
#endif
