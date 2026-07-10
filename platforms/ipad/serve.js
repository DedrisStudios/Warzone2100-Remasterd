// Desktop test server for the WZ web build: serves the Emscripten output with
// the cross-origin isolation headers (COOP/COEP) needed for SharedArrayBuffer /
// WASM threads, plus correct MIME types. Handy for verifying the build in a
// desktop browser before wrapping it for iPad.
//
//   node serve.js [web-install-dir] [port]
//
// Note: browsers cache aggressively via the service worker. To see edits, open
// devtools > Application > Service Workers > Unregister, and clear cache storage.
const http = require('http');
const fs = require('fs');
const path = require('path');

const ROOT = process.argv[2] || process.cwd();
const PORT = parseInt(process.argv[3] || '8770', 10);

const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.wasm': 'application/wasm',
  '.data': 'application/octet-stream',
  '.json': 'application/json',
  '.png': 'image/png',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.css': 'text/css',
};

http.createServer((req, res) => {
  let urlPath = decodeURIComponent(req.url.split('?')[0]);
  if (urlPath === '/') urlPath = '/index.html';
  const filePath = path.join(ROOT, urlPath);
  res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
  res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
  res.setHeader('Cross-Origin-Resource-Policy', 'same-origin');
  fs.readFile(filePath, (err, data) => {
    if (err) { res.statusCode = 404; res.end('Not found: ' + urlPath); return; }
    res.setHeader('Content-Type', MIME[path.extname(filePath).toLowerCase()] || 'application/octet-stream');
    res.statusCode = 200;
    res.end(data);
  });
}).listen(PORT, () => console.log('WZ web server: http://localhost:' + PORT + '  (root: ' + ROOT + ')'));
