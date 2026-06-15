package com.chopaudio.library.service;

import com.chopaudio.library.model.Sample;
import com.chopaudio.library.model.SampleType;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.Set;

/**
 * Extracts metadata from audio files. Format and size come from the filesystem;
 * WAV files additionally get sample rate / channels / duration parsed from the
 * RIFF header. Other formats leave those fields null (best-effort, no external
 * audio libraries required).
 */
@Service
@Slf4j
public class AudioMetadataService {

    /**
     * Build a (not yet persisted) LIBRARY Sample from a file on disk.
     */
    public Sample fromFile(Path path) {
        String fileName = path.getFileName().toString();
        String format = extension(fileName);

        Sample.SampleBuilder builder = Sample.builder()
                .name(stripExtension(fileName))
                .filePath(path.toAbsolutePath().toString())
                .type(SampleType.LIBRARY)
                .format(format)
                .tags(deriveTags(fileName));

        try {
            builder.sizeBytes(Files.size(path));
        } catch (IOException e) {
            log.warn("Could not read size of {}: {}", path, e.getMessage());
        }

        if ("wav".equalsIgnoreCase(format)) {
            readWavHeader(path, builder);
        }

        return builder.build();
    }

    /**
     * Parse the RIFF/WAVE header for sample rate, channels and duration.
     * Silently skips on any read error — metadata is non-essential.
     */
    private void readWavHeader(Path path, Sample.SampleBuilder builder) {
        try (RandomAccessFile raf = new RandomAccessFile(path.toFile(), "r")) {
            byte[] header = new byte[44];
            int read = raf.read(header);
            if (read < 44) {
                return;
            }
            ByteBuffer bb = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN);

            // "RIFF" .... "WAVE" "fmt "
            if (header[0] != 'R' || header[1] != 'I' || header[2] != 'F' || header[3] != 'F') {
                return;
            }

            int channels = bb.getShort(22) & 0xFFFF;
            long sampleRate = bb.getInt(24) & 0xFFFFFFFFL;
            long byteRate = bb.getInt(28) & 0xFFFFFFFFL;
            long dataSize = (raf.length() - 44);

            builder.channels(channels);
            builder.sampleRate((int) sampleRate);
            if (byteRate > 0) {
                builder.durationMs((dataSize * 1000L) / byteRate);
            }
        } catch (IOException e) {
            log.debug("Could not parse WAV header for {}: {}", path, e.getMessage());
        }
    }

    /**
     * Rough tags derived from path/filename tokens so freshly scanned samples
     * are searchable before any manual tagging.
     */
    private Set<String> deriveTags(String fileName) {
        Set<String> tags = new HashSet<>();
        String lower = stripExtension(fileName).toLowerCase();
        for (String token : lower.split("[\\s_\\-.]+")) {
            if (token.length() >= 3) {
                tags.add(token);
            }
        }
        return tags;
    }

    private String extension(String fileName) {
        int dot = fileName.lastIndexOf('.');
        return dot >= 0 && dot < fileName.length() - 1
                ? fileName.substring(dot + 1).toLowerCase()
                : "";
    }

    private String stripExtension(String fileName) {
        int dot = fileName.lastIndexOf('.');
        return dot > 0 ? fileName.substring(0, dot) : fileName;
    }
}
