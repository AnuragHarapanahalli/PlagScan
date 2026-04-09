/**
 * Plagiarism Detector — Node.js Server
 * Fixed: correct binary path for plagiarism/plagiarism.exe subfolder
 */

const express      = require('express');
const multer       = require('multer');
const path         = require('path');
const fs           = require('fs');
const { execFile } = require('child_process');
const os           = require('os');

const app  = express();
const PORT = 3000;

// Always resolve paths relative to THIS file, not the cwd
const __root = __dirname;

// Binary lives inside the "plagiarism" subfolder
const BINARY = path.join(
  __root,
  'plagiarism',
  process.platform === 'win32' ? 'plagiarism.exe' : 'plagiarism'
);

// Multer — store uploads in system temp dir
const upload = multer({ dest: os.tmpdir() });

app.use(express.json());
app.use(express.static(path.join(__root, 'public')));

// ─── GET /health — quick debug endpoint ───────────────────────
app.get('/health', (req, res) => {
  const binaryExists = fs.existsSync(BINARY);
  res.json({
    status:       binaryExists ? 'ok' : 'error',
    binaryPath:   BINARY,
    binaryExists,
    platform:     process.platform,
    nodeVersion:  process.version,
    cwd:          process.cwd(),
    __root,
  });
});

// ─── POST /analyze ────────────────────────────────────────────
app.post('/analyze', upload.fields([
  { name: 'file1', maxCount: 1 },
  { name: 'file2', maxCount: 1 },
]), (req, res) => {

  if (!req.files || !req.files.file1 || !req.files.file2) {
    return res.status(400).json({ error: 'Both files are required.' });
  }

  if (!fs.existsSync(BINARY)) {
    return res.status(500).json({
      error: `C++ binary not found at: ${BINARY}\n\nPlease compile it first:\n  g++ -O2 -std=c++17 -o plagiarism plagiarism.cpp`
    });
  }

  const f1 = req.files.file1[0].path;
  const f2 = req.files.file2[0].path;

  console.log(`[analyze] binary : ${BINARY}`);
  console.log(`[analyze] file1  : ${f1}`);
  console.log(`[analyze] file2  : ${f2}`);

  const startTime = Date.now();

  execFile(BINARY, [f1, f2], { timeout: 15000 }, (err, stdout, stderr) => {
    const elapsed = Date.now() - startTime;

    try { fs.unlinkSync(f1); } catch (_) {}
    try { fs.unlinkSync(f2); } catch (_) {}

    if (err) {
      console.error('[analyze] execFile error:', err.message);
      console.error('[analyze] stderr:', stderr);
      return res.status(500).json({
        error: `Binary execution failed: ${stderr || err.message}`
      });
    }

    console.log('[analyze] stdout:\n', stdout);

    const result = parseOutput(stdout, elapsed);
    console.log('[analyze] parsed result:', JSON.stringify(result, null, 2));
    res.json(result);
  });
});

// ─── Parse C++ stdout into structured JSON ────────────────────
function parseOutput(raw, elapsed) {
  const lines = raw.split('\n');

  // Extract the first number (integer or decimal) after the last colon on a matching line
  const findNumber = (prefix) => {
    const line = lines.find(l => l.includes(prefix));
    if (!line) return null;
    const idx = line.lastIndexOf(':');
    if (idx === -1) return null;
    const after = line.slice(idx + 1).trim();
    const match = after.match(/[\d]+(?:\.[\d]+)?/);
    return match ? match[0] : null;
  };

  const similarity = parseFloat(findNumber('Similarity Score') || '0');
  const matches    = parseInt(findNumber('Matches found')      || '0', 10);
  const gramsFile1 = parseInt(findNumber('Grams in file1')     || '0', 10);
  const gramsFile2 = parseInt(findNumber('Grams in file2')     || '0', 10);
  const kSize      = parseInt(findNumber('K-gram size')        || '5', 10);
  const file1Len   = parseInt(findNumber('File 1 preprocessed')|| '0', 10);
  const file2Len   = parseInt(findNumber('File 2 preprocessed')|| '0', 10);

  let verdict = 'LOW';
  if      (similarity >= 70) verdict = 'HIGH';
  else if (similarity >= 40) verdict = 'MODERATE';

  return { similarity, matches, gramsFile1, gramsFile2, kSize,
           file1Len, file2Len, verdict, elapsedMs: elapsed, raw };
}

// ─── Start ────────────────────────────────────────────────────
app.listen(PORT, () => {
  console.log(`\n🔍 PlagScan UI  →  http://localhost:${PORT}`);
  console.log(`🩺 Health check →  http://localhost:${PORT}/health`);
  console.log(`📦 Binary path  →  ${BINARY}`);
  console.log(`   Exists: ${fs.existsSync(BINARY) ? '✅ yes' : '❌ NO — compile first!'}\n`);
});
