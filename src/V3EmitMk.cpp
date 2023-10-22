// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit Makefile
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2004-2024 by Wilson Snyder. This program is free software; you
// can redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3EmitMk.h"

#include "V3EmitCBase.h"
#include "V3HierBlock.h"
#include "V3Os.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Emit statements and expressions

class EmitMk final {
public:
    // METHODS

    struct FileOrConcatenatedFilesList {
        std::string fileName{};
        std::vector<std::string> concatenatedFileNames{};

        bool isConcatenatingFile() const { return !concatenatedFileNames.empty(); }
    };

    struct FileNameWithScore {
        std::string name;
        std::int64_t score;
    };

    static std::vector<FileOrConcatenatedFilesList>
    singleConcatenatedFilesList(std::vector<FileNameWithScore> inputFiles, std::int64_t totalScore,
                                std::string concatenatedFilePrefix) {
        // Data of a single work unit. Used only in this function.
        struct WorkList {

            std::int64_t totalScore{0};
            std::vector<FileNameWithScore> files{};
            // Number of buckets assigned for this list. Used only in concatenable lists.
            int bucketsCount{0};
            bool isConcatenable{true};
        };

        static const auto convertInputFilesToOutputFiles
            = [](std::vector<FileNameWithScore> inputFiles) {
                  std::vector<FileOrConcatenatedFilesList> outputFiles;
                  outputFiles.reserve(inputFiles.size());
                  for (auto& file : inputFiles) {
                      outputFiles.push_back({/*fileName=*/std::move(file.name)});
                  }
                  return outputFiles;
              };

        const int bucketsCount = v3Global.opt.outputSplitJobs();

        // MAIN PARAMETERS

        // Arbitrary number, mostlikely could be larger
        constexpr std::size_t MinFilesCount = 100;

        // Arbitrary ratio.
        constexpr std::size_t MinFilesPerBucketOnAvg = 4;

        // Definitely should not exceed `totalScore / bucketsCount`.
        const std::int64_t ConcatenableFileMaxScore = totalScore / bucketsCount / 2;

        // ---

        // Return early if there's nothing to do.
        if ((inputFiles.size() < MinFilesCount)
            || (inputFiles.size() < (bucketsCount * MinFilesPerBucketOnAvg))) {
            UINFO(0, "File concatenation skipped - too few files");
            return convertInputFilesToOutputFiles(std::move(inputFiles));
        }

        // Create initial work lists.

        std::vector<WorkList> workLists{};

        for (const auto& inputFile : inputFiles) {
            const bool isConcatenable = (inputFile.score <= ConcatenableFileMaxScore);
            if (!isConcatenable && debug() >= 5) {
                UINFO(5, "Exceeds max score (" << ConcatenableFileMaxScore
                                               << "), excluding: " << inputFile.name << endl);
            }
            // Add new list if the last list's concatenability does not match the inputFile's
            // concatenability
            if (workLists.empty() || workLists.back().isConcatenable != isConcatenable) {
                workLists.push_back(WorkList{});
                workLists.back().isConcatenable = isConcatenable;
            }
            // Add inputFile to the last list
            auto& list = workLists.back();
            list.files.push_back({inputFile.name, inputFile.score});
            list.totalScore += inputFile.score;
        }

        // Collect stats and mark lists with only one file as non-concatenable

        std::size_t concatenableFilesCount = 0;
        std::int64_t concatenableFilesTotalScore = 0;
        std::size_t concatenableFilesListsCount = 0;

        std::vector<WorkList*> concatenableListsByDescSize;

        for (auto& list : workLists) {
            if (!list.isConcatenable) continue;

            // No point in "concatenating" a single file
            if (list.files.size() == 1) {
                list.isConcatenable = false;
                UINFO(5, "Single file, excluding: " << list.files[0].name << endl);
                continue;
            }

            concatenableFilesCount += list.files.size();
            concatenableFilesTotalScore += list.totalScore;
            ++concatenableFilesListsCount;
            // This vector is sorted below
            concatenableListsByDescSize.push_back(&list);
        }

        // Check concatenation conditions again using more precise data
        if ((concatenableFilesCount < MinFilesCount)
            || (concatenableFilesCount < (bucketsCount * MinFilesPerBucketOnAvg))) {
            UINFO(0, "File concatenation skipped - too few files");
            return convertInputFilesToOutputFiles(std::move(inputFiles));
        }

        std::sort(concatenableListsByDescSize.begin(), concatenableListsByDescSize.end(),
                  [](const WorkList* ap, const WorkList* bp) {
                      // Sort in descending order by files count
                      if (ap->files.size() != bp->files.size())
                          return (ap->files.size() > bp->files.size());
                      // As a fallback sort in ascending order by totalSize. This makes lists
                      // with higher score more likely to be excluded.
                      return bp->totalScore > ap->totalScore;
                  });

        // Assign buckets to lists

        // More concatenable lists than buckets. Exclude lists with lowest number of files.
        if (concatenableFilesListsCount > bucketsCount) {
            std::for_each(concatenableListsByDescSize.begin() + bucketsCount,
                          concatenableListsByDescSize.end(), [](auto* listp) {
                              listp->isConcatenable = false;
                              if (debug() >= 5) {
                                  for (const auto& file : listp->files) {
                                      UINFO(5, "Not enough buckets, excluding: " << file.name
                                                                                 << endl);
                                  }
                              }
                          });
            concatenableListsByDescSize.resize(bucketsCount);
            concatenableFilesListsCount = bucketsCount;
            // Recalculate stats
            concatenableFilesCount = 0;
            concatenableFilesTotalScore = 0;
            for (auto* listp : concatenableListsByDescSize) {
                concatenableFilesCount += listp->files.size();
                concatenableFilesTotalScore += listp->totalScore;
            }
        }
        const std::int64_t idealBucketScore = concatenableFilesTotalScore / concatenableFilesCount;

        int availableBuckets = bucketsCount;
        for (auto* listp : concatenableListsByDescSize) {
            if (availableBuckets > 0) {
                listp->bucketsCount = std::min<int>(
                    availableBuckets, std::max<int>(1, listp->totalScore / idealBucketScore));
                availableBuckets -= listp->bucketsCount;
            } else {
                if (debug() >= 5) {
                    for (const auto& file : listp->files) {
                        UINFO(5, "Out of buckets, excluding: " << file.name << endl);
                    }
                }
                // Out of buckets. Instead of recalculating everything just exclude the list.
                listp->isConcatenable = false;
            }
        }

        // Assign files to buckets and build final list of files

        std::vector<FileOrConcatenatedFilesList> outputFiles;

        // At this point the workLists contains concatenatable lists separated by one or more
        // non-concatenable lists. Each concatenatable list has N buckets (where N > 0) which have
        // to be filled with files from the list. Ideally, sum of file scores in each bucket should
        // be equal.
        int concatenatedFileId = 0;
        for (auto& list : workLists) {
            if (!list.isConcatenable) {
                for (auto& file : list.files) {
                    FileOrConcatenatedFilesList e{};
                    e.fileName = std::move(file.name);
                    outputFiles.push_back(std::move(e));
                }
                continue;
            }

            const std::int64_t listIdealBucketScore = list.totalScore / list.bucketsCount;
            auto fileIt = list.files.begin();
            for (int i = 0; i < list.bucketsCount; ++i) {
                FileOrConcatenatedFilesList bucket;
                std::int64_t bucketScore = 0;

                bucket.fileName = concatenatedFilePrefix + std::to_string(concatenatedFileId);
                ++concatenatedFileId;

                for (; fileIt != list.files.end(); ++fileIt) {
                    auto diffNow = std::abs(listIdealBucketScore - bucketScore);
                    auto diffIfAdded
                        = std::abs(listIdealBucketScore - bucketScore - fileIt->score);
                    if (bucketScore == 0 || diffNow > diffIfAdded) {
                        // Bucket score will be better with the file in it
                        bucketScore += fileIt->score;
                        bucket.concatenatedFileNames.push_back(std::move(fileIt->name));
                    } else {
                        // Best possible bucket score reached, process next bucket
                        break;
                    }
                }

                outputFiles.push_back(std::move(bucket));
            }
        }

        return outputFiles;
    }

    static void emitConcatenatingFile(const FileOrConcatenatedFilesList& entry) {
        UASSERT(entry.isConcatenatingFile(), "Passed entry does not represent concatenating file");

        V3OutCFile concatenatingFile{v3Global.opt.makeDir() + "/" + entry.fileName + ".cpp"};
        concatenatingFile.putsHeader();
        for (const auto& file : entry.concatenatedFileNames) {
            concatenatingFile.puts("#include \"" + file + ".cpp\"\n");
        }
    }

    void putMakeClassEntry(V3OutMkFile& of, const string& name) {
        of.puts("\t" + V3Os::filenameNonDirExt(name) + " \\\n");
    }

    void emitClassMake() {
        std::vector<FileOrConcatenatedFilesList> vmClassesSlowList;
        std::vector<FileOrConcatenatedFilesList> vmClassesFastList;
        if (v3Global.opt.outputSplitJobs() > 0) {
            std::vector<FileNameWithScore> slowFiles;
            std::vector<FileNameWithScore> fastFiles;
            std::int64_t slowTotalScore;
            std::int64_t fastTotalScore;

            for (AstNodeFile* nodep = v3Global.rootp()->filesp(); nodep;
                 nodep = VN_AS(nodep->nextp(), NodeFile)) {
                const AstCFile* const cfilep = VN_CAST(nodep, CFile);
                if (cfilep && cfilep->source() && cfilep->support() == false) {
                    auto& files = cfilep->slow() ? slowFiles : fastFiles;
                    auto& totalScore = cfilep->slow() ? slowTotalScore : fastTotalScore;

                    FileNameWithScore f;
                    f.name = V3Os::filenameNonDirExt(cfilep->name());
                    f.score = cfilep->complexityScore();

                    totalScore += f.score;
                    files.push_back(std::move(f));
                }
            }

            vmClassesSlowList = singleConcatenatedFilesList(std::move(slowFiles), slowTotalScore,
                                                            "vm_classes_slow_");
            vmClassesFastList = singleConcatenatedFilesList(std::move(fastFiles), fastTotalScore,
                                                            "vm_classes_fast_");
        }

        // Generate the makefile
        V3OutMkFile of{v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + "_classes.mk"};
        of.putsHeader();
        of.puts("# DESCR"
                "IPTION: Verilator output: Make include file with class lists\n");
        of.puts("#\n");
        of.puts("# This file lists generated Verilated files, for including "
                "in higher level makefiles.\n");
        of.puts("# See " + v3Global.opt.prefix() + ".mk" + " for the caller.\n");

        of.puts("\n### Switches...\n");
        of.puts("# C11 constructs required?  0/1 (always on now)\n");
        of.puts("VM_C11 = 1\n");
        of.puts("# Timing enabled?  0/1\n");
        of.puts("VM_TIMING = ");
        of.puts(v3Global.usesTiming() ? "1" : "0");
        of.puts("\n");
        of.puts("# Coverage output mode?  0/1 (from --coverage)\n");
        of.puts("VM_COVERAGE = ");
        of.puts(v3Global.opt.coverage() ? "1" : "0");
        of.puts("\n");
        of.puts("# Parallel builds?  0/1 (from --output-split)\n");
        of.puts("VM_PARALLEL_BUILDS = ");
        of.puts(v3Global.useParallelBuild() ? "1" : "0");
        of.puts("\n");
        of.puts("# Number of concatenated files per type.  0+ (from --output-split-jobs)\n");
        of.puts("VM_SPLIT_JOBS = ");
        of.puts(v3Global.useParallelBuild() ? std::to_string(v3Global.opt.outputSplitJobs())
                                            : "0");
        of.puts("\n");
        of.puts("# Tracing output mode?  0/1 (from --trace/--trace-fst)\n");
        of.puts("VM_TRACE = ");
        of.puts(v3Global.opt.trace() ? "1" : "0");
        of.puts("\n");
        of.puts("# Tracing output mode in VCD format?  0/1 (from --trace)\n");
        of.puts("VM_TRACE_VCD = ");
        of.puts(v3Global.opt.trace() && v3Global.opt.traceFormat().vcd() ? "1" : "0");
        of.puts("\n");
        of.puts("# Tracing output mode in FST format?  0/1 (from --trace-fst)\n");
        of.puts("VM_TRACE_FST = ");
        of.puts(v3Global.opt.trace() && v3Global.opt.traceFormat().fst() ? "1" : "0");
        of.puts("\n");

        of.puts("\n### Object file lists...\n");
        for (int support = 0; support < 3; ++support) {
            for (const bool& slow : {false, true}) {
                if (support == 2) {
                    of.puts("# Global classes, need linked once per executable");
                } else if (support) {
                    of.puts("# Generated support classes");
                } else {
                    of.puts("# Generated module classes");
                }
                if (slow) {
                    of.puts(", non-fast-path, compile with low/medium optimization\n");
                } else {
                    of.puts(", fast-path, compile with highest optimization\n");
                }
                of.puts(support == 2 ? "VM_GLOBAL" : support == 1 ? "VM_SUPPORT" : "VM_CLASSES");
                of.puts(slow ? "_SLOW" : "_FAST");
                of.puts(" += \\\n");
                if (support == 2 && v3Global.opt.hierChild()) {
                    // Do nothing because VM_GLOBAL is necessary per executable. Top module will
                    // have them.
                } else if (support == 2 && !slow) {
                    putMakeClassEntry(of, "verilated.cpp");
                    if (v3Global.dpi()) putMakeClassEntry(of, "verilated_dpi.cpp");
                    if (v3Global.opt.vpi()) putMakeClassEntry(of, "verilated_vpi.cpp");
                    if (v3Global.opt.savable()) putMakeClassEntry(of, "verilated_save.cpp");
                    if (v3Global.opt.coverage()) putMakeClassEntry(of, "verilated_cov.cpp");
                    if (v3Global.opt.trace()) {
                        putMakeClassEntry(of, v3Global.opt.traceSourceBase() + "_c.cpp");
                    }
                    if (v3Global.usesProbDist()) putMakeClassEntry(of, "verilated_probdist.cpp");
                    if (v3Global.usesTiming()) putMakeClassEntry(of, "verilated_timing.cpp");
                    putMakeClassEntry(of, "verilated_threads.cpp");
                    if (v3Global.opt.usesProfiler()) {
                        putMakeClassEntry(of, "verilated_profiler.cpp");
                    }
                } else if (support == 2 && slow) {
                } else if ((support == 0) && (v3Global.opt.outputSplitJobs() > 0)) {
                    const auto& list = slow ? vmClassesSlowList : vmClassesFastList;
                    for (const auto& entry : list) {
                        if (entry.isConcatenatingFile()) { emitConcatenatingFile(entry); }
                        putMakeClassEntry(of, entry.fileName);
                    }
                } else {
                    for (AstNodeFile* nodep = v3Global.rootp()->filesp(); nodep;
                         nodep = VN_AS(nodep->nextp(), NodeFile)) {
                        const AstCFile* const cfilep = VN_CAST(nodep, CFile);
                        if (cfilep && cfilep->source() && cfilep->slow() == (slow != 0)
                            && cfilep->support() == (support != 0)) {
                            putMakeClassEntry(of, cfilep->name());
                        }
                    }
                }
                of.puts("\n");
            }
        }

        of.puts("\n");
        of.putsHeader();
    }

    void emitOverallMake() {
        // Generate the makefile
        V3OutMkFile of{v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".mk"};
        of.putsHeader();
        of.puts("# DESCR"
                "IPTION: Verilator output: "
                "Makefile for building Verilated archive or executable\n");
        of.puts("#\n");
        of.puts("# Execute this makefile from the object directory:\n");
        of.puts("#    make -f " + v3Global.opt.prefix() + ".mk" + "\n");
        of.puts("\n");

        if (v3Global.opt.exe()) {
            of.puts("default: " + v3Global.opt.exeName() + "\n");
        } else if (!v3Global.opt.libCreate().empty()) {
            of.puts("default: lib" + v3Global.opt.libCreate() + "\n");
        } else {
            of.puts("default: lib" + v3Global.opt.prefix() + "\n");
        }
        of.puts("\n### Constants...\n");
        of.puts("# Perl executable (from $PERL)\n");
        of.puts("PERL = " + V3OutFormatter::quoteNameControls(V3Options::getenvPERL()) + "\n");
        of.puts("# Path to Verilator kit (from $VERILATOR_ROOT)\n");
        of.puts("VERILATOR_ROOT = "
                + V3OutFormatter::quoteNameControls(V3Options::getenvVERILATOR_ROOT()) + "\n");
        of.puts("# SystemC include directory with systemc.h (from $SYSTEMC_INCLUDE)\n");
        of.puts(string{"SYSTEMC_INCLUDE ?= "} + V3Options::getenvSYSTEMC_INCLUDE() + "\n");
        of.puts("# SystemC library directory with libsystemc.a (from $SYSTEMC_LIBDIR)\n");
        of.puts(string{"SYSTEMC_LIBDIR ?= "} + V3Options::getenvSYSTEMC_LIBDIR() + "\n");

        // Only check it if we really need the value
        if (v3Global.opt.systemC() && !V3Options::systemCFound()) {
            v3fatal("Need $SYSTEMC_INCLUDE in environment or when Verilator configured,\n"
                    "and need $SYSTEMC_LIBDIR in environment or when Verilator configured\n"
                    "Probably System-C isn't installed, see http://www.systemc.org\n");
        }

        of.puts("\n### Switches...\n");
        of.puts("# C++ code coverage  0/1 (from --prof-c)\n");
        of.puts(string{"VM_PROFC = "} + ((v3Global.opt.profC()) ? "1" : "0") + "\n");
        of.puts("# SystemC output mode?  0/1 (from --sc)\n");
        of.puts(string{"VM_SC = "} + ((v3Global.opt.systemC()) ? "1" : "0") + "\n");
        of.puts("# Legacy or SystemC output mode?  0/1 (from --sc)\n");
        of.puts(string{"VM_SP_OR_SC = $(VM_SC)\n"});
        of.puts("# Deprecated\n");
        of.puts(string{"VM_PCLI = "} + (v3Global.opt.systemC() ? "0" : "1") + "\n");
        of.puts(
            "# Deprecated: SystemC architecture to find link library path (from $SYSTEMC_ARCH)\n");
        of.puts(string{"VM_SC_TARGET_ARCH = "} + V3Options::getenvSYSTEMC_ARCH() + "\n");

        of.puts("\n### Vars...\n");
        of.puts("# Design prefix (from --prefix)\n");
        of.puts(string{"VM_PREFIX = "} + v3Global.opt.prefix() + "\n");
        of.puts("# Module prefix (from --prefix)\n");
        of.puts(string{"VM_MODPREFIX = "} + v3Global.opt.modPrefix() + "\n");

        of.puts("# User CFLAGS (from -CFLAGS on Verilator command line)\n");
        of.puts("VM_USER_CFLAGS = \\\n");
        if (!v3Global.opt.libCreate().empty()) of.puts("\t-fPIC \\\n");
        const V3StringList& cFlags = v3Global.opt.cFlags();
        for (const string& i : cFlags) of.puts("\t" + i + " \\\n");
        of.puts("\n");

        of.puts("# User LDLIBS (from -LDFLAGS on Verilator command line)\n");
        of.puts("VM_USER_LDLIBS = \\\n");
        const V3StringList& ldLibs = v3Global.opt.ldLibs();
        for (const string& i : ldLibs) of.puts("\t" + i + " \\\n");
        of.puts("\n");

        V3StringSet dirs;
        of.puts("# User .cpp files (from .cpp's on Verilator command line)\n");
        of.puts("VM_USER_CLASSES = \\\n");
        const V3StringSet& cppFiles = v3Global.opt.cppFiles();
        for (const auto& cppfile : cppFiles) {
            of.puts("\t" + V3Os::filenameNonExt(cppfile) + " \\\n");
            const string dir = V3Os::filenameDir(cppfile);
            dirs.insert(dir);
        }
        of.puts("\n");

        of.puts("# User .cpp directories (from .cpp's on Verilator command line)\n");
        of.puts("VM_USER_DIR = \\\n");
        for (const auto& i : dirs) of.puts("\t" + i + " \\\n");
        of.puts("\n");

        of.puts("\n### Default rules...\n");
        of.puts("# Include list of all generated classes\n");
        of.puts("include " + v3Global.opt.prefix() + "_classes.mk\n");
        if (v3Global.opt.hierTop()) {
            of.puts("# Include rules for hierarchical blocks\n");
            of.puts("include " + v3Global.opt.prefix() + "_hier.mk\n");
        }
        of.puts("# Include global rules\n");
        of.puts("include $(VERILATOR_ROOT)/include/verilated.mk\n");

        if (v3Global.opt.exe()) {
            of.puts("\n### Executable rules... (from --exe)\n");
            of.puts("VPATH += $(VM_USER_DIR)\n");
            of.puts("\n");
        }

        for (const string& cppfile : cppFiles) {
            const string basename = V3Os::filenameNonExt(cppfile);
            // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
            of.puts(basename + ".o: " + cppfile + "\n");
            of.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -c -o $@ $<\n");
        }

        if (v3Global.opt.exe()) {
            of.puts("\n### Link rules... (from --exe)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            of.puts(v3Global.opt.exeName()
                    + ": $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) $(VM_PREFIX)__ALL.a $(VM_HIER_LIBS)\n");
            of.puts("\t$(LINK) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) $(LIBS) $(SC_LIBS) -o $@\n");
            of.puts("\n");
        } else if (!v3Global.opt.libCreate().empty()) {
            const string libCreateDeps = "$(VK_OBJS) $(VK_USER_OBJS) $(VK_GLOBAL_OBJS) "
                                         + v3Global.opt.libCreate() + ".o $(VM_HIER_LIBS)";
            of.puts("\n### Library rules from --lib-create\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            of.puts(v3Global.opt.libCreateName(false) + ": " + libCreateDeps + "\n");
            of.puts("\n");
            if (v3Global.opt.hierChild()) {
                // Hierarchical child does not need .so because hierTop() will create .so from .a
                of.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(false)
                        + "\n");
            } else {
                of.puts(v3Global.opt.libCreateName(true) + ": " + libCreateDeps + "\n");
                // Linker on mac emits an error if all symbols are not found here,
                // but some symbols that are referred as "DPI-C" can not be found at this moment.
                // So add dynamic_lookup
                of.puts("ifeq ($(shell uname -s),Darwin)\n");
                of.puts("\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -undefined "
                        "dynamic_lookup -shared -flat_namespace -o $@ $^\n");
                of.puts("else\n");
                of.puts(
                    "\t$(OBJCACHE) $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPT_FAST) -shared -o $@ $^\n");
                of.puts("endif\n");
                of.puts("\n");
                of.puts("lib" + v3Global.opt.libCreate() + ": " + v3Global.opt.libCreateName(false)
                        + " " + v3Global.opt.libCreateName(true) + "\n");
            }
        } else {
            const string libname = "lib" + v3Global.opt.prefix() + ".a";
            of.puts("\n### Library rules (default lib mode)\n");
            // The rule to create .a is defined in verilated.mk, so just define dependency here.
            of.puts(libname + ": $(VK_OBJS) $(VK_USER_OBJS) $(VM_HIER_LIBS)\n");
            of.puts("libverilated.a: $(VK_GLOBAL_OBJS)\n");
            // let default rule depend on '{prefix}__ALL.a', for compatibility
            of.puts("lib" + v3Global.opt.prefix() + ": " + libname
                    + " libverilated.a $(VM_PREFIX)__ALL.a\n");
        }

        of.puts("\n");
        of.putsHeader();
    }

    explicit EmitMk() {
        emitClassMake();
        emitOverallMake();
    }
    virtual ~EmitMk() = default;
};

//######################################################################
class EmitMkHierVerilation final {
    const V3HierBlockPlan* const m_planp;
    const string m_makefile;  // path of this makefile
    void emitCommonOpts(V3OutMkFile& of) const {
        const string cwd = V3Os::filenameRealPath(".");
        of.puts("# Verilation of hierarchical blocks are executed in this directory\n");
        of.puts("VM_HIER_RUN_DIR := " + cwd + "\n");
        of.puts("# Common options for hierarchical blocks\n");
        const string fullpath_bin = V3Os::filenameRealPath(v3Global.opt.buildDepBin());
        const string verilator_wrapper = V3Os::filenameDir(fullpath_bin) + "/verilator";
        of.puts("VM_HIER_VERILATOR := " + verilator_wrapper + "\n");
        of.puts("VM_HIER_INPUT_FILES := \\\n");
        const V3StringList& vFiles = v3Global.opt.vFiles();
        for (const string& i : vFiles) of.puts("\t" + V3Os::filenameRealPath(i) + " \\\n");
        of.puts("\n");
        const V3StringSet& libraryFiles = v3Global.opt.libraryFiles();
        of.puts("VM_HIER_VERILOG_LIBS := \\\n");
        for (const string& i : libraryFiles) {
            of.puts("\t" + V3Os::filenameRealPath(i) + " \\\n");
        }
        of.puts("\n");
    }
    void emitLaunchVerilator(V3OutMkFile& of, const string& argsFile) const {
        of.puts("\t@$(MAKE) -C $(VM_HIER_RUN_DIR) -f " + m_makefile
                + " hier_launch_verilator \\\n");
        of.puts("\t\tVM_HIER_LAUNCH_VERILATOR_ARGSFILE=\"" + argsFile + "\"\n");
    }
    void emit(V3OutMkFile& of) const {
        of.puts("# Hierarchical Verilation -*- Makefile -*-\n");
        of.puts("# DESCR"
                "IPTION: Verilator output: Makefile for hierarchical Verilation\n");
        of.puts("#\n");
        of.puts("# The main makefile " + v3Global.opt.prefix() + ".mk calls this makefile\n");
        of.puts("\n");
        of.puts("ifndef VM_HIER_VERILATION_INCLUDED\n");
        of.puts("VM_HIER_VERILATION_INCLUDED = 1\n\n");

        of.puts(".SUFFIXES:\n");
        of.puts(".PHONY: hier_build hier_verilation hier_launch_verilator\n");

        of.puts("# Libraries of hierarchical blocks\n");
        of.puts("VM_HIER_LIBS := \\\n");
        const V3HierBlockPlan::HierVector blocks
            = m_planp->hierBlocksSorted();  // leaf comes first
        // List in order of leaf-last order so that linker can resolve dependency
        for (auto& block : vlstd::reverse_view(blocks)) {
            of.puts("\t" + block->hierLib(true) + " \\\n");
        }
        of.puts("\n");

        // Build hierarchical libraries as soon as possible to get maximum parallelism
        of.puts("hier_build: $(VM_HIER_LIBS) " + v3Global.opt.prefix() + ".mk\n");
        of.puts("\t$(MAKE) -f " + v3Global.opt.prefix() + ".mk\n");
        of.puts("hier_verilation: " + v3Global.opt.prefix() + ".mk\n");
        emitCommonOpts(of);

        // Instead of direct execute of "cd $(VM_HIER_RUN_DIR) && $(VM_HIER_VERILATOR)",
        // call via make to get message of "Entering directory" and "Leaving directory".
        // This will make some editors and IDEs happy when viewing a logfile.
        of.puts("# VM_HIER_LAUNCH_VERILATOR_ARGSFILE must be passed as a command argument\n");
        of.puts("hier_launch_verilator:\n");
        of.puts("\t$(VM_HIER_VERILATOR) -f $(VM_HIER_LAUNCH_VERILATOR_ARGSFILE)\n");

        // Top level module
        {
            const string argsFile = v3Global.hierPlanp()->topCommandArgsFileName(false);
            of.puts("\n# Verilate the top module\n");
            of.puts(v3Global.opt.prefix()
                    + ".mk: $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            of.puts(V3Os::filenameNonDir(argsFile) + " ");
            for (const auto& itr : *m_planp) of.puts(itr.second->hierWrapper(true) + " ");
            of.puts("\n");
            emitLaunchVerilator(of, argsFile);
        }

        // Rules to process hierarchical blocks
        of.puts("\n# Verilate hierarchical blocks\n");
        for (const V3HierBlock* const blockp : m_planp->hierBlocksSorted()) {
            const string prefix = blockp->hierPrefix();
            const string argsFile = blockp->commandArgsFileName(false);
            of.puts(blockp->hierGenerated(true));
            of.puts(": $(VM_HIER_INPUT_FILES) $(VM_HIER_VERILOG_LIBS) ");
            of.puts(V3Os::filenameNonDir(argsFile) + " ");
            const V3HierBlock::HierBlockSet& children = blockp->children();
            for (V3HierBlock::HierBlockSet::const_iterator child = children.begin();
                 child != children.end(); ++child) {
                of.puts((*child)->hierWrapper(true) + " ");
            }
            of.puts("\n");
            emitLaunchVerilator(of, argsFile);

            // Rule to build lib*.a
            of.puts(blockp->hierLib(true));
            of.puts(": ");
            of.puts(blockp->hierMk(true));
            of.puts(" ");
            for (V3HierBlock::HierBlockSet::const_iterator child = children.begin();
                 child != children.end(); ++child) {
                of.puts((*child)->hierLib(true));
                of.puts(" ");
            }
            of.puts("\n\t$(MAKE) -f " + blockp->hierMk(false) + " -C " + prefix);
            of.puts(" VM_PREFIX=" + prefix);
            of.puts("\n\n");
        }
        of.puts("endif  # Guard\n");
    }

public:
    explicit EmitMkHierVerilation(const V3HierBlockPlan* planp)
        : m_planp{planp}
        , m_makefile{v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + "_hier.mk"} {
        V3OutMkFile of{m_makefile};
        emit(of);
    }
};

//######################################################################
// Gate class functions

void V3EmitMk::debugTestConcatenation(const char* inputFile) {
    UINFO(2, __FUNCTION__ << ": " << endl);
    const std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(inputFile)};
    std::vector<EmitMk::FileNameWithScore> inputList;
    std::int64_t totalScore = 0;

    EmitMk::FileNameWithScore current{};
    while ((*ifp) >> current.score >> std::ws) {
        char ch;
        while (ch = ifp->get(), ch && ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
            current.name.push_back(ch);
        }
        // std::cout << "score:" << current.score << " | '" << current.name << "'" << std::endl;
        totalScore += current.score;
        inputList.push_back(std::move(current));
        current = EmitMk::FileNameWithScore{};
    }

    const std::vector<EmitMk::FileOrConcatenatedFilesList> list
        = EmitMk::singleConcatenatedFilesList(std::move(inputList), totalScore,
                                              "concatenating_file_");

    for (const auto& entry : list) {
        if (entry.isConcatenatingFile()) {
            std::cout << entry.fileName << "  [concatenating file]" << std::endl;
            for (const auto& f : entry.concatenatedFileNames) {
                std::cout << "    " << entry.fileName << " / " << f << std::endl;
            }
        } else {
            std::cout << entry.fileName << std::endl;
        }
    }
}

void V3EmitMk::emitmk() {
    UINFO(2, __FUNCTION__ << ": " << endl);
    const EmitMk emitter;
}

void V3EmitMk::emitHierVerilation(const V3HierBlockPlan* planp) {
    UINFO(2, __FUNCTION__ << ": " << endl);
    EmitMkHierVerilation{planp};
}
