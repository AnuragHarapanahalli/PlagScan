# PlagScan — University Plagiarism Detection System

## Quick Start

### 1. Compile the C++ binary (required once)
```bash
g++ -O2 -std=c++17 -o plagiarism plagiarism.cpp
```

### 2. Run CLI only
```bash
./plagiarism file1.txt file2.txt
```

### 3. Run the Node.js UI
```bash
npm install
node server.js
# Open http://localhost:3000
```

## Project Structure
```
plagiarism_detector/
├── plagiarism.cpp     # Core C++ detection engine
├── server.js          # Node.js Express backend
├── package.json       # Node dependencies
├── public/
│   └── index.html     # Frontend UI
├── file1.txt          # Sample reference document
└── file2.txt          # Sample suspect document
```

## Algorithm
1. **Preprocess** — lowercase + strip punctuation/whitespace
2. **K-Gram Rolling Hash** — polynomial hash with k=5 window, O(n) total
3. **BST Index** — insert all file1 fingerprints, O(n log n) avg
4. **BST Search** — look up each file2 hash, O(n log n) avg
5. **Score** — `matches / total_grams × 100%`

## Time Complexity
| Operation | Average | Worst |
|-----------|---------|-------|
| Preprocessing | O(n) | O(n) |
| K-Gram Hashing | O(n) | O(n) |
| BST Build | O(n log n) | O(n²) |
| BST Search (all) | O(n log n) | O(n²) |
| **Total** | **O(n log n)** | **O(n²)** |
