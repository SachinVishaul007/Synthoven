// Thin wrapper around the Chop backend REST API.
//
// In dev, requests go to a relative /api path that Vite proxies to the backend
// (see vite.config.js), so the browser stays same-origin. To point at a backend
// on another host, set VITE_API_BASE (e.g. "https://chop.example.com").
const BASE = import.meta.env.VITE_API_BASE ?? '';

async function request(path, options = {}) {
  const res = await fetch(`${BASE}${path}`, {
    headers: { 'Content-Type': 'application/json', ...(options.headers || {}) },
    ...options,
  });

  if (!res.ok) {
    let detail = '';
    try { detail = await res.text(); } catch { /* ignore */ }
    throw new Error(`${res.status} ${res.statusText}${detail ? ` — ${detail}` : ''}`);
  }

  if (res.status === 204) return null;
  return res.json();
}

export const api = {
  // ─── Search ──────────────────────────────────────────────────────────────
  /** Library hits immediately + a generationJobId to poll. */
  search(query, generateMissing = true) {
    return request('/api/search', {
      method: 'POST',
      body: JSON.stringify({ query, generateMissing }),
    });
  },

  /** Library-only search (no generation). */
  quickSearch(query) {
    return request(`/api/search?q=${encodeURIComponent(query)}`);
  },

  // ─── Generation ──────────────────────────────────────────────────────────
  startGeneration(prompt, count) {
    return request('/api/generation', {
      method: 'POST',
      body: JSON.stringify({ prompt, count }),
    });
  },

  getGenerationJob(jobId) {
    return request(`/api/generation/${encodeURIComponent(jobId)}`);
  },

  // ─── Library ─────────────────────────────────────────────────────────────
  getAllSamples()       { return request('/api/library/samples'); },
  getLibrarySamples()   { return request('/api/library/samples/library'); },
  getGeneratedSamples() { return request('/api/library/samples/generated'); },
  getFavorites()        { return request('/api/library/samples/favorites'); },
  getRecent()           { return request('/api/library/samples/recent'); },
  getStats()            { return request('/api/library/stats'); },

  /** Upload audio File objects (e.g. from a folder picker) for import. */
  async uploadFiles(files) {
    const form = new FormData();
    for (const f of files) form.append('files', f, f.name);
    // No Content-Type header — the browser sets the multipart boundary.
    const res = await fetch(`${BASE}/api/library/upload`, { method: 'POST', body: form });
    if (!res.ok) {
      let detail = '';
      try { detail = await res.text(); } catch { /* ignore */ }
      throw new Error(`${res.status} ${res.statusText}${detail ? ` — ${detail}` : ''}`);
    }
    return res.json();
  },

  toggleFavorite(id) { return request(`/api/library/samples/${id}/favorite`, { method: 'POST' }); },
  recordPlay(id)     { return request(`/api/library/samples/${id}/play`, { method: 'POST' }); },
  deleteSample(id)   { return request(`/api/library/samples/${id}`, { method: 'DELETE' }); },

  // ─── Player ──────────────────────────────────────────────────────────────
  /** Direct URL an <audio> element can stream from. */
  streamUrl(id) { return `${BASE}/api/player/stream/${id}`; },
  getPlayUrl(id) { return request(`/api/player/url/${id}`); },
};
