#include "JsonWriter.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include <vector>
#include <string>

#include <sequence/Parser.hpp>
#include <sequence/ItemIO.hpp>
#include <sequence/details/StringUtils.hpp>

namespace sequence {

void printRegular(const Item &item) {
  const auto pFilename = item.filename.c_str();
  switch (item.getType()) {
  case Item::SINGLE:
    printf("%s\n", pFilename);
    break;
  case Item::INVALID:
    printf("Invalid\n");
    break;
  case Item::INDICED:
    printf("%s (%zu)\n", pFilename, item.indices.size());
    break;
  case Item::PACKED:
    if (item.step == 1)
      printf("%s [%d:%d]\n", pFilename, item.start, item.end);
    else
      printf("%s [%d:%d]/%d\n", pFilename, item.start, item.end, item.step);
    break;
  }
}

void printRegular(const FolderContent &result) {

  const std::string& folder = result.name;
  const size_t size = folder.find_last_not_of('/');
  for (const Item &item : result.files) {
    printf("%.*s/", (int)size, folder.c_str());
    printRegular(item);
  }
}

std::string toJson(const Item &item) {
  json::JsonObjectStreamWriter writer;
  const auto type = item.getType();
  writer << std::make_pair("type", getTypeString(type));
  if (type != Item::INVALID) {
    writer << std::make_pair("filename", item.filename.c_str());
    if (type != Item::SINGLE)
      writer << std::make_pair("padding", (int)item.padding);
    switch (type) {
    case Item::INDICED: {
      json::JsonArrayStreamWriter indices;
      for (const auto index : item.indices)
        indices << index;
      writer << std::make_pair("indices", indices.build());
      break;
    }
    case Item::PACKED:
      writer << std::make_pair("start", item.start);
      writer << std::make_pair("end", item.end);
      writer << std::make_pair("step", (int)item.step);
      break;
    default:
      break;
    }
  }
  return writer.build();
}

void printJson(const FolderContent &result) {
  json::JsonObjectStreamWriter writer;
  writer << std::make_pair("path", result.name.c_str());
  { // directories
    json::JsonArrayStreamWriter directories;
    for (const Item &item : result.directories)
      directories << item.filename.c_str();
    writer << std::make_pair("directories", directories.build());
  }
  { // files
    json::JsonArrayStreamWriter files;
    for (const Item &item : result.files)
      files << toJson(item);
    writer << std::make_pair("items", files.build());
  }
  printf("%s\n", writer.build().c_str());
}

} // namespace sequence

static void printHelp() {
  puts(R"(USAGE [OPTIONS] [FOLDER]
--help,-h            Print help and exit.
--merge-padding,-m   Merge sequence with different padding.
--recursive,-r       Parse folder recursively.
--pack,-p            Drop indices and replace with one or several contiguous
                     chunks.
--bake-singleton,-b  Replace Items with only one index by it's corresponding
                     filename.
--sort,-s            Print folder and files lexicographically sorted.
--json,-j            Output result as a json object.
--keep=              Strategy to handle ambiguous locations.
       none          flattens the set.
       first         keep first number.
       last          keep last number.
       max-variance  keep number with highest variance (default). Backups to
                     'none' if same variance.
)");
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
    else if (arg == "--json" || arg == "-j")
      json = true;
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

  vector<string> folders;
  folders.emplace_back(folder);

  while (!folders.empty()) {
    const auto current = move(folders.back());
    folders.pop_back();

    auto result = parseDir(configuration, current);

    for (const Item &item : result.directories) {
      const string &filename = item.filename;
      if (recursive && !filename.empty() && filename != "." &&
          filename != "..") {
        const bool lastCurrentCharIsSlash = current.back() == '/';
        if (lastCurrentCharIsSlash)
          folders.push_back(concat(current, filename));
        else
          folders.push_back(concat(current , "/", filename));
      }
    }
    if (json) {
      printJson(result);
    } else {
      printRegular(result);
    }
  }

  return EXIT_SUCCESS;
}
