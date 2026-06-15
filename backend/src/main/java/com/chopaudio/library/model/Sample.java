package com.chopaudio.library.model;

import jakarta.persistence.Column;
import jakarta.persistence.ElementCollection;
import jakarta.persistence.Entity;
import jakarta.persistence.EnumType;
import jakarta.persistence.Enumerated;
import jakarta.persistence.FetchType;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.Index;
import jakarta.persistence.Table;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;

import java.time.Instant;
import java.util.HashSet;
import java.util.Set;

@Entity
@Table(
        name = "samples",
        indexes = {
                @Index(name = "idx_sample_type", columnList = "type"),
                @Index(name = "idx_sample_favorite", columnList = "favorite")
        }
)
@Getter
@Setter
@NoArgsConstructor
@AllArgsConstructor
@Builder
public class Sample {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(nullable = false)
    private String name;

    /** Absolute path on disk; unique so re-scans don't create duplicates. */
    @Column(nullable = false, unique = true, length = 1024)
    private String filePath;

    @Enumerated(EnumType.STRING)
    @Column(nullable = false)
    private SampleType type;

    /** File extension without the dot: wav, mp3, aiff, flac. */
    private String format;

    private Long durationMs;
    private Long sizeBytes;
    private Integer sampleRate;
    private Integer channels;

    /** Musical metadata — best-effort, may be null. */
    private Integer bpm;
    private String musicalKey;

    @ElementCollection(fetch = FetchType.EAGER)
    @Builder.Default
    private Set<String> tags = new HashSet<>();

    @Column(nullable = false)
    @Builder.Default
    private boolean favorite = false;

    @Column(nullable = false)
    @Builder.Default
    private int playCount = 0;

    private Instant lastPlayedAt;

    /** Prompt used to generate this sample (null for LIBRARY samples). */
    @Column(length = 2048)
    private String prompt;

    /** Provider that generated this sample, e.g. "mock" or "suno". */
    private String provider;

    @Column(nullable = false, updatable = false)
    private Instant createdAt;

    @jakarta.persistence.PrePersist
    void onCreate() {
        if (createdAt == null) {
            createdAt = Instant.now();
        }
    }
}
