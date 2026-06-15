package com.chopaudio.library.service;

import com.chopaudio.library.dto.SampleDto;
import com.chopaudio.library.dto.ScanFolderDto;
import com.chopaudio.library.model.Sample;
import com.chopaudio.library.model.ScanFolder;
import org.springframework.stereotype.Component;

import java.util.HashSet;
import java.util.List;

/**
 * Converts JPA entities to their API-facing DTOs.
 */
@Component
public class SampleMapper {

    public SampleDto toDto(Sample s) {
        return new SampleDto(
                s.getId(),
                s.getName(),
                s.getFilePath(),
                s.getType(),
                s.getFormat(),
                s.getDurationMs(),
                s.getSizeBytes(),
                s.getSampleRate(),
                s.getChannels(),
                s.getBpm(),
                s.getMusicalKey(),
                s.getTags() == null ? new HashSet<>() : new HashSet<>(s.getTags()),
                s.isFavorite(),
                s.getPlayCount(),
                s.getLastPlayedAt(),
                s.getPrompt(),
                s.getProvider(),
                s.getCreatedAt()
        );
    }

    public List<SampleDto> toDtoList(List<Sample> samples) {
        return samples.stream().map(this::toDto).toList();
    }

    public ScanFolderDto toDto(ScanFolder f) {
        return new ScanFolderDto(
                f.getId(),
                f.getPath(),
                f.isEnabled(),
                f.getLastScannedAt(),
                f.getSampleCount(),
                f.getCreatedAt()
        );
    }

    public List<ScanFolderDto> toFolderDtoList(List<ScanFolder> folders) {
        return folders.stream().map(this::toDto).toList();
    }
}
