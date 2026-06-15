package com.chopaudio.library.model;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.PrePersist;
import jakarta.persistence.Table;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;

import java.time.Instant;

/**
 * A folder Chop watches and scans for audio samples.
 */
@Entity
@Table(name = "scan_folders")
@Getter
@Setter
@NoArgsConstructor
@AllArgsConstructor
@Builder
public class ScanFolder {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(nullable = false, unique = true, length = 1024)
    private String path;

    @Column(nullable = false)
    @Builder.Default
    private boolean enabled = true;

    private Instant lastScannedAt;

    @Column(nullable = false)
    @Builder.Default
    private int sampleCount = 0;

    @Column(nullable = false, updatable = false)
    private Instant createdAt;

    @PrePersist
    void onCreate() {
        if (createdAt == null) {
            createdAt = Instant.now();
        }
    }
}
