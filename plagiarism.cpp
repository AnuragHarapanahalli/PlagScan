/**
 * ============================================================
 *   University Plagiarism Detection System
 *   Uses: K-Gram Rolling Hash + Binary Search Tree (BST)
 * ============================================================
 *
 *  Algorithm Overview:
 *  1. Preprocess text → lowercase, strip punctuation/whitespace
 *  2. Slide a k-gram window over characters → compute rolling hash
 *  3. Insert all hashes from file1 into a BST
 *  4. For each hash from file2, search the BST → count matches
 *  5. Similarity = matches / total_grams_in_file2
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>
#include <sstream>

// ─────────────────────────────────────────────────────────────
//  CONFIGURATION
// ─────────────────────────────────────────────────────────────
const int    K         = 5;           // k-gram size (characters)
const long long BASE   = 31;          // polynomial hash base (prime-ish)
const long long MOD    = 1000000007LL;// large prime modulus to avoid overflow

// ─────────────────────────────────────────────────────────────
//  BST NODE
// ─────────────────────────────────────────────────────────────
struct BSTNode {
    long long value;   // the fingerprint (hash value)
    int       count;   // how many times this fingerprint was inserted
    BSTNode*  left;
    BSTNode*  right;

    BSTNode(long long v)
        : value(v), count(1), left(nullptr), right(nullptr) {}
};

// ─────────────────────────────────────────────────────────────
//  BST: INSERT
//  Standard BST insertion. If the value already exists we
//  increment count so duplicate k-grams are tracked.
//  Time Complexity: O(log n) average, O(n) worst case (skewed tree)
// ─────────────────────────────────────────────────────────────
BSTNode* bstInsert(BSTNode* root, long long value) {
    if (root == nullptr) {
        // Base case: create a fresh node here
        return new BSTNode(value);
    }
    if (value < root->value) {
        // Go left for smaller values
        root->left  = bstInsert(root->left,  value);
    } else if (value > root->value) {
        // Go right for larger values
        root->right = bstInsert(root->right, value);
    } else {
        // Exact match: just increment the occurrence counter
        root->count++;
    }
    return root;
}

// ─────────────────────────────────────────────────────────────
//  BST: SEARCH
//  Returns true if the given value exists in the BST.
//  Time Complexity: O(log n) average, O(n) worst case
// ─────────────────────────────────────────────────────────────
bool bstSearch(BSTNode* root, long long value) {
    if (root == nullptr) return false;       // not found
    if (value == root->value) return true;   // found!
    if (value < root->value)
        return bstSearch(root->left,  value);
    else
        return bstSearch(root->right, value);
}

// ─────────────────────────────────────────────────────────────
//  BST: FREE MEMORY
//  Post-order traversal to delete every node.
// ─────────────────────────────────────────────────────────────
void bstFree(BSTNode* root) {
    if (root == nullptr) return;
    bstFree(root->left);
    bstFree(root->right);
    delete root;
}

// ─────────────────────────────────────────────────────────────
//  PREPROCESSING
//  Strips everything except lowercase letters. Digits are kept
//  because code plagiarism often involves numeric literals.
//  Whitespace and punctuation are removed so spacing changes
//  and minor edits don't defeat detection.
// ─────────────────────────────────────────────────────────────
std::string preprocess(const std::string& raw) {
    std::string clean;
    clean.reserve(raw.size());
    for (char ch : raw) {
        if (std::isalpha((unsigned char)ch)) {
            // Fold to lowercase for case-insensitive comparison
            clean += (char)std::tolower((unsigned char)ch);
        } else if (std::isdigit((unsigned char)ch)) {
            clean += ch;   // keep digits
        }
        // everything else (spaces, punctuation, newlines) → dropped
    }
    return clean;
}

// ─────────────────────────────────────────────────────────────
//  K-GRAM ROLLING HASH  (Polynomial / Rabin-Karp style)
//
//  For a window of k characters c[0]..c[k-1] the hash is:
//    H = c[0]*BASE^(k-1) + c[1]*BASE^(k-2) + ... + c[k-1]  (mod MOD)
//
//  Rolling update when sliding one position right:
//    H_new = (H_old - c[0]*BASE^(k-1)) * BASE + c[k]   (mod MOD)
//
//  This makes building all n-k+1 hashes O(n) instead of O(n*k).
//
//  Returns a vector of all hash fingerprints for the text.
// ─────────────────────────────────────────────────────────────
std::vector<long long> computeKGramHashes(const std::string& text) {
    std::vector<long long> hashes;
    int n = (int)text.size();
    if (n < K) return hashes;   // text too short to form a single k-gram

    // Precompute BASE^(K-1) mod MOD  (the "leading coefficient" to subtract)
    long long highPow = 1;
    for (int i = 0; i < K - 1; i++)
        highPow = (highPow * BASE) % MOD;

    // Compute the first window hash
    long long h = 0;
    for (int i = 0; i < K; i++)
        h = (h * BASE + text[i]) % MOD;
    hashes.push_back(h);

    // Slide the window one character at a time
    for (int i = K; i < n; i++) {
        // Remove the contribution of the character leaving the window
        h = (h - (long long)text[i - K] * highPow % MOD + MOD) % MOD;
        // Shift everything left by one position and add new character
        h = (h * BASE + text[i]) % MOD;
        hashes.push_back(h);
    }
    return hashes;
}

// ─────────────────────────────────────────────────────────────
//  FILE READER
// ─────────────────────────────────────────────────────────────
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Cannot open file: " << filename << "\n";
        return "";
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// ─────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    std::string file1 = (argc > 1) ? argv[1] : "file1.txt";
    std::string file2 = (argc > 2) ? argv[2] : "file2.txt";

    std::cout << "=========================================\n";
    std::cout << "   University Plagiarism Detection System\n";
    std::cout << "=========================================\n\n";

    // ── Step 1: Read & preprocess both files ──────────────────
    std::string raw1 = readFile(file1);
    std::string raw2 = readFile(file2);

    if (raw1.empty() || raw2.empty()) {
        std::cerr << "[ERROR] One or both files could not be read.\n";
        return 1;
    }

    std::string text1 = preprocess(raw1);
    std::string text2 = preprocess(raw2);

    std::cout << "[INFO] File 1 preprocessed length : " << text1.size() << " chars\n";
    std::cout << "[INFO] File 2 preprocessed length : " << text2.size() << " chars\n\n";

    // ── Step 2: Build fingerprints for file1 → insert into BST ─
    std::vector<long long> hashes1 = computeKGramHashes(text1);
    BSTNode* bstRoot = nullptr;

    for (long long h : hashes1)
        bstRoot = bstInsert(bstRoot, h);

    std::cout << "[BST]  Indexed " << hashes1.size()
              << " k-gram fingerprints from \"" << file1 << "\"\n\n";

    // ── Step 3: Compute fingerprints for file2 → search BST ────
    std::vector<long long> hashes2 = computeKGramHashes(text2);

    int matches    = 0;
    int totalGrams = (int)hashes2.size();

    for (long long h : hashes2) {
        if (bstSearch(bstRoot, h))
            matches++;
    }

    // ── Step 4: Report ─────────────────────────────────────────
    double similarity = (totalGrams > 0)
                        ? (100.0 * matches / totalGrams)
                        : 0.0;

    std::cout << "---------------- RESULTS ---------------\n";
    std::cout << "  K-gram size (k)      : " << K        << "\n";
    std::cout << "  Grams in file1       : " << hashes1.size() << "\n";
    std::cout << "  Grams in file2       : " << totalGrams     << "\n";
    std::cout << "  Matches found        : " << matches        << "\n";
    std::cout << "  Similarity Score     : " << similarity << " %\n";
    std::cout << "----------------------------------------\n";

    if (similarity >= 70.0)
        std::cout << "  [VERDICT] HIGH similarity - likely plagiarism\n";
    else if (similarity >= 40.0)
        std::cout << "  [VERDICT] MODERATE similarity - review recommended\n";
    else
        std::cout << "  [VERDICT] LOW similarity - probably original\n";

    std::cout << "==========================================\n";

    // Clean up BST
    bstFree(bstRoot);
    return 0;
}
