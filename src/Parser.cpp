#include "sequence/Parser.hpp"

#include <string>

#if defined(_WIN64) || defined(_WIN32)
#include <Windows.h>
#elif defined(__APPLE__) || defined(__linux)
#include <dirent.h>
#include <sys/stat.h>
#endif

#include "sequence/details/Utils.hpp"
#include "sequence/details/ParserUtils.hpp"

using namespace sequence::details;

namespace sequence {

FolderContent parse(const Configuration &config,
                    GetNextEntryFunction getNextEntry) {
  FolderContent result;
  Items &directories = result.directories;
  Items &files = result.files;
  // Scanning and bucketing files.
  FileBucketizer bucketizer;
  FilesystemEntry entry;
  while (getNextEntry(entry)) {
    if (entry.isDirectory) {
      directories.emplace_back(entry.filename);
    } else {
      bucketizer.ingest(entry.filename);
    }
  }
  // Splitting recursively to retain a single location.
  auto buckets = splitAllAndSort(config.getPivotIndex, bucketizer.transfer());
  // Merging padding if necessary.
  if (config.mergePadding && buckets.size() >= 2) {
    mergeCompatiblePadding(buckets);
  }
  // Packing indices if needed.
  if (config.pack) {
    for (auto &bucket : buckets) {
      bucket.pack();
    }
  }
  // Output items.
  for (auto &bucket : buckets) {
    bucket.output(config.bakeSingleton,
                  [&files](Item item) { files.push_back(std::move(item)); });
  }
  // Sorting if needed.
  if (config.sort) {
    sortIfNeeded(directories);
    sortIfNeeded(buckets);
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
#if defined(_WIN64) || defined(_WIN32)
struct Lister {
private:
  WIN32_FIND_DATA fdFile;
  HANDLE hFind;
  std::string tmp;
  bool noMoreFile;

public:
  Lister(const char *pFilename) {
    tmp = pFilename;
    tmp += "\\*";
    hFind = FindFirstFile(tmp.c_str(), &fdFile);
    noMoreFile = hFind == INVALID_HANDLE_VALUE;
  }

  GetNextEntryFunction getNextEntryFunction() {
    return [&](sequence::FilesystemEntry &entry) -> bool {
      if (noMoreFile)
        return false;
      tmp = fdFile.cFileName;
      entry.pFilename = tmp.c_str();
      entry.isDirectory = fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
      noMoreFile = !FindNextFile(hFind, &fdFile);
      return true;
    };
  }

  ~Lister() {
    if (hFind != INVALID_HANDLE_VALUE)
      FindClose(hFind);
  }
};
////////////////////////////////////////////////////////////////////////////////
#elif defined(__APPLE__) || defined(__linux)
////////////////////////////////////////////////////////////////////////////////
struct Lister {
private:
  const char *const pDirectory;
  DIR *pDir;
  struct dirent *direntry;
  std::string symlink_buffer;

public:
  Lister(const char *pFilename)
      : pDirectory(pFilename), pDir(opendir(pFilename)), direntry(nullptr) {}

  int resolveLinkMode(const struct dirent *const direntry) {
    symlink_buffer.clear();
    symlink_buffer += pDirectory;
    symlink_buffer += '/';
    symlink_buffer += direntry->d_name;
    struct stat stats;
    if (stat(symlink_buffer.c_str(), &stats) == 0) {
      if (S_ISREG(stats.st_mode))
        return DT_REG;
      if (S_ISDIR(stats.st_mode))
        return DT_DIR;
    }
    return DT_UNKNOWN;
  }

  GetNextEntryFunction getNextEntryFunction() {
    return [&](sequence::FilesystemEntry &entry) -> bool {
      if (!pDir) {
        return false;
      }
      for (;;) {
        direntry = readdir(pDir);
        if (!direntry) {
          return false;
        }
        const int st_mode = direntry->d_type == DT_LNK
                                ? resolveLinkMode(direntry)
                                : direntry->d_type;
        if (st_mode == DT_DIR) {
          entry.isDirectory = true;
        } else if (st_mode == DT_REG) {
          entry.isDirectory = false;
        } else {
          continue;
        }
        entry.filename = direntry->d_name;
        return true;
      }
    };
  }

  ~Lister() {
    if (pDir)
      closedir(pDir);
  }
};
////////////////////////////////////////////////////////////////////////////////
#else
#error "Unsupported platform"
#endif

FolderContent parseDir(const Configuration &configuration,
                       CStringView foldername) {
  Lister lister(foldername.ptr());
  auto content = parse(configuration, lister.getNextEntryFunction());
  content.name = foldername.toString();
  return content;
}

} // namespace sequence
