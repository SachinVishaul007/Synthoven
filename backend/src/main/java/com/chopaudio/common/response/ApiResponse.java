package com.chopaudio.common.response;

import lombok.AllArgsConstructor;
import lombok.Getter;

import java.time.Instant;

/**
 * Generic envelope for simple action/status endpoints (favorite toggle, delete,
 * stats, error responses). Data-heavy endpoints return their DTOs directly.
 */
@Getter
@AllArgsConstructor
public class ApiResponse<T> {

    private final boolean success;
    private final String message;
    private final T data;
    private final Instant timestamp;

    public static <T> ApiResponse<T> ok(T data) {
        return new ApiResponse<>(true, null, data, Instant.now());
    }

    public static <T> ApiResponse<T> ok(String message, T data) {
        return new ApiResponse<>(true, message, data, Instant.now());
    }

    public static ApiResponse<Void> message(String message) {
        return new ApiResponse<>(true, message, null, Instant.now());
    }

    public static ApiResponse<Void> error(String message) {
        return new ApiResponse<>(false, message, null, Instant.now());
    }
}
