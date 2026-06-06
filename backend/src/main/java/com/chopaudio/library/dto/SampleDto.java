package com.chopaudio.library.dto;

import com.chopaudio.library.model.SampleType;

import java.time.Instant;
import java.util.Set;

/**
 * API representation of a {@link com.chopaudio.library.model.Sample}.
 */
public record SampleDto(
        Long id,
        String name,
        String filePath,
        SampleType type,
        String format,
        Long durationMs,
        Long sizeBytes,
        Integer sampleRate,
        Integer channels,
        Integer bpm,
        String musicalKey,
        Set<String> tags,
        boolean favorite,
        int playCount,
        Instant lastPlayedAt,
        String prompt,
        String provider,
        Instant createdAt
) {
}
