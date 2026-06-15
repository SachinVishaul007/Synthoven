package com.chopaudio.library.dto;

import jakarta.validation.constraints.NotBlank;

/**
 * Request body for POST /api/library/folders.
 */
public record AddFolderRequest(
        @NotBlank(message = "path is required")
        String path
) {
}
