/*
 * sequenceparsertrie.hpp
 *
 *  Created on: Sep 8, 2012
 *      Author: Guillaume Chatelet
 */

#ifndef SEQUENCEPARSERTRIE_HPP_
#define SEQUENCEPARSERTRIE_HPP_

#include <sequence/Item.hpp>
#include <functional>

namespace sequence {
/**
 * PatternSet splitting strategies
 * They choose a location index as a pivot for splitting the set
 */
enum SplitIndexStrategy {
	RETAIN_NONE, RETAIN_LAST_LOCATION, RETAIN_HIGHEST_VARIANCE, RETAIN_FIRST_LOCATION
};

struct Configuration {
	SplitIndexStrategy getPivotIndex;
	bool mergePadding;
	bool pack;
	bool bakeSingleton;
	bool sort;
	Configuration() :
			getPivotIndex(RETAIN_HIGHEST_VARIANCE), mergePadding(false), pack(false), bakeSingleton(false), sort(false) {
	}
};

/**
 * Standard function to parse a file system directory
 */
FolderContent parseDir(const Configuration &configuration, const CHAR* foldername);

/**
 * Special function to parse a custom representation.
 * Just pass in a GetNextEntryFunction function.
 */
struct FilesystemEntry {
	const CHAR* pFilename;
	bool isDirectory;
};
typedef std::function<bool(FilesystemEntry&)> GetNextEntryFunction;
FolderContent parse(const Configuration &config, const GetNextEntryFunction &getNextEntry);

} /* namespace sequence */

#endif /* SEQUENCEPARSERTRIE_HPP_ */
