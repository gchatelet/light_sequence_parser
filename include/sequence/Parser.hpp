#ifndef SEQUENCEPARSERTRIE_HPP_
#define SEQUENCEPARSERTRIE_HPP_

#include <sequence/Item.hpp>
#include <functional>

namespace sequence {

/**
 * Splitting strategies
 *
 * When a sequence contains several locations we need a way to decide which
 * location will be used to split the set and then try to group files.
 * eg :  file-01-01.jpg
 *       file-01-02.jpg
 *       file-02-02.jpg
 *       file-03-02.jpg
 *
 * A few predefined strategies are available
 * - RETAIN_NONE
 *  no location is kept, flattening the sequence as individual files.
 *       file-01-01.jpg
 *       file-01-02.jpg
 *       file-02-02.jpg
 *       file-03-02.jpg
 *
 * - RETAIN_FIRST_LOCATION
 *  the first location will be used kept, thus grouping the files according to
 *  the second location.
 *       file-##-02.jpg (1-3)
 *       file-01-01.jpg
 *
 * - RETAIN_LAST_LOCATION
 *  the last location will be used kept, thus grouping the files according to
 *  the first location.
 *       file-01-##.jpg (1-2)
 *       file-02-02.jpg
 *       file-03-02.jpg
 *
 * - RETAIN_HIGHEST_VARIANCE
 *  the location with the maximum variance will be kept. Here the first
 *  location contains 1,2,3 and the second 1,2 : the first location will be
 *  kept.
 *       file-##-02.jpg (1-3)
 *       file-01-01.jpg
 *
 */
enum SplitIndexStrategy {
	RETAIN_NONE, RETAIN_LAST_LOCATION, RETAIN_HIGHEST_VARIANCE, RETAIN_FIRST_LOCATION
};

/**
 * A simple structure to setup the parser
 */
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
 * Structure returned by the parser
 */
struct FolderContent {
    STRING name; // parsed folder name
    Items directories, files;
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
