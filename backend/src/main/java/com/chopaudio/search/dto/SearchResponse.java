package com.chopaudio.search.dto;

import com.chopaudio.library.dto.SampleDto;

import java.util.List;

/**
 * Response for POST /api/search. Library hits return immediately; if a
 * generation job was started, {@code generationJobId} can be polled via
 * GET /api/generation/{jobId}.
 *
 * @param status one of {@code searching} (generation in flight) or {@code done}
 */
public record SearchResponse(
        String query,
        List<SampleDto> libraryResults,
        String generationJobId,
        String status
) {
}
