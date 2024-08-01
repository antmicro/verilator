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
#include "verilated.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>
#include <type_traits>

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

namespace vlRandom {

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

#if VL_DEBUG
    std::string dbg_str;
#endif

public:
    typedef std::streambuf::traits_type traits_type;

protected:
    int overflow(int c = traits_type::eof()) override {
        char c2 = static_cast<char>(c);
        if (pbase() == pptr()) return 0;
        size_t size = pptr() - pbase();
#ifdef VL_DEBUG
        std::stringstream dbg_ss;
        dbg_ss << "\n; EMIT SMTLib\n";
        dbg_ss.write(pbase(), size);
        dbg_str += dbg_ss.str();
        //VL_WARN_MT(__FILE__, __LINE__, "randomize", dbg_ss.str().c_str());
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

using WValue = std::vector<WData>;


size_t sizeWidth(size_t size) {
#ifndef _WIN32
    if (size == 0) return 1;
    return __builtin_clz(size);
#else
    int i = m_len;
    i |= (i >> 1);
    i |= (i >> 2);
    i |= (i >> 4);
    i |= (i >> 8);
    i |= (i >> 16);
    i -= ((i >> 1) & 0x55555555);
    i = (((i >> 2) & 0x33333333) + (i & 0x33333333));
    i = (((i >> 4) + i) & 0x0f0f0f0f);
    i += (i >> 8);
    i += (i >> 16);
    return i & 0x0000003f;
#endif
}

std::pair<WValue, WValue> getPrimePair() {
    // TODO: This is a mock

    // Maybe we can use prime powers?
    return std::make_pair(
        WValue({
            0x00000121,
            0x00000000,
            0x00000000,
            0x4404A7E8,
            0x777839E7,
            0x749CE90C,
            0x65277B06,
            0x5A13AE34,
            0xF2E44DEA,
            0x2AEA2879,
            0x000001D4
        }),
        WValue({
            0x000007E1,
            0x00000000,
            0x00000000,
            0xE1178E81,
            0xE478B23B,
            0x1C46D01A,
            0x79F5080F,
            0x62E7F4A7,
            0x62CD8A51,
            0x77D9D58B
        })
    );
}

struct CharBuf {
    char* buf;

    CharBuf(size_t length)
        : buf(new char[length + 1]) {
        buf[length] = '\0';
    }
    CharBuf(const CharBuf& other) = delete;
    CharBuf& operator==(const CharBuf& other) = delete;
    CharBuf(CharBuf&& other) {
        other.buf = buf;
        buf = nullptr;
    }
    CharBuf& operator==(CharBuf&& other) {
        other.buf = buf;
        buf = nullptr;
        return *this;
    }
    ~CharBuf() {
        delete[] buf;
    }
};

std::ostream& operator<<(std::ostream& os, const CharBuf& buf) {
    os << buf.buf;
    return os;
}

template <typename OnBit>
void forEachBitMSB(const WValue& data, int width, OnBit onBit) {
    using word_t = WData;

    int wordIdx = (width - 1) / (sizeof(word_t) * 8);
    int bitIdx = (width - 1) % (sizeof(word_t) * 8);
    do {
        word_t word = data[wordIdx];
        while (bitIdx > 0) onBit((word & (0x1 << bitIdx)) != 0);
        --wordIdx;
        bitIdx = sizeof(word_t) - 1;
    } while (wordIdx > 0);
}

void emitSMTFormatConst(std::ostream& os, const WValue& data, int width) {
    // TODO: Use hex representations when 4|width
    static const size_t fmtLen = 2;
    char* const buf = new char[width + fmtLen + 1];
    buf[width + fmtLen] = '\0';
    buf[0] = '#';
    buf[1] = 'b';

    int bitIdx = 0;
    for (uint32_t dword : data) {
        while (bitIdx != width) {
            buf[fmtLen + width - 1 - bitIdx] = dword & 0x1 ? '1' : '0';
            dword >>= 1;
            ++bitIdx;
            if (bitIdx % 32 == 0) break;
        }
    }
    // Zero-extend
    while (bitIdx < width) buf[fmtLen + width - 1 - bitIdx] = '0';

    os << buf;
    delete[] buf;
}

void emitSMTFormatConst(std::ostream& os, unsigned long long data, int width) {
    // TODO: Use hex representations when 4|width
    static const size_t fmtLen = 2;
    char* const buf = new char[width + fmtLen + 1];
    buf[width + fmtLen] = '\0';
    buf[0] = '#';
    buf[1] = 'b';

    int bitIdx = 0;
    while (bitIdx != width) {
        buf[fmtLen + width - 1 - bitIdx] = data & 0x1 ? '1' : '0';
        data >>= 1;
        ++bitIdx;
    }
    // Zero-extend
    while (bitIdx < width) buf[fmtLen + width - 1 - bitIdx] = '0';

    os << buf;
    delete[] buf;
}

template <typename Emitter>
void emitSMTExtract(std::ostream& os, int idx, Emitter emitExpr) {
    static_assert(std::is_invocable<Emitter, std::ostream&>());

    os << "((_ extract " << idx << ' ' << idx << ") ";
    emitExpr(os);
    os << ')';
}

template <typename Emitter>
void emitSMTExtract(std::ostream& os, int idx, int width, Emitter emitExpr) {
    static_assert(std::is_invocable<Emitter, std::ostream&>());

    os << "((_ extract " << idx + width - 1 << ' ' << idx << ") ";
    emitExpr(os);
    os << ')';
}

template <typename Emitter>
void emitSMTConstraintMod(std::ostream& os, VlRNG& rng, Emitter emitConcatVec) {
    static_assert(std::is_invocable<Emitter, std::ostream&>());

    std::pair<WValue, WValue> primes = getPrimePair();
    unsigned long long hash = VL_RANDOM_RNG_I(rng);

    auto emitRemainder = [&](std::ostream& os) {
        os << "(bvurem (bvmul ";
        size_t totalWidth = emitConcatVec(os);
        os << ' ';
        emitSMTFormatConst(os, primes.first, totalWidth);
        os << ") ";
        emitSMTFormatConst(os, primes.second, totalWidth);
        os << ')';
    };

    os << "(= (let ((vl-xored-remainder (bvxor ";
    emitSMTExtract(os, 0, 32, emitRemainder);
    os << ' ';
    emitSMTFormatConst(os, hash, 32);
    os << "))) (bvxor";
    for (int bitIdx = 0; bitIdx < 32; ++bitIdx) {
        os << ' ';
        emitSMTExtract(os, bitIdx, [](std::ostream& os){ os << "vl-xored-remainder"; });
    }
    os << ")) #b0)"; // XOR to zero
}

template <typename Emitter>
void emitSMTConstraintMul(std::ostream& os, VlRNG& rng, Emitter emitConcatVec) {
    static_assert(std::is_invocable<Emitter, std::ostream&>());

    std::pair<WValue, WValue> primes = getPrimePair();
    unsigned long long hash = VL_RANDOM_RNG_I(rng);

    auto emitRemainder = [&](std::ostream& os) {
        os << "(bvmul ";
        size_t totalWidth = emitConcatVec(os);
        os << ' ';
        emitSMTFormatConst(os, primes.first, totalWidth);
        os << ") ";
    };

    os << "(= (let ((vl-xored-remainder (bvxor ";
    emitSMTExtract(os, 0, 32, emitRemainder);
    os << ' ';
    emitSMTFormatConst(os, hash, 32);
    os << "))) (bvxor";
    for (int bitIdx = 0; bitIdx < 32; ++bitIdx) {
        os << ' ';
        emitSMTExtract(os, bitIdx, [](std::ostream& os){ os << "vl-xored-remainder"; });
    }
    os << ")) #b0)"; // XOR to zero
}

void smtEmitVarDecl(std::ostream& os, const VlRandomVarRef& varref) {
    os << "(declare-fun " << varref.name << " () (_ BitVec " << varref.width << "))";
}

void smtEmitArrDecl(std::ostream& os, const VlRandomArrRef& arrref) {
    os << "(declare-fun " << arrref.arrName << " () (Array (_ BitVec " << sizeWidth(arrref.length)
       << ") (_ BitVec " << sizeWidth(arrref.width) << ")))";
}

bool setRef(void* const datap, const int width, std::string&& val) {
    VlWide<VL_WQ_WORDS_E> qowp;
    VL_SET_WQ(qowp, 0ULL);
    WDataOutP owp = qowp;
    int obits = width;
    if (obits > VL_QUADSIZE) owp = reinterpret_cast<WDataOutP>(datap);
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
        CData* const p = static_cast<CData*>(datap);
        *p = VL_CLEAN_II(obits, obits, owp[0]);
    } else if (obits <= VL_SHORTSIZE) {
        SData* const p = static_cast<SData*>(datap);
        *p = VL_CLEAN_II(obits, obits, owp[0]);
    } else if (obits <= VL_IDATASIZE) {
        IData* const p = static_cast<IData*>(datap);
        *p = VL_CLEAN_II(obits, obits, owp[0]);
    } else if (obits <= VL_QUADSIZE) {
        QData* const p = static_cast<QData*>(datap);
        *p = VL_CLEAN_QQ(obits, obits, VL_SET_QW(owp));
    } else {
        _vl_clean_inplace_w(obits, owp);
    }
    return true;
}

bool smtIsCharWhitespace(char c) { return (c == ' ') | (c == '\n') | (c == '\r'); }

struct SMTParseState {
    std::istream& s;
    char buf = '\0';
    bool err;
    const char* err_filename;
    int err_linenum;
#ifdef VL_DEBUG
    std::string d_read;
#endif

    SMTParseState(std::istream& s_)
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
            std::string err_hl;
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

            std::string err_str = "Internal: Unable to parse solver's response: invalid "
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
    static_assert(std::is_invocable<ParseInner, SMTParseState&>());

    if (s.popChar() != '(') return false;
    s.popWhitespace();
    if (!parseInner(s)) return false;
    s.popWhitespace();
    if (s.popChar() != ')') return s.error(__FILE__, __LINE__);
    return true;
}

bool smtParseIdent(SMTParseState& s, std::string& ident) {
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

bool smtParseValueRaw(SMTParseState& s, std::string& value) {
    bool allowNum = false;
    value.clear();
    while (true) {
        char c = s.peekChar();
        if (smtIsCharWhitespace(c) || c == '(' || c == ')') return value.length() > 0;
        value += s.popChar();
    }
}

template <typename LhsT, typename ParseLhs, typename Assign>
bool smtParseAssignList(SMTParseState& s, ParseLhs parseLhs, Assign assign) {
    static_assert(std::is_default_constructible<LhsT>());
    static_assert(std::is_invocable<ParseLhs, SMTParseState&, LhsT&>());
    static_assert(std::is_invocable<Assign, LhsT, std::string&&>());

    std::string ident, value;
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

}  //namespace vlRandom

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
    std::iostream& f = vlRandom::getSolver();
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
        vlRandom::smtEmitVarDecl(f, var.second);
        f << '\n';
    }
    for (const auto& arr : m_arrs) {
        vlRandom::smtEmitArrDecl(f, arr.second);
        f << '\n';
    }
    for (const std::string& constraint : m_constraints) f << "(assert " << constraint << ")\n";

    unsigned long long times = (VL_RANDOM_RNG_I(rngr) % 10) + 1;
    for (int i = 0; i < times; i++) {
        if (!checkSat(f)) {
            f << "(reset)\n";
            return false;
        }
    }

    parseSolution(f);

    f << "(assert ";
    vlRandom::emitSMTConstraintMod(f, rngr, [this](std::ostream& os) -> size_t {
        return emitConcatAll(os);
    });
    f << ")\n";
    bool sat = checkSat(f);
    parseSolution(f);

    f << "(reset)\n";
    return true;
}

void VlRandomizer::parseSolution(std::iostream& f) {
    f << "(get-value (";
    for (const auto& var : m_vars) f << var.second.name << ' ';
    f << "))\n";

    // Quasi-parse S-expression of the form ((x #xVALUE) (y #bVALUE) (z #xVALUE))
    {
        vlRandom::SMTParseState s(f);
        vlRandom::smtParseAssignList</*lhs*/ std::string>(
            s,
            /*parseLhs*/ vlRandom::smtParseIdent,
            /*assign*/ [&](const std::string& ident, std::string&& value) {
                auto vars_it = m_vars.find(ident);
                if (vars_it != m_vars.end()) {
                    auto& ref = vars_it->second;
                    if (m_randmode && !ref.randModeIdxNone()) {
                        if (!(m_randmode->at(ref.randModeIdx))) return true;
                    }
                    vlRandom::setRef(ref.datap, ref.width, std::move(value));
                    return true;
                }

                auto arrs_it = m_arrs.find(ident);
                if (arrs_it != m_arrs.end()) {
                    // TODO: Implement array value setting
                }
                return true;
            });
        //(void)s.checkErr();
    }
}

size_t VlRandomizer::emitConcatAll(std::ostream& os) const {
    size_t width = 0;
    os << "(concat";

    for (auto& var : m_vars) {
        os << ' ' << var.second.name;
        width += var.second.width;
    }

    for (auto& arr : m_arrs) {
        size_t idxWidth = vlRandom::sizeWidth(arr.second.length);
        os << " (concat";
        for (size_t idx = 0; idx < arr.second.length; ++idx) {
            os << " (select " << arr.second.arrName << ' ';
            vlRandom::emitSMTFormatConst(os, idx, idxWidth);
            os << ')';
        }
        width += arr.second.length * arr.second.width;
    }
    os << ')';
    return width;
}

void VlRandomizer::hard(std::string&& constraint) {
    m_constraints.emplace_back(std::move(constraint));
}

void VlRandomizer::clear() { m_constraints.clear(); }

#ifdef VL_DEBUG
void VlRandomizer::dump() const {
    for (const auto& var : m_vars) {
        std::stringstream ss;
        vlRandom::smtEmitVarDecl(ss, var.second);
        VL_PRINTF("Variable (.%zu.): %s\n", var.second.width, ss.str().c_str());
    }
    for (const auto& arr : m_arrs) {
        std::stringstream ss;
        vlRandom::smtEmitArrDecl(ss, arr.second);
        VL_PRINTF("Array (.%zu. [%zu]): %s\n", arr.second.width, arr.second.length,
                  ss.str().c_str());
    }
    for (const std::string& c : m_constraints) VL_PRINTF("Constraint: %s\n", c.c_str());
}
#endif
