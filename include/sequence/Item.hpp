#ifndef ITEM_HPP_
#define ITEM_HPP_

#include <utility>
#include <vector>
#include <string>

namespace sequence {

typedef unsigned int index_type;
typedef std::vector<index_type> VALUES;

typedef std::string STRING;
typedef typename STRING::value_type CHAR;

/**
 * An Item represents a sequence entry.
 * It can be of the following types :
 * - INVALID : the item is not in a valid state
 * - SINGLE  : it's either a single file or directory
 * - INDICED : it's a sequence with a set of numbers attached to it
 * - PACKED  : it's a contiguous sequence going from start to end
 */
struct Item {
	enum
		: CHAR {PADDING_CHAR = '#'
	};
    enum Type {
        INVALID, SINGLE, INDICED, PACKED
    };
    STRING filename;
    VALUES indices;
    index_type start, end;
    char padding, step;
    Item() :
                    start(-1), end(-1), padding(-1), step(-1) {
    }
    Item(const STRING& filename) :
                    filename(filename), start(-1), end(-1), padding(-1), step(-1) {
    }
    Item(STRING&& filename) :filename(std::move(filename)),
    start(-1), end(-1), padding(-1), step(-1) {
    }
    Item(STRING&& filename, VALUES&& values) : filename(std::move(filename)), indices(std::move(values)),
    start(-1), end(-1), padding(-1), step(-1) {
    }
    Type getType() const {
        if (filename.empty()) {
            return INVALID;
        }
        if (!indices.empty()) {
            return INDICED;
        }
        if (step == -1) {
            return SINGLE;
        }
        return PACKED;
    }
};

/**
 * For convenience, Items is defined to be a vector of Item
 */
typedef std::vector<Item> Items;

}

#endif /* ITEM_HPP_ */
