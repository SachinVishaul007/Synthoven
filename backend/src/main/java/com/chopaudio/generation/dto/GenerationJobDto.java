package com.chopaudio.generation.dto;

import com.chopaudio.library.dto.SampleDto;

import java.time.Instant;
import java.util.List;

/**
 * Status of a generation job, returned by POST /api/generation and polled via
 * GET /api/generation/{jobId}.
 *
 * <p>{@code status} is one of: {@code queued}, {@code generating},
 * {@code complete}, {@code failed}.
 */
public record GenerationJobDto(
        String jobId,
        String status,
        String prompt,
        String provider,
        List<SampleDto> samples,
        String error,
        Instant createdAt,
        Instant completedAt
) {
}
