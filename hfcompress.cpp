#include <algorithm>
#include <bitset>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

struct HFTreeNode {
    std::unique_ptr<HFTreeNode> left = nullptr;
    std::unique_ptr<HFTreeNode> right = nullptr;
    uint8_t val{};
    char c;

    explicit HFTreeNode(const uint8_t val, const char c) {
        this->val = val;
        this->c = c;
    }

    explicit HFTreeNode(const char c) {
        this->c = c;
    }
};

struct CompareNode {
    bool operator()(
        const std::unique_ptr<HFTreeNode> &a,
        const std::unique_ptr<HFTreeNode> &b) const noexcept {
        return a->val > b->val; // smaller 'val' means higher priority
    }
};

void generateCodes(const HFTreeNode *node, const std::string &prefix, std::map<char, std::string> &map) {
    if (!node) { return; }

    // Node is a leaf
    if (!node->left and !node->right) {
        map[node->c] = prefix;
        return;
    }

    generateCodes(node->left.get(), prefix + '0', map);
    generateCodes(node->right.get(), prefix + '1', map);
}

std::string serializeTree(const HFTreeNode *node) {
    std::stringstream bits;
    if (node->left == nullptr && node->right == nullptr) {
        bits << "1";
        bits << std::bitset<8>(node->c);
    } else {
        bits << "0";
        if (node->left != nullptr) bits << serializeTree(node->left.get());
        if (node->right != nullptr) bits << serializeTree(node->right.get());
    }
    return bits.str();
}

std::string encodeString(std::map<char, std::string> &map, const std::string &text, const HFTreeNode *root,
                         uint16_t &treeSize) {
    std::stringstream bits;
    const std::string serialRoot = serializeTree(root);
    treeSize = serialRoot.size();
    bits << serialRoot;
    for (char c: text) {
        bits << map[c];
    }
    return bits.str();
}

std::string decodeString(HFTreeNode *root, const std::string &bits) {
    std::stringstream decoded;

    int const offset = stoi(bits.substr(0, 7));
    const std::string a = bits.substr(8, 16);
    int const treeSize = stoi(a, nullptr, 2);

    std::string sub = bits.substr(24 + treeSize);
    sub = sub.substr(0, sub.size() - offset - 1);
    auto node = root;

    for (char const bit: sub) {
        node = (bit == '0') ? node->left.get() : node->right.get();

        if (!node->left and !node->right) {
            decoded << node->c;
            node = root;
        }
    }
    return decoded.str();
}

std::map<char, int> getFreq(const std::string &text) {
    std::map<char, int> map;
    for (char c: text) {
        if (map.find(c) == map.end()) {
            map.insert(std::make_pair(c, 1));
        } else {
            map[c]++;
        }
    }
    return map;
}

HFTreeNode *createHFTree(const std::map<char, int> &map,
                         std::priority_queue<std::unique_ptr<HFTreeNode>, std::vector<std::unique_ptr<HFTreeNode> >,
                             CompareNode> &pq) {
    for (auto [ch, val]: map) {
        pq.push(std::make_unique<HFTreeNode>(val, ch));
        // std::cout << ch << " " << val << "\n";
    }
    while (pq.size() > 1) {
        auto left = std::move(const_cast<std::unique_ptr<HFTreeNode> &>(pq.top()));
        pq.pop();
        auto right = std::move(const_cast<std::unique_ptr<HFTreeNode> &>(pq.top()));
        pq.pop();

        auto parent = std::make_unique<HFTreeNode>(left->val + right->val, '\0');
        parent->left = std::move(left);
        parent->right = std::move(right);
        pq.push(std::move(parent));
    }
    return pq.top().get();
}

std::vector<uint8_t> convertToBytes(const std::string &string, const uint16_t &treeSize) {
    std::vector<uint8_t> binaryStream;
    const uint8_t bits_to_add = 8 - string.size() % 8;
    binaryStream.reserve(((string.size() + bits_to_add) / 8) + 1);
    binaryStream.push_back(bits_to_add);

    binaryStream.push_back(treeSize >> 8);
    binaryStream.push_back(treeSize);

    uint8_t nextByte = 0;
    int index = 0;

    for (char const c: string) {
        if (index >= 8) {
            index = 0;
            binaryStream.push_back(nextByte);
            nextByte = 0;
        }
        nextByte <<= 1;
        if (c == '1') nextByte |= 1;
        index++;
    }
    for (int i = 0; i < bits_to_add; i++) {
        nextByte <<= 1;
    }
    binaryStream.push_back(nextByte);

    return binaryStream;
}

void writeToFile(const char *path, const std::vector<uint8_t> &binaryStream) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char *>(binaryStream.data()), binaryStream.size());
    out.close();
}

std::vector<uint8_t> readBinaryFile(const std::string &filename) {
    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    if (!in) throw std::runtime_error("Failed to open file:" + filename);

    const std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!in.read(reinterpret_cast<char *>(buffer.data()), size)) {
        throw std::runtime_error("Failed to read file: " + filename);
    }
    return buffer;
}

std::unique_ptr<HFTreeNode> deserializeTree(const std::string &bits, std::size_t &index) {
    if (index >= bits.size())
        throw std::invalid_argument("Unexpected end of bits");

    const char bit = bits[index++];

    if (bit == '1') {
        // Leaf node: read next 8 bits as character
        if (index + 8 > bits.size())
            throw std::invalid_argument("Unexpected end of bits in leaf");

        const std::string byteStr = bits.substr(index, 8);
        index += 8;

        char c = static_cast<char>(std::stoi(byteStr, nullptr, 2));
        return std::make_unique<HFTreeNode>(c);
    }
    if (bit == '0') {
        // Internal node
        auto node = std::make_unique<HFTreeNode>('\0');
        node->left = deserializeTree(bits, index);
        node->right = deserializeTree(bits, index);
        return node;
    }
    throw std::invalid_argument("Invalid bit in stream");
}

void printUsage(const std::string &progName) {
    std::cout << "Usage:\n"
            << "  " << progName << " <input_text_file> [-o <output_file>]\n\n"
            << "Example:\n"
            << "  " << progName << " input.txt -o compressed.bin\n"
            << "Usage:\n"
            << "  " << progName << " --decode <input_bin_file> [-o <output_text_file>]\n\n"
            << "Example:\n"
            << "  " << progName << " --decode input.huff -o test.txt\n";
}

std::string readTextFile(const std::string &input) {
    std::stringstream fullText;
    std::string line;
    std::fstream file(input);

    if (!file.is_open()) {
        std::cerr << "File not found";
        exit(-1);
    }

    while (std::getline(file, line)) {
        // std::cout << "2\n";
        // std::cout << line << "\n";
        fullText << line;
    }
    std::cout << "2\n";

    return fullText.str();
}

void decompress(int argc, char **argv) {
    const std::string INPUT = argv[2];
    std::string OUTPUT;

    // Parse optional arguments
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o") {
            if (i + 1 < argc)
                OUTPUT = argv[++i];
            else {
                std::cerr << "Error: Missing filename after -o\n";
                return;
            }
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            // printUsage(argv[0]);
            return;
        }
    }

    if (OUTPUT.empty()) {
        OUTPUT = INPUT + ".huff";
    }

    const std::vector<uint8_t> decodedStream = readBinaryFile(INPUT);

    std::stringstream bitStreamStr;
    for (uint8_t i: decodedStream) {
        bitStreamStr << std::bitset<8>(i).to_string();
    }

    std::size_t bitIndex = 24;
    const std::unique_ptr<HFTreeNode> deserializedTree = deserializeTree(bitStreamStr.str(), bitIndex);

    std::string const decompressed = decodeString(deserializedTree.get(), bitStreamStr.str());

    std::ofstream out(OUTPUT);
    out << decompressed;
    std::cout << "Decompressed file complete";
}

void compress(int const argc, char **argv) {
    std::string OUTPUT;

    const char* INPUT = argv[1];

    // Parse optional arguments
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o") {
            if (i + 1 < argc)
                OUTPUT = argv[++i];
            else {
                std::cerr << "Error: Missing filename after -o\n";
                return;
            }
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            // printUsage(argv[0]);
            return;
        }
    }

    if (OUTPUT.empty()) {
        OUTPUT = INPUT + ".huff";
    }

    std::cout << "Input file : " << INPUT << '\n';
    std::cout << "Output file: " << OUTPUT << '\n';

    // Get text to be decoded
    const std::string text = readTextFile(INPUT);

    // Create map to get the frequency of each character
    const std::map<char, int> map = getFreq(text);

    // create a priority queue to ensure the tree is built using the smallest values up
    std::priority_queue<std::unique_ptr<HFTreeNode>, std::vector<std::unique_ptr<HFTreeNode> >, CompareNode> pq;

    // Get the root of the Huffman Tree
    const HFTreeNode *root = createHFTree(map, pq);

    // Get a map of the compressed letters and generate the codes
    std::map<char, std::string> compressMap;
    generateCodes(root, "", compressMap);

    uint16_t treeSize = 0;
    const std::vector<uint8_t> binaryStream = convertToBytes(encodeString(compressMap, text, root, treeSize), treeSize);
    writeToFile(OUTPUT.c_str(), binaryStream);
    std::cout << "Compression complete\n";
}

// Driver Function
int main(const int argc, char **argv) {
    if (argc <= 1) {
        printUsage(argv[0]);
        return -1;
    }

    if (std::strcmp(argv[1], "--decode") == 0) {
        decompress(argc, argv);
    } else {
        compress(argc, argv);
    }
}
