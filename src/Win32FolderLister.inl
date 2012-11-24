#ifndef WIN32FOLDERLISTER_INL_
#define WIN32FOLDERLISTER_INL_

#include <Windows.h>
#include <string>

struct Lister {
private:
	WIN32_FIND_DATA fdFile;
	HANDLE hFind;
	std::string tmp;
	bool noMoreFile;
public:
	Lister(const char* pFilename) {
		tmp = pFilename;
		tmp += "\\*";
		hFind = FindFirstFile(tmp.c_str(), &fdFile);
		noMoreFile = hFind == INVALID_HANDLE_VALUE;
	}

	std::function<bool(sequence::FilesystemEntry&)> operator()() {
		return [&](sequence::FilesystemEntry &entry) -> bool {
			if(noMoreFile) 
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

#endif /* WIN32FOLDERLISTER_INL_ */
