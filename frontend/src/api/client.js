// Thin wrapper around the Chop backend REST API.
//
// In dev, requests go to a relative /api path that Vite proxies to the backend
// (see vite.config.js), so the browser stays same-origin. To point at a backend
// on another host, set VITE_API_BASE (e.g. "https://chop.example.com").
const BASE = import.meta.env.VITE_API_BASE ?? '';
const isJuce = typeof window !== 'undefined' && window.__JUCE__ !== undefined;

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
  async search(query, generateMissing = true) {
    if (isJuce) {
      try {
        const results = await window.__JUCE__.backend.search(query);
        return { libraryResults: results, generationJobId: null, status: 'complete' };
      } catch (err) {
        console.error('JUCE search error:', err);
        return { libraryResults: [], generationJobId: null, status: 'failed' };
      }
    }
    return request('/api/search', {
      method: 'POST',
      body: JSON.stringify({ query, generateMissing }),
    });
  },

  /** Library-only search (no generation). */
  async quickSearch(query) {
    if (isJuce) {
      try {
        return await window.__JUCE__.backend.search(query);
      } catch (err) {
        console.error('JUCE quickSearch error:', err);
        return [];
      }
    }
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
  async getAllSamples() {
    if (isJuce) {
      try {
        return await window.__JUCE__.backend.getLibrarySamples();
      } catch (err) {
        return [];
      }
    }
    return request('/api/library/samples');
  },

  async getLibrarySamples() {
    if (isJuce) {
      try {
        return await window.__JUCE__.backend.getLibrarySamples();
      } catch (err) {
        return [];
      }
    }
    return request('/api/library/samples/library');
  },

  async getGeneratedSamples() {
    return [];
  },

  async getFavorites() {
    return [];
  },

  async getRecent() {
    return [];
  },

  async getStats() {
    if (isJuce) {
      try {
        const status = await window.__JUCE__.backend.getLibraryStatus();
        return {
          totalSamples: status.fileCount || 0,
          totalDuration: 0,
        };
      } catch (err) {
        return { totalSamples: 0, totalDuration: 0 };
      }
    }
    return request('/api/library/stats');
  },

  /** Upload audio File objects (e.g. from a folder picker) for import. */
  async uploadFiles(files) {
    if (isJuce) {
      return [];
    }
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

  toggleFavorite(id) {
    if (isJuce) return Promise.resolve(null);
    return request(`/api/library/samples/${id}/favorite`, { method: 'POST' });
  },

  recordPlay(id) {
    if (isJuce) return Promise.resolve(null);
    return request(`/api/library/samples/${id}/play`, { method: 'POST' });
  },

  deleteSample(id) {
    if (isJuce) return Promise.resolve(null);
    return request(`/api/library/samples/${id}`, { method: 'DELETE' });
  },

  // ─── Player ──────────────────────────────────────────────────────────────
  /** Direct URL an <audio> element can stream from. */
  streamUrl(id) {
    return `${BASE}/api/player/stream/${id}`;
  },
  getPlayUrl(id) {
    if (isJuce) return Promise.resolve(null);
    return request(`/api/player/url/${id}`);
  },
};
