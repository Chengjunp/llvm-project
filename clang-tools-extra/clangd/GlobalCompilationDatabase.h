//===--- GlobalCompilationDatabase.h -----------------------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANGD_GLOBALCOMPILATIONDATABASE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANGD_GLOBALCOMPILATIONDATABASE_H

#include "ProjectModules.h"
#include "support/Function.h"
#include "support/Path.h"
#include "support/Threading.h"
#include "support/ThreadsafeFS.h"
#include "clang/Tooling/ArgumentsAdjusters.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/StringMap.h"
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace clang {
namespace clangd {

struct ProjectInfo {
  // The directory in which the compilation database was discovered.
  // Empty if directory-based compilation database discovery was not used.
  std::string SourceRoot;
};

/// Provides compilation arguments used for parsing C and C++ files.
class GlobalCompilationDatabase {
public:
  virtual ~GlobalCompilationDatabase() = default;

  /// If there are any known-good commands for building this file, returns one.
  virtual std::optional<tooling::CompileCommand>
  getCompileCommand(PathRef File) const = 0;

  /// Finds the closest project to \p File.
  virtual std::optional<ProjectInfo> getProjectInfo(PathRef File) const {
    return std::nullopt;
  }

  /// Get the modules in the closest project to \p File
  virtual std::unique_ptr<ProjectModules>
  getProjectModules(PathRef File) const {
    return nullptr;
  }

  /// Makes a guess at how to build a file.
  /// The default implementation just runs clang on the file.
  /// Clangd should treat the results as unreliable.
  virtual tooling::CompileCommand getFallbackCommand(PathRef File) const;

  /// If the CDB does any asynchronous work, wait for it to complete.
  /// For use in tests.
  virtual bool blockUntilIdle(Deadline D) const { return true; }

  using CommandChanged = Event<std::vector<std::string>>;
  /// The callback is notified when files may have new compile commands.
  /// The argument is a list of full file paths.
  CommandChanged::Subscription watch(CommandChanged::Listener L) const {
    return OnCommandChanged.observe(std::move(L));
  }

protected:
  mutable CommandChanged OnCommandChanged;
};

// Helper class for implementing GlobalCompilationDatabases that wrap others.
class DelegatingCDB : public GlobalCompilationDatabase {
public:
  DelegatingCDB(const GlobalCompilationDatabase *Base);
  DelegatingCDB(std::unique_ptr<GlobalCompilationDatabase> Base);

  std::optional<tooling::CompileCommand>
  getCompileCommand(PathRef File) const override;

  std::optional<ProjectInfo> getProjectInfo(PathRef File) const override;

  std::unique_ptr<ProjectModules>
  getProjectModules(PathRef File) const override;

  tooling::CompileCommand getFallbackCommand(PathRef File) const override;

  bool blockUntilIdle(Deadline D) const override;

private:
  const GlobalCompilationDatabase *Base;
  std::unique_ptr<GlobalCompilationDatabase> BaseOwner;
  CommandChanged::Subscription BaseChanged;
};

/// Gets compile args from tooling::CompilationDatabases built for parent
/// directories.
class DirectoryBasedGlobalCompilationDatabase
    : public GlobalCompilationDatabase {
public:
  struct Options {
    Options(const ThreadsafeFS &TFS) : TFS(TFS) {}

    const ThreadsafeFS &TFS;
    // Frequency to check whether e.g. compile_commands.json has changed.
    std::chrono::steady_clock::duration RevalidateAfter =
        std::chrono::seconds(5);
    // Frequency to check whether e.g. compile_commands.json has been created.
    // (This is more expensive to check frequently, as we check many locations).
    std::chrono::steady_clock::duration RevalidateMissingAfter =
        std::chrono::seconds(30);
    // Used to provide per-file configuration.
    std::function<Context(llvm::StringRef)> ContextProvider;
    // Only look for a compilation database in this one fixed directory.
    // FIXME: fold this into config/context mechanism.
    std::optional<Path> CompileCommandsDir;
  };

  DirectoryBasedGlobalCompilationDatabase(const Options &Opts);
  ~DirectoryBasedGlobalCompilationDatabase() override;

  /// Scans File's parents looking for compilation databases.
  /// Any extra flags will be added.
  /// Might trigger OnCommandChanged, if CDB wasn't broadcasted yet.
  std::optional<tooling::CompileCommand>
  getCompileCommand(PathRef File) const override;

  /// Returns the path to first directory containing a compilation database in
  /// \p File's parents.
  std::optional<ProjectInfo> getProjectInfo(PathRef File) const override;

  std::unique_ptr<ProjectModules>
  getProjectModules(PathRef File) const override;

  bool blockUntilIdle(Deadline Timeout) const override;

private:
  Options Opts;

  class DirectoryCache;
  // Keyed by possibly-case-folded directory path.
  // We can hand out pointers as they're stable and entries are never removed.
  mutable llvm::StringMap<DirectoryCache> DirCaches;
  mutable std::mutex DirCachesMutex;

  std::vector<DirectoryCache *>
  getDirectoryCaches(llvm::ArrayRef<llvm::StringRef> Dirs) const;

  struct CDBLookupRequest {
    PathRef FileName;
    // Whether this lookup should trigger discovery of the CDB found.
    bool ShouldBroadcast = false;
    // Cached results newer than this are considered fresh and not checked
    // against disk.
    std::chrono::steady_clock::time_point FreshTime;
    std::chrono::steady_clock::time_point FreshTimeMissing;
  };
  struct CDBLookupResult {
    std::shared_ptr<const tooling::CompilationDatabase> CDB;
    ProjectInfo PI;
  };
  std::optional<CDBLookupResult> lookupCDB(CDBLookupRequest Request) const;

  class BroadcastThread;
  std::unique_ptr<BroadcastThread> Broadcaster;

  // Performs broadcast on governed files.
  void broadcastCDB(CDBLookupResult Res) const;

  // cache test calls lookupCDB directly to ensure valid/invalid times.
  friend class DirectoryBasedGlobalCompilationDatabaseCacheTest;
};

/// Extracts system include search path from drivers matching QueryDriverGlobs
/// and adds them to the compile flags.
/// Returns null when \p QueryDriverGlobs is empty.
using SystemIncludeExtractorFn = llvm::unique_function<void(
    tooling::CompileCommand &, llvm::StringRef) const>;
SystemIncludeExtractorFn
getSystemIncludeExtractor(llvm::ArrayRef<std::string> QueryDriverGlobs);

/// Wraps another compilation database, and supports overriding the commands
/// using an in-memory mapping.
class OverlayCDB : public DelegatingCDB {
public:
  // Makes adjustments to a tooling::CompileCommand which will be used to
  // process a file (possibly different from the one in the command).
  using CommandMangler = llvm::unique_function<void(tooling::CompileCommand &,
                                                    StringRef File) const>;

  // Base may be null, in which case no entries are inherited.
  // FallbackFlags are added to the fallback compile command.
  // Adjuster is applied to all commands, fallback or not.
  OverlayCDB(const GlobalCompilationDatabase *Base,
             std::vector<std::string> FallbackFlags = {},
             CommandMangler Mangler = nullptr);

  std::optional<tooling::CompileCommand>
  getCompileCommand(PathRef File) const override;
  tooling::CompileCommand getFallbackCommand(PathRef File) const override;

  /// Sets or clears the compilation command for a particular file.
  /// Returns true if the command was changed (including insertion and removal),
  /// false if it was unchanged.
  bool
  setCompileCommand(PathRef File,
                    std::optional<tooling::CompileCommand> CompilationCommand);

  std::unique_ptr<ProjectModules>
  getProjectModules(PathRef File) const override;

private:
  mutable std::mutex Mutex;
  llvm::StringMap<tooling::CompileCommand> Commands; /* GUARDED_BY(Mut) */
  CommandMangler Mangler;
  std::vector<std::string> FallbackFlags;
};

} // namespace clangd
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANGD_GLOBALCOMPILATIONDATABASE_H
