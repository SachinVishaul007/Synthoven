package com.chopaudio.search.dto;

import jakarta.validation.constraints.NotBlank;

/**
 * Request body for POST /api/search.
 *
 * @param query           free-text description, e.g. "punchy techno kick"
 * @param generateMissing when true (default), also kicks off a generation job
 */
public record SearchRequest(
        @NotBlank(message = "query is required")
        String query,
        Boolean generateMissing
) {
    public boolean shouldGenerate() {
        return generateMissing == null || generateMissing;
    }
}
