package com.chopaudio.generation.dto;

import jakarta.validation.constraints.NotBlank;

/**
 * Request body for POST /api/generation.
 *
 * @param prompt natural-language description of the sample to generate
 * @param count  number of variations to produce (defaults applied in service)
 */
public record GenerationRequest(
        @NotBlank(message = "prompt is required")
        String prompt,
        Integer count
) {
}
