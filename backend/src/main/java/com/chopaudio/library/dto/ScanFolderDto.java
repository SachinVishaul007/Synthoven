package com.chopaudio.library.dto;

import java.time.Instant;

/**
 * API representation of a watched {@link com.chopaudio.library.model.ScanFolder}.
 */
public record ScanFolderDto(
        Long id,
        String path,
        boolean enabled,
        Instant lastScannedAt,
        int sampleCount,
        Instant createdAt
) {
}
