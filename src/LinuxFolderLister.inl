/*
 * LinuxFolderLister.inl
 *
 *  Created on: Nov 24, 2012
 *      Author: Guillaume Chatelet
 */

#ifndef LINUXFOLDERLISTER_INL_
#define LINUXFOLDERLISTER_INL_

#include <dirent.h>

struct Lister {
private:
	DIR *pDir;
	struct dirent *direntry;
public:
	Lister(const char* pFilename) :
			pDir(opendir(pFilename)), direntry(nullptr) {
	}

	std::function<bool(sequence::FilesystemEntry&)> operator()() {
		return [&](sequence::FilesystemEntry &entry) -> bool {
			if(!pDir) {
				return false;
			}
			direntry = readdir(pDir);
			if(direntry==nullptr) {
				return false;
			}
			entry.pFilename = direntry->d_name;
			entry.isDirectory = direntry->d_type != DT_REG;
			return true;
		};
	}

	~Lister() {
		if (pDir)
			closedir(pDir);
	}
};

#endif /* LINUXFOLDERLISTER_INL_ */
