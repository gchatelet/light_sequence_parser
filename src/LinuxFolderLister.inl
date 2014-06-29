#ifndef LINUXFOLDERLISTER_INL_
#define LINUXFOLDERLISTER_INL_

#include <dirent.h>
#include <sys/stat.h>

struct Lister {
private:
  const char * const pDirectory;
  DIR *pDir;
  struct dirent *direntry;
  std::string symlink_buffer;
public:
  Lister(const char* pFilename) :
      pDirectory(pFilename), pDir(opendir(pFilename)), direntry(nullptr) {
  }

  int resolveLinkMode(const struct dirent * const direntry) {
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

  std::function<bool(sequence::FilesystemEntry&)> operator()() {
    return [&](sequence::FilesystemEntry &entry) -> bool {
      if(!pDir) {
        return false;
      }
      for(;;) {
        direntry = readdir(pDir);
        if(!direntry) {
          return false;
        }
        const int st_mode = direntry->d_type == DT_LNK ? resolveLinkMode(direntry): direntry->d_type;
        if (st_mode == DT_DIR) {
          entry.isDirectory = true;
        } else if (st_mode == DT_REG) {
          entry.isDirectory = false;
        } else {
          continue;
        }
        entry.pFilename = direntry->d_name;
        return true;
      }
    };
  }

  ~Lister() {
    if (pDir)
      closedir(pDir);
  }
};

#endif /* LINUXFOLDERLISTER_INL_ */
