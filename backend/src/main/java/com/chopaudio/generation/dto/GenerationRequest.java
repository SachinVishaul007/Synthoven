package com.chopaudio.generation.dto;

import jakarta.validation.constraints.NotBlank;

/**
 * Request body for POST /api/generation.
 *
 * @param prompt          natural-language description of the sample to generate
 * @param count           number of variations to produce (defaults applied in service)
 * @param durationSeconds desired length in seconds (provider default when null)
 * @param cfgScale        prompt-adherence / "creativity" control (provider default when null)
 * @param category        optional category hint prepended to the prompt (e.g. "drum one shot")
 */
public record GenerationRequest(
        @NotBlank(message = "prompt is required")
        String prompt,
        Integer count,
        Double durationSeconds,
        Double cfgScale,
        String category
) {
}
