// Screenshot einer gerenderten Seite in TRMNL-Größe, danach Umwandlung in 1 Bit (wie E-Ink).
const { chromium } = require('playwright');
const fs = require('fs');
const [,, inFile, outFile, w, h] = process.argv;
(async () => {
  const body = fs.readFileSync(inFile, 'utf8');
  const html = `<!doctype html><html><head><meta charset="utf-8"><style>html,body{margin:0;background:#fff}
  .view{position:relative;width:${w}px;height:${h}px;overflow:hidden}</style></head><body>${body}</body></html>`;
  const b = await chromium.launch();
  const p = await b.newPage({ viewport: { width: +w, height: +h } });
  await p.setContent(html, { waitUntil: 'networkidle' });
  await p.evaluate(() => document.fonts.ready);
  await p.screenshot({ path: outFile });
  await b.close();
})();
