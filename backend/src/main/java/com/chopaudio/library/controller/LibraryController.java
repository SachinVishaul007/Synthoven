package com.chopaudio.library.controller;

import com.chopaudio.common.response.ApiResponse;
import com.chopaudio.library.dto.AddFolderRequest;
import com.chopaudio.library.dto.SampleDto;
import com.chopaudio.library.dto.ScanFolderDto;
import com.chopaudio.library.model.SampleType;
import com.chopaudio.library.service.LibraryService;
import jakarta.validation.Valid;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.multipart.MultipartFile;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api/library")
public class LibraryController {

    private final LibraryService libraryService;

    public LibraryController(LibraryService libraryService) {
        this.libraryService = libraryService;
    }

    // ─── Samples ─────────────────────────────────────────────────────────────

    @GetMapping("/samples")
    public List<SampleDto> getAllSamples() {
        return libraryService.getAllSamples();
    }

    @GetMapping("/samples/library")
    public List<SampleDto> getLibrarySamples() {
        return libraryService.getByType(SampleType.LIBRARY);
    }

    @GetMapping("/samples/generated")
    public List<SampleDto> getGeneratedSamples() {
        return libraryService.getByType(SampleType.GENERATED);
    }

    @GetMapping("/samples/favorites")
    public List<SampleDto> getFavorites() {
        return libraryService.getFavorites();
    }

    @GetMapping("/samples/recent")
    public List<SampleDto> getRecent() {
        return libraryService.getRecent();
    }

    @GetMapping("/samples/{id}")
    public SampleDto getSample(@PathVariable Long id) {
        return libraryService.getById(id);
    }

    @PostMapping("/samples/{id}/favorite")
    public SampleDto toggleFavorite(@PathVariable Long id) {
        return libraryService.toggleFavorite(id);
    }

    @PostMapping("/samples/{id}/play")
    public SampleDto recordPlay(@PathVariable Long id) {
        return libraryService.recordPlay(id);
    }

    @DeleteMapping("/samples/{id}")
    public ApiResponse<Void> deleteSample(@PathVariable Long id) {
        libraryService.deleteSample(id);
        return ApiResponse.message("Sample deleted");
    }

    /** Upload audio files (e.g. a whole folder) and import them as library samples. */
    @PostMapping(value = "/upload", consumes = MediaType.MULTIPART_FORM_DATA_VALUE)
    public ResponseEntity<List<SampleDto>> upload(@RequestParam("files") MultipartFile[] files) {
        return ResponseEntity.status(HttpStatus.CREATED).body(libraryService.importUploads(files));
    }

    // ─── Folders ─────────────────────────────────────────────────────────────

    @GetMapping("/folders")
    public List<ScanFolderDto> getFolders() {
        return libraryService.getFolders();
    }

    @PostMapping("/folders")
    public ResponseEntity<ScanFolderDto> addFolder(@Valid @RequestBody AddFolderRequest request) {
        return ResponseEntity.status(HttpStatus.CREATED).body(libraryService.addFolder(request));
    }

    @DeleteMapping("/folders/{id}")
    public ApiResponse<Void> removeFolder(@PathVariable Long id) {
        libraryService.removeFolder(id);
        return ApiResponse.message("Folder removed");
    }

    // ─── Stats ───────────────────────────────────────────────────────────────

    @GetMapping("/stats")
    public Map<String, Object> getStats() {
        return libraryService.getStats();
    }
}
