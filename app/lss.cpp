#include <sequence/Parser.hpp>
#include <cstdio>
#include <cstdlib>

namespace sequence {

inline static void printRegular(const Item& item) {
	const auto pFilename = item.filename.c_str();
	switch (item.getType()) {
	case Item::SINGLE:
		printf("%s\n", pFilename);
		break;
	case Item::INVALID:
		printf("Invalid\n");
		break;
	case Item::INDICED:
		printf("%s (%lu) %d\n", pFilename, (unsigned long) item.indices.size(), item.padding);
		break;
	case Item::PACKED:
		if (item.step == 1)
			printf("%s [%d:%d] #%d\n", pFilename, item.start, item.end, item.padding);
		else
			printf("%s [%d:%d]/%d #%d\n", pFilename, item.start, item.end, item.step, item.padding);
		break;
	}
}

inline static void printRegular(const FolderContent&result) {
	printf("\n\n* %s\n", result.name.c_str());
	for (const Item &item : result.directories)
		printRegular(item);
	printf("\n");
	for (const Item &item : result.files)
		printRegular(item);
}

inline static void printJson(const Item& item) {
	const auto pFilename = item.filename.c_str();
	switch (item.getType()) {
	case Item::SINGLE:
		printf("%s\n", pFilename);
		break;
	case Item::INVALID:
		printf("Invalid\n");
		break;
	case Item::INDICED:
		printf("%s (%lu) %d\n", pFilename, (unsigned long) item.indices.size(), item.padding);
		break;
	case Item::PACKED:
		if (item.step == 1)
			printf("%s [%d:%d] #%d\n", pFilename, item.start, item.end, item.padding);
		else
			printf("%s [%d:%d]/%d #%d\n", pFilename, item.start, item.end, item.step, item.padding);
		break;
	}
}

inline static void printJson(const FolderContent&result) {
	printf("{");
	printf(R"("path":"%s",[)", result.name.c_str());
	for (const Item &item : result.directories)
		printJson(item);
	printf("],[");
	for (const Item &item : result.files)
		printJson(item);
	printf("]");
	printf("}");
}

} // namespace sequence

static void printHelp() {
	printf("USAGE [OPTIONS] [FOLDER]\n");
	printf("--help,-h            Print help and exit\n");
	printf("--merge-padding,-m   Merge sequence with different padding\n");
	printf("--recursive,-r       Parse folder recursively\n");
	printf("--pack,-p            Drop indices and replace with one or several contiguous chunks\n");
	printf("--bake-singleton,-b  Replace Items with only one index by it's corresponding filename\n");
	printf("--sort,-s            Print folder and files lexicographically sorted\n");
//	printf("--json,-j            Output result as a json object\n");
	printf("--keep=              Strategy to handle ambiguous locations\n");
	printf("       none          flattens the set\n");
	printf("       first         keep first number\n");
	printf("       last          keep last number\n");
	printf("       max-variance  keep number with highest variance (default)\n");
	printf("                     backups to 'none' if same variance\n");
}

#include <list>

int main(int argc, char **argv) {
	using namespace std;
	using namespace sequence;

	bool recursive = false;
	bool json = false;
	Configuration configuration;
	configuration.getPivotIndex = RETAIN_HIGHEST_VARIANCE;

	string folder = ".";
	for (int i = 1; i < argc; ++i) {
		string arg = argv[i];
		if (arg == "--help" || arg == "-h") {
			printHelp();
			return 0;
		} else if (arg == "--merge-padding" || arg == "-m")
			configuration.mergePadding = true;
		else if (arg == "--pack" || arg == "-p")
			configuration.pack = true;
		else if (arg == "--recursive" || arg == "-r")
			recursive = true;
		else if (arg == "--bake-singleton" || arg == "-b")
			configuration.bakeSingleton = true;
		else if (arg == "--sort" || arg == "-s")
			configuration.sort = true;
//		else if (arg == "--json" || arg == "-j")
//			json = true;
		else if (arg == "--keep=none")
			configuration.getPivotIndex = RETAIN_NONE;
		else if (arg == "--keep=first")
			configuration.getPivotIndex = RETAIN_FIRST_LOCATION;
		else if (arg == "--keep=last")
			configuration.getPivotIndex = RETAIN_LAST_LOCATION;
		else if (arg == "--keep=max-variance")
			configuration.getPivotIndex = RETAIN_HIGHEST_VARIANCE;
		else if (arg[0] == '-') {
			printf("Unknown option : %s\n", arg.c_str());
			return EXIT_FAILURE;
		} else
			folder = arg;
	}

	list<Item> folders;
	folders.emplace_back(folder);

	while (!folders.empty()) {
		const auto &current = folders.front().filename;

		auto result = parseDir(configuration, current.c_str());

		for (const Item &item : result.directories) {
			const string& filename = item.filename;
			if (recursive && !filename.empty() && filename != "." && filename != "..") {
				const bool lastCurrentCharIsSlash = current.back() == '/';
				if (lastCurrentCharIsSlash)
					folders.emplace_back(current + filename);
				else
					folders.emplace_back(current + '/' + filename);
			}
		}
		if (json)
			printJson(result);
		else
			printRegular(result);

		folders.pop_front();
	}

	return EXIT_SUCCESS;
}
